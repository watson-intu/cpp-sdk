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
	m_Voice( "en-GB_KateVoice" )
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

void TextToSpeech::GetVoices( GetVoicesCallback callback )
{
	new RequestObj<Voices>( this, "/v1/voices", "GET", NULL_HEADERS, EMPTY_STRING, callback );
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

	TextToSpeech::ToSoundCallback	m_Callback;
};

void TextToSpeech::Synthesis( const std::string & a_Text, AudioFormatType a_eFormat, Delegate<const std::string &> a_Callback, 
	bool a_isStreaming /* = false */ )
{
	if (!a_isStreaming)
	{
		Json::Value json;
		json["text"] = a_Text;
		Headers headers;
		headers["Content-Type"] = "application/json";

		std::string cacheName(m_Voice + "_" + GetFormatName(a_eFormat));

		new RequestData(this, "/v1/synthesize?accept=" + GetFormatId(a_eFormat) + "&voice=" + m_Voice,
			"POST", headers, json.toStyledString(), a_Callback,
			new IService::CacheRequest(cacheName, StringHash::DJB(a_Text.c_str())));
	}
}

void TextToSpeech::ToSound( const std::string & a_Text, ToSoundCallback a_Callback )
{
	Json::Value json;
	json["text"] = a_Text;
	Headers headers;
	headers["Content-Type"] = "application/json";

	std::string cacheName(m_Voice + "_" + GetFormatName(AF_WAV));

	new RequestSound(this, "/v1/synthesize?accept=audio/wav&voice=" + m_Voice,
		"POST", headers, json.toStyledString(), a_Callback,
		new IService::CacheRequest(cacheName, StringHash::DJB(a_Text.c_str())));
}

void TextToSpeech::ToSound( const std::string & a_Text, 
	StreamCallback a_StreamCallback, 
	WordsCallback a_WordsCallback /*= WordsCallback()*/ )
{
	Connection::SP spConnection(new Connection(this, a_Text, a_StreamCallback, a_WordsCallback));
	if (!spConnection->Start())
	{
		Log::Error("TextToSpeech", "Failed to start streaming Text To Speech service. Can't continue..");
		a_StreamCallback(NULL);
	}
	else
		m_Connections.push_back(spConnection);
}


std::string & TextToSpeech::GetFormatName(AudioFormatType a_eFormat)
{
	static std::map<AudioFormatType, std::string> map;
	static bool map_init = false;
	if (!map_init)
	{
		map[AF_OGG] = "ogg";
		map[AF_WAV] = "wav";
		map[AF_FLAC] = "flac";
		map_init = true;
	}

	return map[a_eFormat];
}

std::string & TextToSpeech::GetFormatId(AudioFormatType a_eFormat)
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

// ----------------------------

TextToSpeech::Connection::Connection(TextToSpeech * a_pTTS, const std::string & a_Text, StreamCallback a_Callback, WordsCallback a_WordsCallback  ) :
	m_pTTS(a_pTTS),
	m_Text(a_Text),
	m_Callback(a_Callback),
	m_WordsCallback(a_WordsCallback)
{}

bool TextToSpeech::Connection::Start()
{
	std::string url = m_pTTS->GetConfig()->m_URL + "/v1/synthesize?voice=" + m_pTTS->m_Voice;
	StringUtil::Replace(url, "https://", "wss://", true);
	StringUtil::Replace(url, "http://", "ws://", true);

	m_spSocket = IWebClient::Create( url );
	m_spSocket->SetHeaders(m_pTTS->m_Headers);
	m_spSocket->SetFrameReceiver(DELEGATE(Connection, OnListenMessage, IWebSocket::FrameSP, shared_from_this()));
	m_spSocket->SetStateReceiver(DELEGATE(Connection, OnListenState, IWebClient *, shared_from_this()));

	if (!m_spSocket->Send())
	{
		Log::Error("TextToSpeech", "Failed to connect web socket to %s", url.c_str());
		return false;
	}

	Json::Value root;
	root["text"] = m_Text;
	root["accept"] = "audio/wav";

	std::vector<std::string> words;
	words.push_back("words");
	SerializeVector("timings", words, root);

	m_spSocket->SendText(Json::FastWriter().write(root));
	return true;
}

void TextToSpeech::Connection::OnListenMessage(IWebSocket::FrameSP a_spFrame)
{
	if (a_spFrame->m_Op == IWebSocket::TEXT_FRAME)
	{
		Json::Value json;
		Json::Reader reader(Json::Features::strictMode());
		if (reader.parse(a_spFrame->m_Data, json))
		{
			if (json.isMember("words"))
			{
				if ( m_WordsCallback.IsValid() )
				{
					Json::Value value = json["words"][0];
					std::string word = value[0].asString();
					double startTime = value[1].asDouble();
					double endTime = value[2].asDouble();

					m_WordsCallback( new Words(word,startTime,endTime) );
				}
			}
		}
		else
			Log::Error("TextToSpeech", "Failed to parse response from TTS service: %s", a_spFrame->m_Data.c_str());
	}
	else if (a_spFrame->m_Op == IWebSocket::BINARY_FRAME)
	{
		if ( m_Callback.IsValid() )
			m_Callback( new std::string( a_spFrame->m_Data ) );
	}
}

void TextToSpeech::Connection::OnListenState(IWebClient * a_pClient)
{
	if (a_pClient == m_spSocket.get())
	{
		if (a_pClient->GetState() == IWebClient::DISCONNECTED || a_pClient->GetState() == IWebClient::CLOSED)
		{
			if (m_Callback.IsValid())
				m_Callback( NULL );

			m_pTTS->m_Connections.remove(shared_from_this());
		}
	}
}

