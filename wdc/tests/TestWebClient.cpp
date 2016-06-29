/**
* Copyright 2016 IBM Corp. All Rights Reserved.
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

#include "UnitTest.h"
#include "utils/WebClient.h"
#include "utils/Log.h"
#include "utils/ThreadPool.h"
#include "utils/Time.h"

class TestWebClient : UnitTest
{
public:
	//! Construction
	TestWebClient() : UnitTest("TestWebClient"), m_bHTTPTested( false ), m_bHTTPSTested( false ), m_bWSTested( false )
	{}

	virtual void RunTest()
	{
		Time start;

		ThreadPool pool(1);

		WebClient client;
		client.SetURL( "wss://stream.watsonplatform.net/speech-to-text/api/v1/recognize" );
		client.SetHeader( "Authorization", "Basic ZTZlNThlYjAtNWU5Mi00OGJmLTlkNjctYTAyYTE1ODA3ZGRmOnJOZGhOZUZwS0hGeA==" );
		client.SetStateReceiver( DELEGATE( TestWebClient, OnState, IWebClient *, this ) );
		client.SetDataReceiver( DELEGATE( TestWebClient, OnWebSocketResponse, WebClient::RequestData *, this ) );
		client.SetFrameReceiver( DELEGATE( TestWebClient, OnFrameReceived, IWebSocket::FrameSP, this ) );
		Test( client.Send() );

		start = Time();
		while(! m_bWSTested && (Time().GetEpochTime() - start.GetEpochTime()) < 300.0 )
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}
		Test( m_bWSTested );
		client.Close();			// make sure we are closed

		client.Request( "http://www.boost.org/doc/libs/1_36_0/doc/html/boost_asio/example/http/client/async_client.cpp", WebClient::Headers(), "GET", "", 
			DELEGATE( TestWebClient, OnResponse, WebClient::RequestData *, this),
			DELEGATE( TestWebClient, OnState, IWebClient *, this ) );

		start = Time();
		while(! m_bHTTPTested && (Time().GetEpochTime() - start.GetEpochTime()) < 15.0 )
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}
		Test( m_bHTTPTested );

		client.Request( "https://github.com/eidheim/Simple-Web-Server", WebClient::Headers(), "GET", "", 
			DELEGATE(TestWebClient, OnSecureResponse, WebClient::RequestData *, this),
			DELEGATE(TestWebClient, OnState, IWebClient *, this));

		start = Time();
		while (!m_bHTTPSTested && (Time().GetEpochTime() - start.GetEpochTime()) < 15.0)
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}
		Test(m_bHTTPSTested);
	}
	void OnWebSocketResponse( WebClient::RequestData * a_pResonse )
	{
		Log::Debug( "TestWebClient", "WebSocket response, status code %u : %s", a_pResonse->m_StatusCode, a_pResonse->m_StatusMessage.c_str() );
	}

	void OnFrameReceived( IWebSocket::FrameSP a_spFrame )
	{
		Log::Debug( "TestWebClient", "Frame received: (%d) %s", a_spFrame->m_Op, a_spFrame->m_Data.c_str() );
	}

	void OnState( IWebClient * a_pConnector )
	{
		Log::Debug( "TestWebClient", "OnState(): %d", a_pConnector->GetState() );
		if ( a_pConnector->GetState() == WebClient::CLOSED )
			m_bWSTested = true;
	}

	void OnResponse( WebClient::RequestData * a_pResponse )
	{
		Log::Debug( "TestWebClient", "OnResponse(): Version: %s, Status: %u, Content: %s",
			a_pResponse->m_Version.c_str(), a_pResponse->m_StatusCode, a_pResponse->m_Content.c_str() );

		if ( a_pResponse->m_StatusCode == 200 )
			m_bHTTPTested = true;
	}
	void OnSecureResponse( WebClient::RequestData * a_pResponse )
	{
		Log::Debug("TestWebClient", "OnSecureResponse(): Version: %s, Status: %u, Content: %s",
			a_pResponse->m_Version.c_str(), a_pResponse->m_StatusCode, a_pResponse->m_Content.c_str());

		if (a_pResponse->m_StatusCode == 200)
			m_bHTTPSTested = true;
	}

	bool m_bHTTPTested;
	bool m_bHTTPSTested;
	bool m_bWSTested;
};

TestWebClient TEST_CONNECTOR;
