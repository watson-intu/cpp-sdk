/**
* Copyright 2015 IBM Corp. All Rights Reserved.
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

#include "TextToSpeech.h"
#include "utils/StringHash.h"

REG_SERIALIZABLE( TextToSpeech );
RTTI_IMPL( Voice, ISerializable );
RTTI_IMPL( Voices, ISerializable );
RTTI_IMPL( TextToSpeech, IService );



TextToSpeech::TextToSpeech() : IService( "TextToSpeechV1" ),
	m_Voice( "en-GB_KateVoice" ),
	m_AudioFormat( AF_WAV )
{}	

void TextToSpeech::Serialize(Json::Value & json)
{
	IService::Serialize(json);
	json["m_Voice"] = m_Voice;
}

void TextToSpeech::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
	if (json.isMember("m_Voice"))
		m_Voice = json["m_Voice"].asString();
}

bool TextToSpeech::Start()
{
	if (! IService::Start() )
		return false;

	if (! StringUtil::EndsWith( m_pConfig->m_URL, "text-to-speech/api" ) )
	{
		Log::Error( "TextToSpeech", "Configured URL not ended with text-to-speech/api" );
		return false;
	}

	return true;
}

//! Check the status of the service and call the passed function with the result
void TextToSpeech::GetServiceStatus(ServiceStatusCallback a_Callback)
{
	if (m_pConfig != NULL)
		new ServiceStatusChecker(this, a_Callback);
	else
		a_Callback(ServiceStatus(m_ServiceId, false));
}

//! Creates an object responsible for service status checking
TextToSpeech::ServiceStatusChecker::ServiceStatusChecker(TextToSpeech* a_pTtsService, ServiceStatusCallback a_Callback)
	: m_pTtsService(a_pTtsService), m_Callback(a_Callback)
{
	m_pTtsService->GetVoices(DELEGATE(TextToSpeech::ServiceStatusChecker, OnCheckService, Voices *, this));
}

//! Callback function invoked when service status is checked
void TextToSpeech::ServiceStatusChecker::OnCheckService(Voices* a_pVoices)
{
	if (m_Callback.IsValid())
		m_Callback(ServiceStatus(m_pTtsService->m_ServiceId, a_pVoices != NULL));

	delete a_pVoices;
	delete this;
}

bool TextToSpeech::GetVoices( GetVoicesCallback callback )
{
	new RequestObj<Voices>( this, "/v1/voices", "GET", NULL_HEADERS, EMPTY_STRING, callback );
	return true;
}

class RequestSound : public IService::RequestData
{
public:
	RequestSound( IService * a_pService,
		const std::string & a_Parameters,		// additional data to append onto the endpoint
		const std::string & a_RequestType,		// type of request GET, POST, DELETE
		const Headers & a_Headers,				// additional headers to add to the request
		const std::string & a_Body,				// the body to send if any
		Delegate<Sound *> a_Callback,
		IService::CacheRequest * a_pCacheReq,
		float a_fTimeOut = 30.0f ) :
		m_Callback(a_Callback),
		RequestData(a_pService, a_Parameters, a_RequestType, a_Headers, a_Body,
			DELEGATE(RequestSound, OnResponse, const std::string &, this), a_pCacheReq, a_fTimeOut )
	{}

private:
	void OnResponse( const std::string & a_Data )
	{
		Sound * pSound = new Sound();
		if (! pSound->Load( a_Data ) )
		{
			Log::Error( "RequestSound", "Failed to parse sound." );
			delete pSound;
			pSound = NULL;
		}

		if ( m_Callback.IsValid() )
			m_Callback( pSound );
	}

	TextToSpeech::ToSpeechCallback	m_Callback;
};

bool TextToSpeech::ToSpeech( const std::string & a_Text, ToSpeechCallback a_Callback )
{
	Json::Value json;
	json["text"] = a_Text;
	Headers headers;
	headers["Content-Type"] = "application/json";

	new RequestSound( this, "/v1/synthesize?accept=" + GetFormatName( m_AudioFormat ) + "&voice=" + m_Voice,
		"POST", headers, json.toStyledString(), a_Callback, 
		new IService::CacheRequest( m_Voice, StringHash::DJB( a_Text.c_str() ) ) );

	return true;
}

std::string & TextToSpeech::GetFormatName(AudioFormatType a_eFormat)
{
	static std::map<AudioFormatType, std::string> map;
	static bool map_init = false;
	if (!map_init)
	{
		map[AF_OGG] = "audio/ogg;codecs=opus";
		map[AF_WAV] = "audio/wav";
		map[AF_FLAC] = "audio/flac";
		map_init = true;
	}

	return map[a_eFormat];
}

