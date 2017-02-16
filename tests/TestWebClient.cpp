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
#include "utils/IWebClient.h"
#include "utils/Log.h"
#include "utils/ThreadPool.h"
#include "utils/Time.h"

class TestWebClient : UnitTest
{
public:
	bool m_bHTTPTested;
	bool m_bHTTPSTested;
	bool m_bWSTested;
	bool m_bWSClosed;

	//! Construction
	TestWebClient() : UnitTest("TestWebClient"), m_bHTTPTested( false ), m_bHTTPSTested( false ), m_bWSTested( false ), m_bWSClosed( false )
	{}

	virtual void RunTest()
	{
		Time start;

		ThreadPool pool(1);

		URL url( "wss://stream.watsonplatform.net/speech-to-text/api/v1/recognize" );
		IWebClient::SP spClient = IWebClient::Create( url );
		spClient->SetHeader( "Authorization", "Basic ZTZlNThlYjAtNWU5Mi00OGJmLTlkNjctYTAyYTE1ODA3ZGRmOnJOZGhOZUZwS0hGeA==" );
		spClient->SetStateReceiver( DELEGATE( TestWebClient, OnState, IWebClient *, this ) );
		spClient->SetDataReceiver( DELEGATE( TestWebClient, OnWebSocketResponse, IWebClient::RequestData *, this ) );
		spClient->SetFrameReceiver( DELEGATE( TestWebClient, OnFrameReceived, IWebSocket::FrameSP, this ) );
		Test( spClient->Send() );

		Spin( m_bWSTested );
		Test( m_bWSTested );
		spClient->Close();			// make sure we are closed
		Spin( m_bWSClosed );
		Test( m_bWSClosed );

		spClient->Request( "http://www.google.com",
			IWebClient::Headers(), "GET", "", 
			DELEGATE( TestWebClient, OnResponse, IWebClient::RequestData *, this),
			DELEGATE( TestWebClient, OnState, IWebClient *, this ) );

		Spin( m_bHTTPTested );
		Test( m_bHTTPTested );

		spClient->Request( "https://www.google.com", IWebClient::Headers(), "GET", "", 
			DELEGATE(TestWebClient, OnSecureResponse, IWebClient::RequestData *, this),
			DELEGATE(TestWebClient, OnState, IWebClient *, this));

		Spin( m_bHTTPSTested );
		Test(m_bHTTPSTested);
	}
	void OnWebSocketResponse( IWebClient::RequestData * a_pResonse )
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
		if ( a_pConnector->GetState() == IWebClient::CONNECTED )
			m_bWSTested = true;
		else if ( a_pConnector->GetState() == IWebClient::CLOSED || a_pConnector->GetState() == IWebClient::DISCONNECTED )
			m_bWSClosed = true;
	}

	void OnResponse( IWebClient::RequestData * a_pResponse )
	{
		Log::Debug( "TestWebClient", "OnResponse(): Version: %s, Status: %u, Content: %s",
			a_pResponse->m_Version.c_str(), a_pResponse->m_StatusCode, a_pResponse->m_Content.c_str() );

		if ( a_pResponse->m_StatusCode == 200 )
			m_bHTTPTested = true;
	}
	void OnSecureResponse( IWebClient::RequestData * a_pResponse )
	{
		Log::Debug("TestWebClient", "OnSecureResponse(): Version: %s, Status: %u, Content: %s",
			a_pResponse->m_Version.c_str(), a_pResponse->m_StatusCode, a_pResponse->m_Content.c_str());

		if (a_pResponse->m_StatusCode == 200)
			m_bHTTPSTested = true;
	}
};

TestWebClient TEST_CONNECTOR;
