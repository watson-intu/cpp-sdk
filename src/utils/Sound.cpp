/**
* Copyright 2017 IBM Corp. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/


#include <fstream>
#include <streambuf>
#include <string>
#include <string.h>	// memcpy

#include "Sound.h"
#include "WatsonException.h"

RTTI_IMPL(Sound, ISerializable);

Sound::Sound() : m_Rate(-1), m_Channels(-1), m_Bits(-1), m_bHeaderStreamed( false )
{}

Sound::Sound(const std::string & a_WaveData) : m_Rate(-1), m_Channels(-1), m_Bits(-1), m_bHeaderStreamed( false )
{
	if (! LoadWave( a_WaveData, m_Rate, m_Channels, m_Bits, m_WaveData ) )
		throw WatsonException( "LoadWave() failed." );
}

void Sound::Serialize(Json::Value & json)
{
	json["m_Rate"] = m_Rate;
	json["m_Channels"] = m_Channels;
	json["m_Bits"] = m_Bits;
	json["m_WaveData"] = m_WaveData;
}

void Sound::Deserialize(const Json::Value & json)
{
	if ( json.isMember( "m_Rate" ) )
		m_Rate = json["m_Rate"].asInt();
	if ( json.isMember( "m_Channels" ) )
		m_Channels = json["m_Channels"].asInt();
	if ( json.isMember( "m_Bits" ) )
		m_Bits = json["m_Bits"].asInt();
	if ( json.isMember( "m_WaveData" ) )
		m_WaveData = json["m_WaveData"].asString();
}


//----------------------------------------------------------------------------

void Sound::InitializeSound(int a_Rate, int a_Channels, int a_Bits, const std::string & a_WaveData)
{
	Release();

	m_Rate = a_Rate;
	m_Channels = a_Channels;
	m_Bits = a_Bits;
	m_WaveData = a_WaveData;
}

void Sound::Release()
{
	m_WaveData.clear();
	m_Rate = -1;
	m_Channels = -1;
	m_Bits = -1;
	m_StreamData.clear();
	m_bHeaderStreamed = false;
}

bool Sound::Load( const std::string & a_WaveData )
{
	return LoadWave( a_WaveData, m_Rate, m_Channels, m_Bits, m_WaveData );
}

bool Sound::Save( std::string & a_WaveData ) const
{
	return SaveWave( a_WaveData, m_Rate, m_Channels, m_Bits, m_WaveData );
}

bool Sound::LoadFromFile(const std::string & a_FileName)
{
	std::ifstream input(a_FileName.c_str(), std::ios::binary );
	if (!input.is_open())
		return false;

	std::string fileData( (std::istreambuf_iterator<char>(input) ),
		(std::istreambuf_iterator<char>()) );
	return LoadWave( fileData, m_Rate, m_Channels, m_Bits, m_WaveData );
}

bool Sound::SaveToFile(const std::string & a_FileName) const
{
	std::string fileData;
	if (! SaveWave( fileData, m_Rate, m_Channels, m_Bits, m_WaveData ) )
		return false;

	try {
		std::ofstream output(a_FileName.c_str(), std::ios::binary );
		output << fileData;
	}
	catch( const std::exception & ex )
	{
		Log::Error( "Sound::SaveToFile() caught exception: %s", ex.what() );
		return false;
	}

	return true;
}

bool Sound::LoadFromStream(const std::string & a_StreamData)
{
	if ( m_bHeaderStreamed )
	{
		// we have already read the header, just append any additional data as PCM data
		m_WaveData += a_StreamData;
		return true;
	}

	m_StreamData += a_StreamData;
	if ( LoadWave( m_StreamData, m_Rate, m_Channels, m_Bits, m_WaveData ) )
	{
		m_bHeaderStreamed = true;
		m_StreamData.clear();
	}

	return true;
}

bool Sound::ResetLoadFromStream()
{
	m_StreamData.clear();
	m_bHeaderStreamed = false;
	return true;
}

#pragma warning(disable: 4244 )

bool Sound::Resample( int a_Rate )
{
	if ( m_Bits == 16 )
	{
		size_t bps = m_Channels * (m_Bits / 8);				// bytes per sample
		size_t newSamples = (size_t)((m_WaveData.size() / bps) * a_Rate) / m_Rate;
		size_t newSize = newSamples * bps;

		std::string resampled;
		resampled.resize( newSize );

		for(size_t i = 0; i< newSamples ; i += m_Channels )
		{
			size_t s = (size_t)(i * m_Rate) / a_Rate;
			for(size_t j=0;j<(size_t)m_Channels;++j)
				*(((unsigned short *)resampled.data()) + i + j) = *(((unsigned short *)m_WaveData.data()) + s + j);
		}

		m_Rate = a_Rate;
		m_WaveData = resampled;
		return true;
	}

	Log::Error( "Sound", "Unsupported bits for Resample." );
	return false;
}

//----------------------------------------------------------------------------

typedef unsigned short	WORD;
typedef unsigned int	DWORD;
typedef unsigned char	BYTE;

typedef DWORD	IdTag;

#pragma pack( push, 1 )
struct IFF_FORM_CHUNK
{
	IdTag		form_id;
	DWORD		form_length;
	IdTag		id;
};

struct IFF_CHUNK
{
	IdTag		id;
	int			length;
};

struct WAV_PCM
{
	WORD  format_tag;
	WORD  channels;
	int  sample_rate;
	int  average_data_rate;
	WORD  alignment;
	WORD  bits_per_sample;
};

struct WAV_CUE
{
	DWORD	number;					// cue number
	DWORD	start;					// starting sample frame
	DWORD	id;						// cue id
	DWORD	reserved1, reserved2;	// ?
	DWORD	end;					// ending frame
};

struct WAV_CUE_LABEL
{
	IFF_CHUNK	chunk;
	DWORD		number;					// cue number
	DWORD		reserved;
	char		name;					// cue name, length of string to end of chunk + chunk.length
};
#pragma pack(pop)

#define CHUNKID(string)		(*((IdTag *)(string)))
#define IDTAG(a,b,c,d)		((IdTag)((d)<<24)|((c)<<16)|((b)<<8)|(a))

bool Sound::LoadWave( const std::string & a_FileData, int & a_Rate, int & a_Channels, int & a_Bits,
	std::string & a_WaveData )
{
	const BYTE * pFile = (const BYTE *)&a_FileData[0];

	IFF_FORM_CHUNK	*pForm = (IFF_FORM_CHUNK *)pFile;
	if ( (((BYTE *)pForm) + sizeof(IFF_FORM_CHUNK)) > (BYTE *)&a_FileData[ a_FileData.size() - 1 ] )
		return false;
	if (pForm->form_id != CHUNKID("RIFF") || pForm->id != CHUNKID("WAVE"))
	{
		Log::Error( "Sound", "RIFF/WAVE header not found." );
		return false;
	}

	IFF_CHUNK *pEnd = (IFF_CHUNK *)(pFile + a_FileData.size());
	IFF_CHUNK *pChunk = (IFF_CHUNK *)(pFile + sizeof(IFF_FORM_CHUNK));

	while (pChunk < pEnd)
	{
		if ( (((BYTE *)pChunk) + sizeof(IFF_CHUNK)) > (BYTE *)pEnd )
			return false;		
		int chunkLength = pChunk->length;
		if ( chunkLength < 0 )		// handle -1 chunk lengths passed down by TTS service..
			chunkLength = (BYTE *)pEnd - (BYTE *)(pChunk + 1);

		switch (pChunk->id)
		{
		case IDTAG('f', 'm', 't', ' '):
		{
			WAV_PCM *pPCM = (WAV_PCM *)(pChunk + 1);
			if ( (((BYTE *)pPCM) + sizeof(WAV_PCM)) > (BYTE *)pEnd )
				return false;

			a_Rate = pPCM->sample_rate;
			a_Channels = pPCM->channels;
			a_Bits = pPCM->bits_per_sample;
		}
		break;
		case IDTAG('d', 'a', 't', 'a'):
		{
			a_WaveData.resize( chunkLength );
			memcpy( &a_WaveData[0], pChunk + 1, chunkLength );
		}
		break;
		default:
			break;
		}

		pChunk = (IFF_CHUNK *)(((BYTE *)(pChunk + 1)) + chunkLength);
	}

	return true;
}

bool Sound::SaveWave(std::string & a_FileData, int nRate, int nChannels, int nBits, const std::string & a_WaveData )
{
	IFF_FORM_CHUNK form;
	form.form_id = CHUNKID("RIFF");
	form.form_length = sizeof(IFF_CHUNK) + sizeof(WAV_PCM) +
		sizeof(IFF_CHUNK) + a_WaveData.size() + sizeof(form.id);
	form.id = CHUNKID("WAVE");

	IFF_CHUNK format;
	format.id = CHUNKID("fmt ");
	format.length = sizeof(WAV_PCM);

	WAV_PCM pcm;
	pcm.format_tag = 0x1;
	pcm.channels = nChannels;
	pcm.sample_rate = nRate;
	pcm.average_data_rate = (nRate * nBits * nChannels) / 8;
	pcm.alignment = 0x2;
	pcm.bits_per_sample = nBits;

	IFF_CHUNK data;
	data.id = CHUNKID("data");
	data.length = a_WaveData.size();

	std::stringstream ss;
	ss.write( (const char *)&form, sizeof(form) );
	ss.write( (const char *)&format, sizeof(format) );
	ss.write( (const char *)&pcm, sizeof(pcm) );
	ss.write( (const char *)&data, sizeof(data) );
	ss.write( a_WaveData.c_str(), a_WaveData.size() );

	a_FileData = ss.str();
	return true;
}

