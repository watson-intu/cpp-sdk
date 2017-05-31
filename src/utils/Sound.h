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


#ifndef WDC_SOUND_H
#define WDC_SOUND_H

#include "ISerializable.h"
#include "UtilsLib.h"

class UTILS_API Sound : public ISerializable
{
public:
	RTTI_DECL();

	// Construction
	Sound();
	Sound(const std::string & a_WaveData);

	// ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	// Accessors
	//! This returns the sample rate of the contained sound.
	int	GetRate() const
	{
		return m_Rate;
	}
	//! This returns the number of channels
	int	GetChannels() const
	{
		return m_Channels;
	}
	//! This returns the bits per sample.
	int	GetBits() const
	{
		return m_Bits;
	}
	//! This returns a reference to the PCM data for the sound as contained by a std::string.
	const std::string & GetWaveData() const
	{
		return m_WaveData;
	}
	//! This function calculates the length of the contained sound in seconds.
	float GetTime(int rate = -1) const
	{
		if( m_Bits < 0 || m_Channels < 0 || m_Rate < 0 )
			return 0.0f;
		if (rate < 0)
			rate = m_Rate;

		int bytesPerSecond = rate * ((m_Bits >> 3) * m_Channels);
		return(float(m_WaveData.size()) / bytesPerSecond);
	}

	// Mutators

	//! Initialize this sound object with the provided parameters
	void InitializeSound(int a_Rate, int a_Channels, int a_Bits, const std::string & a_WaveData);
	//! Clear the contained sound data.
	void Release();

	//! Load a WAV formatted sound from the provided data buffer.
	bool Load( const std::string & a_WaveData );
	//! Save this sound into the provided buffer in WAV format.
	bool Save( std::string & a_WaveData ) const;
	//! Load from a file on disk using the provided filename.
	bool LoadFromFile(const std::string & a_FileName);
	//! Save to a file on disk using the provided filename.
	bool SaveToFile(const std::string & a_FileName) const;

	//! Load into this sound from partial incoming stream data, this function
	//! can be called multiple times with additional data.
	bool LoadFromStream(const std::string & a_StreamData);
	//! This can be called to reset this object if you plan to start calling LoadFromStream()
	//! with different stream data.
	bool ResetLoadFromStream();
	//! Resample the contained sound to the provided sample rate.
	bool Resample( int a_Rate );

	// Static
	static bool	LoadWave(const std::string & a_FileData, int & a_Rate, int & a_Channels, int & a_Bits,
		std::string & a_WaveData);
	static bool SaveWave(std::string & a_FileData, int nRate, int nChannels, int nBits,
		const std::string & a_WaveData);

private:
	// Data
	int					m_Rate;				// sample rate
	int					m_Channels;			// number of channels
	int					m_Bits;				// bits per sample
	std::string			m_WaveData;			// PCM data container

	//! Streaming data members
	std::string			m_StreamData;		// buffer for incoming stream data
	bool				m_bHeaderStreamed;	// LoadFromStream() will set to true once the header has been read
};


#endif

