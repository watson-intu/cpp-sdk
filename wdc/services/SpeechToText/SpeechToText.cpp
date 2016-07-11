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

//! If enabled, audio is queued when the server is not listening..
#define ENABLE_AUDIO_QUEUE		0

#include "SpeechToText.h"
#include "utils/TimerPool.h"
#include "utils/StringUtil.h"
#include "utils/WatsonException.h"

const std::string SERVICE_ID = "SpeechToTextV1";
const float WS_KEEP_ALIVE_TIME = 10.0f;
const float LISTEN_TIMEOUT = 60.0f;
const int MAX_QUEUED_RECORDINGS = 30 * 8;
const int MAX_RECOGNIZE_CLIP_SIZE = 4 * (1024 * 1024);
const float RECONNECT_TIME = 5.0f;

REG_SERIALIZABLE( SpeechToText );
RTTI_IMPL( SpeechToText, IService );
RTTI_IMPL( SpeechModel, ISerializable );
RTTI_IMPL( SpeechModels, ISerializable );
RTTI_IMPL( WordConfidence, ISerializable );
RTTI_IMPL( TimeStamp, ISerializable );
RTTI_IMPL( SpeechAlt, ISerializable );
RTTI_IMPL( SpeechResult, ISerializable );
RTTI_IMPL( RecognizeResults, ISerializable );



SpeechToText::SpeechToText() : IService( "SpeechToTextV1" ),
	m_RecognizeModel("en-US_BroadbandModel" ),
	m_ListenActive( false ),
	m_Connected(false),
	m_ListenSocket( NULL ),
	m_AudioSent( false ),
	m_IsListening( false ),
	m_MaxAlternatives( 1 ),
	m_Timestamps( false ),
	m_WordConfidence( false ),
	m_Continous( true ),
	m_Interium( true ),
	m_DetectSilence( true ),
	m_SilenceThreshold( 0.03f ),
	m_MaxAudioQueueSize( 1024 * 1024 ),		// default to 1MB of audio data
	m_RecordingHZ( -1 ),
	m_bReconnecting( false )
{}

SpeechToText::~SpeechToText()
{
	assert( m_ListenSocket == NULL );
}

void SpeechToText::Serialize(Json::Value & json)
{
	IService::Serialize( json );

	json["m_RecognizeModel"] = m_RecognizeModel;
	json["m_MaxAlternatives"] = m_MaxAlternatives;
	json["m_Timestamps"] = m_Timestamps;
	json["m_WordConfidence"] = m_WordConfidence;
	json["m_Continous"] = m_Continous;
	json["m_Interium"] = m_Interium;
	json["m_DetectSilence"] = m_DetectSilence;
	json["m_SilenceThreshold"] = m_SilenceThreshold;
	json["m_MaxAudioQueueSize"] = m_MaxAudioQueueSize;
}

void SpeechToText::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);

	if (json.isMember("m_RecognizeModel"))
		m_RecognizeModel = json["m_RecognizeModel"].asString();
	if (json.isMember("m_MaxAlternatives"))
		m_MaxAlternatives = json["m_MaxAlternatives"].asInt();
	if (json.isMember("m_Timestamps"))
		m_Timestamps = json["m_Timestamps"].asBool();
	if (json.isMember("m_WordConfidence"))
		m_WordConfidence = json["m_WordConfidence"].asBool();
	if (json.isMember("m_Continous"))
		m_Continous = json["m_Continous"].asBool();
	if (json.isMember("m_Interium"))
		m_Interium = json["m_Interium"].asBool();
	if (json.isMember("m_DetectSilence"))
		m_DetectSilence = json["m_DetectSilence"].asBool();
	if (json.isMember("m_SilenceThreshold"))
		m_SilenceThreshold = json["m_SilenceThreshold"].asFloat();
	if (json.isMember("m_MaxAudioQueueSize"))
		m_MaxAudioQueueSize = json["m_MaxAudioQueueSize"].asUInt();
}

bool SpeechToText::Start()
{
	if (! IService::Start() )
		return false;

	if (! StringUtil::EndsWith( m_pConfig->m_URL, "speech-to-text/api" ) )
	{
		Log::Error( "SpeechToText", "Configured URL not ended with speech-to-text/api" );
		return false;
	}

	return true;
}

bool SpeechToText::Stop()
{
	StopListening();
	return IService::Stop();
}

//! Check the status of the service and call the passed function with the result
void SpeechToText::GetServiceStatus( ServiceStatusCallback a_Callback )
{
	if (m_pConfig != NULL)
		new ServiceStatusChecker(this, a_Callback);
	else
		a_Callback(ServiceStatus(m_ServiceId, false));
}

//! Creates an object responsible for service status checking
SpeechToText::ServiceStatusChecker::ServiceStatusChecker(SpeechToText* a_pSttService, ServiceStatusCallback a_Callback)
	: m_pSttService(a_pSttService), m_Callback(a_Callback)
{
	m_pSttService->GetModels(DELEGATE(SpeechToText::ServiceStatusChecker, OnCheckService, SpeechModels *, this));
}

//! Callback function invoked when service status is checked
void SpeechToText::ServiceStatusChecker::OnCheckService(SpeechModels* a_pSpeechModels)
{
	if (m_Callback.IsValid())
		m_Callback(ServiceStatus(m_pSttService->m_ServiceId, a_pSpeechModels != NULL));

	delete a_pSpeechModels;
	delete this;
}


bool SpeechToText::StartListening(OnRecognize callback)
{
	if (!callback.IsValid())
		return false;
	if (m_IsListening)
		return false;

	m_IsListening = true;
	m_ListenCallback = callback;

	if (! CreateListenConnector() )
		OnReconnect();
	m_spKeepAliveTimer = TimerPool::Instance()->StartTimer( 
		VOID_DELEGATE( SpeechToText, KeepAlive, this ), WS_KEEP_ALIVE_TIME, true, true ); 
	return true;
}

void SpeechToText::OnListen(const SpeechAudioData & clip)
{
	if (m_IsListening)
	{
		if (m_RecordingHZ < 0)
		{
			m_RecordingHZ = clip.m_Rate;
			SendStart();
		}

		if (!m_DetectSilence || clip.m_Level >= m_SilenceThreshold)
		{
			if (m_ListenActive)
			{
				m_ListenSocket->SendBinary( clip.m_PCM );
				m_AudioSent = true;
			}
#if ENABLE_AUDIO_QUEUE
			else
			{
				// we have not received the "listening" state yet from the server, so just queue
				// the audio clips until that happens.
				m_ListenRecordings.push_back( clip );

				unsigned int audio_bytes = 0;
				for( AudioQueue::iterator iAudio = m_ListenRecordings.begin(); iAudio != m_ListenRecordings.end(); ++iAudio )
					audio_bytes += (*iAudio).m_PCM.size();

				// check the length of this queue and do something if it gets too full.
				if (audio_bytes > m_MaxAudioQueueSize)
				{
					Log::Error("SpeechToText", "Recording queue is full, calling StopListening().");

					StopListening();
					if (m_OnError.IsValid())
						m_OnError("Recording queue is full.");
				}
			}
#endif
		}
		else if (m_AudioSent)
		{
			SendStop();
			m_AudioSent = false;
		}

		// After sending start, we should get into the listening state within the amount of time specified
		// by LISTEN_TIMEOUT. If not, then stop listening and record the error.
		if (!m_ListenActive && (Time().GetEpochTime() - m_LastStartSent.GetEpochTime()) > LISTEN_TIMEOUT)
		{
			m_LastStartSent = Time();
			m_ListenSocket->Close();		// close the socket, this will trigger the reconnect logic.

			Log::Error("SpeechToText", "Failed to enter listening state.");

			if (m_OnError.IsValid())
				m_OnError("Failed to enter listening state.");
		}
	}
}

bool SpeechToText::StopListening()
{
	if (!m_IsListening)
		return false;

	m_IsListening = false;
	CloseListenConnector();

	m_spKeepAliveTimer.reset();
	m_ListenRecordings.clear();
	m_ListenCallback.Reset();
	m_RecordingHZ = -1;

	return true;
}

bool SpeechToText::CreateListenConnector()
{
	if (m_ListenSocket == NULL)
	{
		std::string url = GetConfig()->m_URL + "/v1/recognize?x-watson-learning-opt-out=1&model=" + StringUtil::UrlEscape( m_RecognizeModel );
		StringUtil::Replace(url, "https://", "wss://", true );
		StringUtil::Replace(url, "http://", "ws://", true );

		m_ListenSocket = IWebClient::Create();
		m_ListenSocket->SetURL( url );
		m_ListenSocket->SetHeaders(m_Headers);
		m_ListenSocket->SetFrameReceiver( DELEGATE( SpeechToText, OnListenMessage, IWebSocket::FrameSP, this ) );
		m_ListenSocket->SetStateReceiver( DELEGATE( SpeechToText, OnListenState, IWebClient *, this ) );
		m_ListenSocket->SetDataReceiver( DELEGATE( SpeechToText, OnListenData, IWebClient::RequestData *, this ) );

		if (! m_ListenSocket->Send() )
		{
			m_Connected = false;
			Log::Error( "StreamSTT", "Failed to connect web socket to %s", url.c_str() );
			return false;
		}

		// set to -1 so next time OnListen() is invoked, we'll send the send start
		m_RecordingHZ = -1;
		m_LastStartSent = Time();
	}

	return true;
}

void SpeechToText::CloseListenConnector()
{
	if (m_ListenSocket != NULL)
	{
		// only delete if we are in the close or disconnected state, if it's still connected then OnListenState()
		// will delete the object when the state changes to disconnected or closed.
		if( m_ListenSocket->GetState() == IWebClient::CLOSED || m_ListenSocket->GetState() == IWebClient::DISCONNECTED )
			delete m_ListenSocket;
		m_ListenSocket = NULL;
	}
}

//! This can be invoked when the network is lost but before this class has detected
//! we have been disconnected.
void SpeechToText::Disconnected()
{
	if ( m_ListenSocket != NULL ) 
	{
	    m_Connected = false;
		m_ListenSocket->Close();
	}
}

void SpeechToText::SendStart()
{
	if (m_ListenSocket == NULL)
		throw WatsonException("SendStart() called with null connector.");

	Json::Value start;
	start["action"] = "start";
	start["content-type"] = StringUtil::Format("audio/l16;rate=%u;channels=1;", m_RecordingHZ );
	start["continuous"] = m_Continous;
	start["max_alternatives"] = m_MaxAlternatives;
	start["interim_results"] = m_Interium;
	start["word_confidence"] = m_WordConfidence;
	start["timestamps"] = m_Timestamps;
	start["inactivity_timeout"] = -1;

	m_ListenSocket->SendText( Json::FastWriter().write( start ) );
	m_LastStartSent = Time();
}

void SpeechToText::SendStop()
{
	if (m_ListenSocket == NULL)
		throw WatsonException("SendStart() called with null connector.");

	if (m_ListenActive)
	{
		Json::Value stop;
		stop["action"] = "stop";

		m_ListenSocket->SendText( Json::FastWriter().write( stop ) );
		m_LastStartSent = Time();     // sending stop, will send the listening state again..
		m_ListenActive = false;
	}
}

// This keeps the WebSocket connected when we are not sending any data.
void SpeechToText::KeepAlive()
{
	if (m_ListenSocket != NULL)
	{
		Json::Value nop;
		nop["action"] = "no-op";

		Log::Status( "SpeechToText", "Sending keep alive." );
		m_ListenSocket->SendText( Json::FastWriter().write( nop ) );
	}
}

void SpeechToText::OnListenMessage( IWebSocket::FrameSP a_spFrame )
{
	if ( a_spFrame->m_Op == IWebSocket::TEXT_FRAME )
	{
		Json::Value json;

		Json::Reader reader( Json::Features::strictMode() );
		if (reader.parse(a_spFrame->m_Data, json)) 
		{
			if (json.isMember("results"))
			{
				RecognizeResults * pResults = new RecognizeResults();
				if (ISerializable::DeserializeObject( json, pResults ) == pResults )
				{
					// Add language to RecognizeResults
					std::string language = StringUtil::RightTrim(m_RecognizeModel, "_");
					pResults->SetLanguage(language);
					// when we get results, start listening for the next block ..
					// if continuous is true, then we don't need to do this..
					if (!m_Continous && pResults->HasFinalResult())
						SendStart();

					if (m_ListenCallback.IsValid())
						m_ListenCallback(pResults);
					else
						StopListening();            // automatically stop listening if our callback is destroyed.
				}
				else
				{
					Log::Error("SpeechToText", "Failed to parse results: %s", a_spFrame->m_Data.c_str() );
					delete pResults;
				}
			}
			else if (json.isMember("state"))
			{
				std::string state = json["state"].asString();

	#if ENABLE_DEBUGGING
				Log.Debug("SpeechToText", "Server state is %s", state.c_str() );
	#endif
				if (state == "listening")
				{
					if (m_IsListening)
					{
						if (!m_ListenActive)
						{
							m_ListenActive = true;

							// send all pending audio clips ..
							while (m_ListenRecordings.size() > 0)
							{
								const SpeechAudioData & clip = m_ListenRecordings.front();
								m_ListenSocket->SendBinary( clip.m_PCM );
								m_ListenRecordings.pop_front();

								m_AudioSent = true;
							}
						}
					}
				}

			}
			else if (json.isMember("error"))
			{
				std::string error = json["error"].asString();
				Log::Error("SpeechToText", "Error: %s", error.c_str() );

				if ( m_ListenSocket != NULL )		// this may be NULL, since this callback can be invoked after we already called CloseListeningConnection().
					m_ListenSocket->Close();
				if (m_OnError.IsValid())
					m_OnError(error);
			}
			else
			{
				Log::Warning("SpeechToText", "Unknown message: %s", a_spFrame->m_Data.c_str() );
			}
		}
		else
		{
			Log::Error("SpeechToText", "Failed to parse JSON from server: %s", a_spFrame->m_Data.c_str() );
		}
	}
}

void SpeechToText::OnListenState( IWebClient * a_pClient )
{
	if ( a_pClient == m_ListenSocket )
	{
		if ( m_IsListening )
		{
			Log::Debug("SpeechToText", "m_ListenSocket.GetState() = %u", a_pClient->GetState() );
			if ( a_pClient->GetState() == IWebClient::DISCONNECTED || a_pClient->GetState() == IWebClient::CLOSED )
			{
				m_Connected = false;
				m_ListenActive = false;		// stop trying to send audio data
				OnReconnect();
			}
			else if ( a_pClient->GetState() == IWebClient::CONNECTED )
			{
				m_Connected = true;
			}
		}
	}
	else 
	{
		// handle a removed client when it finally decides to invoke this callback.
		if ( a_pClient->GetState() == IWebClient::DISCONNECTED || a_pClient->GetState() == IWebClient::CLOSED )
		{
			Log::Status("SpeechToText", "Deleting previous WebClient." );
			delete a_pClient;
		}
	}
}

void SpeechToText::OnListenData( IWebClient::RequestData * a_pData )
{
	const std::string & transId = a_pData->m_Headers["X-Global-Transaction-ID"];
	const std::string & dpTransId = a_pData->m_Headers["X-DP-Watson-Tran-ID"];

	Log::Status( "SpeechToText", "Connected, Global Transaction ID: %s, DP Transaction ID: %s",
		transId.c_str(), dpTransId.c_str() );
}

void SpeechToText::OnReconnect()
{
	if ( m_IsListening )
	{
		if (! m_bReconnecting )
		{
			// if not in the correct state and not already trying to reconnect, go ahead and try to connect in a few seconds.
			Log::Status( "SpeechToText", "Disconnected, will try to reconnect in %f seconds.", RECONNECT_TIME );
			m_spReconnectTimer = TimerPool::Instance()->StartTimer( 
				VOID_DELEGATE( SpeechToText, OnReconnect, this ), RECONNECT_TIME, true, false );
			m_bReconnecting = true;
			m_Connected = false;
			m_ListenActive = false;
		}
		else if ( m_ListenSocket->GetState() == IWebClient::CLOSED 
			|| m_ListenSocket->GetState() == IWebClient::DISCONNECTED )
		{
			Log::Status( "SpeechToText", "Trying to reconnect." );

			// set to -1 so next time OnListen() is invoked, we'll send the send start
			m_RecordingHZ = -1;
			m_LastStartSent = Time();
			m_bReconnecting = false;
			m_spReconnectTimer.reset();

			// this may get recursive because it can call OnListenState()
			if (!m_ListenSocket->Send())
			{
				Log::Error("SpeechToText", "Failed to connect web socket to %s", m_ListenSocket->GetURL().GetURL().c_str());
				OnReconnect();
			}
		}
		else
		{
			Log::Error( "SpeechToText", "m_ListenSocket not getting into the correct state.");
			StopListening();
		}
	}
}

IService::Request * SpeechToText::GetModels(OnGetModels callback)
{
	return new RequestObj<SpeechModels>( this, "/v1/models", "GET", NULL_HEADERS, EMPTY_STRING, callback );
}

IService::Request * SpeechToText::Recognize(const SpeechAudioData & clip, OnRecognize callback)
{
	Sound sound;
	sound.InitializeSound( clip.m_Rate, clip.m_Channels, clip.m_Bits, clip.m_PCM );

	return Recognize( sound, callback );
}

IService::Request * SpeechToText::Recognize( const Sound & sound, OnRecognize callback )
{
	std::string wav;
	if (! sound.Save( wav ) )
	{
		Log::Error( "SpeechToText", "Failed to save wav." );
		return NULL;
	}

	Headers headers;
	headers["Content-Type"] = "audio/wav";

	return new RequestObj<RecognizeResults>( this, "/v1/recognize?model=" + StringUtil::UrlEscape( m_RecognizeModel ), 
		"POST", headers, wav, callback );
}


