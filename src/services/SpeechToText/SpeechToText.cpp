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

const float WS_KEEP_ALIVE_TIME = 10.0f;
const float LISTEN_TIMEOUT = 60.0f;
const int MAX_QUEUED_RECORDINGS = 30 * 8;
const int MAX_RECOGNIZE_CLIP_SIZE = 4 * (1024 * 1024);
const float RECONNECT_TIME = 1.0f;

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
	m_IsListening( false ),
	m_MaxAlternatives( 1 ),
	m_Timestamps( false ),
	m_WordConfidence( false ),
	m_Continous( true ),
	m_Interium( true ),
	m_Timeout( 30 ),
	m_DetectSilence( true ),
	m_SilenceThreshold( 0.03f ),
	m_MaxAudioQueueSize( 1024 * 1024 ),		// default to 1MB of audio data
	m_bLearningOptOut( true ),
	m_fResultDelay( 0.05f )
{}

SpeechToText::~SpeechToText()
{}

void SpeechToText::Serialize(Json::Value & json)
{
	IService::Serialize( json );

	SerializeVector( "m_Models", m_Models, json );
	json["m_MaxAlternatives"] = m_MaxAlternatives;
	json["m_Timestamps"] = m_Timestamps;
	json["m_WordConfidence"] = m_WordConfidence;
	json["m_Continous"] = m_Continous;
	json["m_Interium"] = m_Interium;
	json["m_DetectSilence"] = m_DetectSilence;
	json["m_SilenceThreshold"] = m_SilenceThreshold;
	json["m_MaxAudioQueueSize"] = m_MaxAudioQueueSize;
	json["m_LearningOptOut"] = m_bLearningOptOut;
	json["m_fResultDelay"] = m_fResultDelay;
	json["m_Timeout"] = m_Timeout;
}

void SpeechToText::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);

	DeserializeVector( "m_Models", json, m_Models );
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
	if (json.isMember("m_LearningOptOut"))
		m_bLearningOptOut = json["m_LearningOptOut"].asBool();
	if (json.isMember("m_fResultDelay"))
		m_fResultDelay = json["m_fResultDelay"].asFloat();
	if (json.isMember("m_Timeout"))
		m_Timeout = json["m_Timeout"].asInt();

	if ( m_Models.size() == 0 )
		m_Models.push_back( "en-US_BroadbandModel" );
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

void SpeechToText::RefreshConnections()
{
	if ( m_IsListening )
	{
		for( Connectionlist::iterator iConn = m_Connections.begin(); iConn != m_Connections.end(); ++iConn )
			(*iConn)->Refresh();	
	}	
}

bool SpeechToText::StartListening(OnRecognize callback)
{
	if (!callback.IsValid())
		return false;
	if (m_IsListening)
		return false;

	m_IsListening = true;
	m_ListenCallback = callback;

	for( ModelList::iterator iModels = m_Models.begin(); iModels != m_Models.end(); ++iModels )
	{
		Connection::SP spConnection( new Connection( this, *iModels ) );
		spConnection->Start();

		m_Connections.push_back( spConnection );
	}

	return true;
}

void SpeechToText::OnListen(const SpeechAudioData & clip)
{
	if (m_IsListening)
	{
		for( Connectionlist::iterator iConn = m_Connections.begin(); iConn != m_Connections.end(); ++iConn )
			(*iConn)->SendAudio( clip );
	}
}

bool SpeechToText::StopListening()
{
	if (!m_IsListening)
		return false;

	m_IsListening = false;
	m_Connections.clear();
	m_ListenCallback.Reset();

	return true;
}

void SpeechToText::Disconnected()
{
	for( Connectionlist::iterator iConn = m_Connections.begin(); iConn != m_Connections.end(); ++iConn )
		(*iConn)->Disconnected();
}

bool SpeechToText::SetLearningOptOut(bool a_Flag)
{
	if(m_bLearningOptOut != a_Flag)
	{
		m_bLearningOptOut = a_Flag;

		OnRecognize callback = m_ListenCallback;	// save the callback so we can restore it..
		StopListening();
		StartListening(callback);	
	}

	return true;
}

//---------------------------------------

SpeechToText::Connection::Connection( SpeechToText * a_pSTT, const std::string & a_RecognizeModel /*= "en-US_BroadbandModel"*/ ) : 
	m_pSTT( a_pSTT ), 
	m_RecognizeModel( a_RecognizeModel ),
	m_Language( m_RecognizeModel.substr( 0, m_RecognizeModel.find_first_of('_') ) ),
	m_ListenActive( false ),
	m_AudioSent( false ),
	m_Connected( false ),
	m_RecordingHZ( -1 ),
	m_bReconnecting( false )
{}

SpeechToText::Connection::~Connection()
{
	CloseListenConnector();
}

void SpeechToText::Connection::Start()
{
	if (! CreateListenConnector() )
		OnReconnect();
}

void SpeechToText::Connection::Refresh()
{
	m_Connected = false;
	m_ListenActive = false;
	m_spListenSocket->Close();

	CloseListenConnector();
	if (! CreateListenConnector() )
		OnReconnect();
}

void SpeechToText::Connection::SendAudio(const SpeechAudioData & clip)
{
	if (m_RecordingHZ < 0)
	{
		m_RecordingHZ = clip.m_Rate;
		SendStart();
	}

	if (!m_pSTT->m_DetectSilence || clip.m_Level >= m_pSTT->m_SilenceThreshold)
	{
		if (m_ListenActive)
		{
			m_spListenSocket->SendBinary( clip.m_PCM );
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
				if (m_pSTT->m_OnError.IsValid())
					m_pSTT->m_OnError("Recording queue is full.");
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
		m_spListenSocket->Close();		// close the socket, this will trigger the reconnect logic.

		Log::Error("SpeechToText", "Failed to enter listening state.");

		if (m_pSTT->m_OnError.IsValid())
			m_pSTT->m_OnError("Failed to enter listening state.");
	}
}

bool SpeechToText::Connection::CreateListenConnector()
{
	if (!m_spListenSocket)
	{
		std::string learningOptOut( m_pSTT->m_bLearningOptOut ? "1" : "0" );
		std::string url = m_pSTT->GetConfig()->m_URL + "/v1/recognize?x-watson-learning-opt-out=" + learningOptOut + "&model=" + StringUtil::UrlEscape( m_RecognizeModel );
		StringUtil::Replace(url, "https://", "wss://", true );
		StringUtil::Replace(url, "http://", "ws://", true );

		m_spListenSocket = IWebClient::Create( url );
		m_spListenSocket->SetHeaders(m_pSTT->m_Headers);
		m_spListenSocket->SetFrameReceiver( DELEGATE( Connection, OnListenMessage, IWebSocket::FrameSP, shared_from_this() ) );
		m_spListenSocket->SetStateReceiver( DELEGATE( Connection, OnListenState, IWebClient *, shared_from_this() ) );
		m_spListenSocket->SetDataReceiver( DELEGATE( Connection, OnListenData, IWebClient::RequestData *, shared_from_this() ) );

		if (! m_spListenSocket->Send() )
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

void SpeechToText::Connection::CloseListenConnector()
{
	m_spListenSocket.reset();
}

void SpeechToText::Connection::Disconnected()
{
	if ( m_spListenSocket ) 
	{
	    m_Connected = false;
		m_spListenSocket->Close();
	}
}

void SpeechToText::Connection::SendStart()
{
	if (!m_spListenSocket)
		throw WatsonException("SendStart() called with null connector.");

	Json::Value start;
	start["action"] = "start";
	start["content-type"] = StringUtil::Format("audio/l16;rate=%u;channels=1;", m_RecordingHZ );
	start["continuous"] = m_pSTT->m_Continous;
	start["max_alternatives"] = m_pSTT->m_MaxAlternatives;
	start["interim_results"] = m_pSTT->m_Interium;
	start["word_confidence"] = m_pSTT->m_WordConfidence;
	start["timestamps"] = m_pSTT->m_Timestamps;
	start["inactivity_timeout"] = m_pSTT->m_Timeout;

	m_spListenSocket->SendText( Json::FastWriter().write( start ) );
	m_LastStartSent = Time();
}

void SpeechToText::Connection::SendStop()
{
	if (!m_spListenSocket)
		throw WatsonException("SendStart() called with null connector.");

	if (m_ListenActive)
	{
		Json::Value stop;
		stop["action"] = "stop";

		m_spListenSocket->SendText( Json::FastWriter().write( stop ) );
		m_LastStartSent = Time();     // sending stop, will send the listening state again..
		m_ListenActive = false;
	}
}

// This keeps the WebSocket connected when we are not sending any data.
void SpeechToText::Connection::KeepAlive()
{
	if (m_spListenSocket)
	{
		Json::Value nop;
		nop["action"] = "no-op";

		Log::Debug( "SpeechToText", "Sending keep alive." );
		m_spListenSocket->SendText( Json::FastWriter().write( nop ) );
	}
}

void SpeechToText::Connection::OnListenMessage( IWebSocket::FrameSP a_spFrame )
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
					pResults->SetLanguage( m_Language );

					// when we get results, start listening for the next block ..
					// if continuous is true, then we don't need to do this..
					if (!m_pSTT->m_Continous && pResults->HasFinalResult())
						SendStart();

					m_pSTT->OnReconizeResult( pResults );
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
					if (m_pSTT->m_IsListening)
					{
						if (!m_ListenActive)
						{
							m_ListenActive = true;

							// send all pending audio clips ..
							while (m_ListenRecordings.size() > 0)
							{
								const SpeechAudioData & clip = m_ListenRecordings.front();
								m_spListenSocket->SendBinary( clip.m_PCM );
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

				if ( m_spListenSocket )		// this may be NULL, since this callback can be invoked after we already called CloseListeningConnection().
					m_spListenSocket->Close();
				if (m_pSTT->m_OnError.IsValid())
					m_pSTT->m_OnError(error);
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

void SpeechToText::Connection::OnListenState( IWebClient * a_pClient )
{
	if ( a_pClient == m_spListenSocket.get() )
	{
		if ( m_pSTT->m_IsListening )
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
}

void SpeechToText::Connection::OnListenData( IWebClient::RequestData * a_pData )
{
	const std::string & transId = a_pData->m_Headers["X-Global-Transaction-ID"];
	const std::string & dpTransId = a_pData->m_Headers["X-DP-Watson-Tran-ID"];

	Log::Status( "SpeechToText", "Connected, Global Transaction ID: %s, DP Transaction ID: %s",
		transId.c_str(), dpTransId.c_str() );
}

void SpeechToText::Connection::OnReconnect()
{
	if ( m_pSTT->m_IsListening )
	{
		if (! m_bReconnecting )
		{
			// if not in the correct state and not already trying to reconnect, go ahead and try to connect in a few seconds.
			Log::Status( "SpeechToText", "Disconnected, will try to reconnect in %f seconds.", RECONNECT_TIME );
			m_spReconnectTimer = TimerPool::Instance()->StartTimer( 
				VOID_DELEGATE( Connection, OnReconnect, this ), RECONNECT_TIME, true, false );
			m_bReconnecting = true;
			m_Connected = false;
			m_ListenActive = false;
		}
		else if ( m_spListenSocket->GetState() == IWebClient::CLOSED 
			|| m_spListenSocket->GetState() == IWebClient::DISCONNECTED )
		{
			Log::Status( "SpeechToText", "Trying to reconnect." );

			// set to -1 so next time OnListen() is invoked, we'll send the send start
			m_RecordingHZ = -1;
			m_LastStartSent = Time();
			m_bReconnecting = false;
			m_spReconnectTimer.reset();

			// this may get recursive because it can call OnListenState()
			if (!m_spListenSocket->Send())
			{
				Log::Error("SpeechToText", "Failed to connect web socket to %s", m_spListenSocket->GetURL().GetURL().c_str());
				OnReconnect();
			}
		}
		else
		{
			Log::Error( "SpeechToText", "m_ListenSocket not getting into the correct state.");
			m_pSTT->StopListening();
		}
	}
}

void SpeechToText::GetModels(OnGetModels callback)
{
	new RequestObj<SpeechModels>( this, "/v1/models", "GET", NULL_HEADERS, EMPTY_STRING, callback );
}

void SpeechToText::Recognize(const SpeechAudioData & clip, OnRecognize callback, const std::string & a_RecognizeModel /*= "en-US_BroadbandModel"*/)
{
	Sound sound;
	sound.InitializeSound( clip.m_Rate, clip.m_Channels, clip.m_Bits, clip.m_PCM );

	Recognize( sound, callback, a_RecognizeModel );
}

void SpeechToText::Recognize( const Sound & sound, OnRecognize callback, const std::string & a_RecognizeModel /*= "en-US_BroadbandModel" */)
{
	std::string wav;
	if ( sound.Save( wav ) )
	{
		Headers headers;
		headers["Content-Type"] = "audio/wav";

		new RequestObj<RecognizeResults>( this, "/v1/recognize?model=" + StringUtil::UrlEscape( a_RecognizeModel ), 
			"POST", headers, wav, callback );
	}
	else
	{
		Log::Error( "SpeechToText", "Failed to save wav." );
	}
}

void SpeechToText::OnReconizeResult( RecognizeResults * a_pResult )
{
	m_ResultList.push_back( a_pResult );
	if ( m_ResultList.size() >= m_Models.size() )
		OnFinalizeResult();
	else
		m_spResultTimer = TimerPool::Instance()->StartTimer( VOID_DELEGATE( SpeechToText, OnFinalizeResult, this ), m_fResultDelay, true, false );
}

void SpeechToText::OnFinalizeResult()
{
	m_spResultTimer.reset();

	RecognizeResults * pBestResult = NULL;
	for( ResultList::iterator iResult = m_ResultList.begin(); iResult != m_ResultList.end(); ++iResult )
	{
		RecognizeResults * pResult = *iResult;
		if ( pBestResult == NULL || pResult->GetConfidence() > pBestResult->GetConfidence() )
		{
			if ( pBestResult != NULL )
				delete pBestResult;
			pBestResult = pResult;
		}
		else
			delete pResult;
	}
	m_ResultList.clear();

	if (!m_ListenCallback.IsValid())
	{
		StopListening();            // automatically stop listening if our callback is destroyed.
		delete pBestResult;
	}
	else
	{
		m_ListenCallback(pBestResult);
	}
}

//------------------------

//! Creates an object responsible for service status checking
SpeechToText::ServiceStatusChecker::ServiceStatusChecker(SpeechToText* a_pSttService, ServiceStatusCallback a_Callback)
	: m_pService(a_pSttService), m_Callback(a_Callback)
{
	m_pService->GetModels(DELEGATE(SpeechToText::ServiceStatusChecker, OnCheckService, SpeechModels *, this));
}

//! Callback function invoked when service status is checked
void SpeechToText::ServiceStatusChecker::OnCheckService(SpeechModels* a_pSpeechModels)
{
	if (m_Callback.IsValid())
		m_Callback(ServiceStatus(m_pService->m_ServiceId, a_pSpeechModels != NULL));

	delete a_pSpeechModels;
	delete this;
}

