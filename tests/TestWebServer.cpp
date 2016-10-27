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
#include "utils/IWebServer.h"
#include "utils/Log.h"
#include "utils/ThreadPool.h"
#include "utils/Time.h"

class TestWebServer : UnitTest
{
public:
	//! Construction
	TestWebServer() : UnitTest("TestWebServer"), 
		m_bHTTPTested(false), 
		m_bWSTested(false),
		m_bClientClosed( false )
	{}

	virtual void RunTest()
	{
		Time start;

		ThreadPool pool(1);

		IWebServer * pServer = IWebServer::Create();
		pServer->AddEndpoint("/test_http", DELEGATE(TestWebServer, OnTestHTTP, IWebServer::RequestSP, this));
		pServer->AddEndpoint("/test_ws", DELEGATE(TestWebServer, OnTestWS, IWebServer::RequestSP, this));
		Test(pServer->Start());

		// test web requests
		IWebClient::SP spClient = IWebClient::Create();
		spClient->Request("http://127.0.0.1/test_http", WebClient::Headers(), "GET", "",
			DELEGATE(TestWebServer, OnResponse, WebClient::RequestData *, this),
			DELEGATE(TestWebServer, OnState, IWebClient *, this));

		start = Time();
		while (!m_bClientClosed && (Time().GetEpochTime() - start.GetEpochTime()) < 15.0)
		{
			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		}
		Test(m_bHTTPTested);

		m_bClientClosed = false;
		spClient->SetURL("ws://127.0.0.1/test_ws");
		spClient->SetStateReceiver(DELEGATE(TestWebServer, OnState, IWebClient *, this));
		spClient->SetDataReceiver(DELEGATE(TestWebServer, OnWebSocketResponse, WebClient::RequestData *, this));
		spClient->SetFrameReceiver(DELEGATE(TestWebServer, OnClientFrame, IWebSocket::FrameSP, this));
		Test(spClient->Send());

		// test web sockets
		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 10.0)
		{
			spClient->SendText("Testing text");

			std::string sData;
			sData.resize( rand() * 4 );
			for(size_t i=0;i<sData.size();++i)
				sData[i] = (char)(rand() % 255);

			spClient->SendBinary(sData);

			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(0));
		}
		Test(spClient->Close());
		Test(m_bWSTested );

		while (!m_bClientClosed)
		{
			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		}

		spClient.reset();
		delete pServer;
	}

	void OnTestHTTP(IWebServer::RequestSP a_spRequest)
	{
		Log::Debug("TestWebServer", "OnTestHTTP()");
		Test(a_spRequest.get() != NULL );
		Test(a_spRequest->m_RequestType == "GET");

		a_spRequest->m_spConnection->SendAsync("HTTP/1.1 200 Hello World\r\nConnection: close\r\n\r\n");
	}

	void OnTestWS(IWebServer::RequestSP a_spRequest)
	{
		Log::Debug("TestWebServer", "OnTestWS()");
		Test(a_spRequest.get() != NULL);
		Test(a_spRequest->m_RequestType == "GET");

		IWebServer::Headers::iterator iWebSocketKey = a_spRequest->m_Headers.find("Sec-WebSocket-Key");
		Test(iWebSocketKey != a_spRequest->m_Headers.end());

		a_spRequest->m_spConnection->SetFrameReceiver( DELEGATE(TestWebServer, OnServerFrame, IWebSocket::FrameSP, this ) );
		a_spRequest->m_spConnection->StartWebSocket(iWebSocketKey->second );
		Test(a_spRequest->m_spConnection->IsWebSocket());

		//a_spRequest->m_spConnection->SendAsync("HTTP/1.1 200 Hello World\r\nConnection: close\r\n\r\n");
	}

	void OnWebSocketResponse(WebClient::RequestData * a_pResonse)
	{
		Log::Debug("TestWebServer", "WebSocket response, status code %u : %s", 
			a_pResonse->m_StatusCode, a_pResonse->m_StatusMessage.c_str());
	}

	void OnServerFrame(IWebSocket::FrameSP a_spFrame )
	{
		IWebSocket::SP spSocket = a_spFrame->m_wpSocket.lock();

		Log::Debug("TestSecureWebServer", "OnServerFrame() OpCode: %d, Data: %s", a_spFrame->m_Op, 
			a_spFrame->m_Op == IWebSocket::TEXT_FRAME ? a_spFrame->m_Data.c_str() : StringUtil::Format( "%u bytes", a_spFrame->m_Data.size()).c_str() );

		if ( spSocket )
		{
			if (a_spFrame->m_Op == IWebSocket::BINARY_FRAME)
				spSocket->SendBinary(a_spFrame->m_Data);
			else if (a_spFrame->m_Op == IWebSocket::TEXT_FRAME)
				spSocket->SendText(a_spFrame->m_Data);
		}
	}

	void OnClientFrame( IWebSocket::FrameSP a_spFrame )
	{
		Log::Debug("TestWebServer", "OnClientFrame() OpCode: %d, Data: %s", a_spFrame->m_Op, 
			a_spFrame->m_Op == IWebSocket::TEXT_FRAME ? a_spFrame->m_Data.c_str() 
				: StringUtil::Format( "%u bytes", a_spFrame->m_Data.size()).c_str() );
		m_bWSTested = true;
	}
	
	void OnError()
	{
		Log::Debug("TestWebServer", "OnError()");
	}

	void OnState(IWebClient * a_pConnector)
	{
		Log::Debug("TestWebServer", "OnState(): %d", a_pConnector->GetState());
		if (a_pConnector->GetState() == WebClient::CLOSED || a_pConnector->GetState() == IWebClient::DISCONNECTED)
			m_bClientClosed = true;
	}

	void OnResponse(WebClient::RequestData * a_pResponse)
	{
		Log::Debug("TestWebServer", "OnResponse(): Version: %s, Status: %u, Content: %s",
			a_pResponse->m_Version.c_str(), a_pResponse->m_StatusCode, a_pResponse->m_Content.c_str());

		Test(a_pResponse->m_StatusCode == 200);
		m_bHTTPTested = true;
	}


	bool m_bHTTPTested;
	bool m_bWSTested;
	bool m_bClientClosed;
};

TestWebServer TEST_WEB_SERVER;
