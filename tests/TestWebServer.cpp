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
#include "utils/WebServer.h"
#include "utils/Log.h"
#include "utils/ThreadPool.h"
#include "utils/Time.h"

class TestWebServer : UnitTest
{
public:
	//! Construction
	TestWebServer() : UnitTest("TestWebServer"), 
		m_bHTTPTested(false), 
		m_bHTTPSTested(false), 
		m_bWSTested(false),
		m_bWSSTested(false),
		m_bClientClosed( false )
	{}

	virtual void RunTest()
	{
		Time start;

		ThreadPool pool(1);

		WebServer server;
		server.AddEndpoint("/test_http", DELEGATE(TestWebServer, OnTestHTTP, WebServer::RequestSP, this));
		server.AddEndpoint("/test_ws", DELEGATE(TestWebServer, OnTestWS, WebServer::RequestSP, this));
		Test(server.Start());

		// test web requests
		WebClient client;
		client.Request("http://127.0.0.1/test_http", WebClient::Headers(), "GET", "",
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
		client.SetURL("ws://127.0.0.1/test_ws");
		client.SetStateReceiver(DELEGATE(TestWebServer, OnState, IWebClient *, this));
		client.SetDataReceiver(DELEGATE(TestWebServer, OnWebSocketResponse, WebClient::RequestData *, this));
		client.SetFrameReceiver(DELEGATE(TestWebServer, OnClientFrame, IWebSocket::FrameSP, this));
		Test(client.Send());

		// test web sockets
		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0)
		{
			client.SendText("Testing text");

			std::string sData;
			sData.resize( rand() * 4 );
			for(size_t i=0;i<sData.size();++i)
				sData[i] = (char)(rand() % 255);

			client.SendBinary(sData);

			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(0));
		}
		Test(client.Close());
		Test(m_bWSTested );

		while (!m_bClientClosed)
		{
			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		}

		SecureWebServer secure_server("./etc/tests/server.crt", 
			"./etc/tests/server.key");
		secure_server.AddEndpoint("/test_https", DELEGATE(TestWebServer, OnTestHTTPS, SecureWebServer::RequestSP, this));
		secure_server.AddEndpoint("/test_wss", DELEGATE(TestWebServer, OnTestWSS, SecureWebServer::RequestSP, this));
		Test(secure_server.Start());

		m_bClientClosed = false;
		client.Request("https://127.0.0.1/test_https", WebClient::Headers(), "GET", "",
			DELEGATE(TestWebServer, OnSecureResponse, WebClient::RequestData *, this),
			DELEGATE(TestWebServer, OnState, IWebClient *, this));

		start = Time();
		while (!m_bClientClosed && (Time().GetEpochTime() - start.GetEpochTime()) < 15.0)
		{
			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		}
		Test(m_bHTTPSTested);

		m_bClientClosed = false;
		client.SetURL("wss://127.0.0.1/test_wss");
		client.SetStateReceiver(DELEGATE(TestWebServer, OnState, IWebClient *, this));
		client.SetDataReceiver(DELEGATE(TestWebServer, OnWebSocketResponse, WebClient::RequestData *, this));
		client.SetFrameReceiver(DELEGATE(TestWebServer, OnSecureClientFrame, IWebSocket::FrameSP, this));
		Test(client.Send());

		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0)
		{
			client.SendText("Testing text");
			client.SendBinary("Testing binary");

			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(0));
		}
		Test(client.Close());
		Test(m_bWSSTested);

		while (!m_bClientClosed)
		{
			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		}
	}

	void OnTestHTTP(WebServer::RequestSP a_spRequest)
	{
		Log::Debug("TestWebServer", "OnTestHTTP()");
		Test(a_spRequest.get() != NULL );
		Test(a_spRequest->m_RequestType == "GET");

		a_spRequest->m_spConnection->SendAsync("HTTP/1.1 200 Hello World\r\nConnection: close\r\n\r\n");
	}

	void OnTestWS(WebServer::RequestSP a_spRequest)
	{
		Log::Debug("TestWebServer", "OnTestWS()");
		Test(a_spRequest.get() != NULL);
		Test(a_spRequest->m_RequestType == "GET");

		WebServer::Headers::iterator iWebSocketKey = a_spRequest->m_Headers.find("Sec-WebSocket-Key");
		Test(iWebSocketKey != a_spRequest->m_Headers.end());

		a_spRequest->m_spConnection->SetFrameReceiver( DELEGATE(TestWebServer, OnServerFrame, IWebSocket::FrameSP, this ) );
		a_spRequest->m_spConnection->StartWebSocket(iWebSocketKey->second );
		Test(a_spRequest->m_spConnection->IsWebSocket());

		//a_spRequest->m_spConnection->SendAsync("HTTP/1.1 200 Hello World\r\nConnection: close\r\n\r\n");
	}

	void OnTestHTTPS(SecureWebServer::RequestSP a_spRequest)
	{
		Log::Debug("TestWebServer", "OnTestHTTPS()");
		Test(a_spRequest.get() != NULL);
		Test(a_spRequest->m_RequestType == "GET");

		a_spRequest->m_spConnection->SendAsync("HTTP/1.1 200 Hello World\r\nConnection: close\r\n\r\n");
	}

	void OnTestWSS(SecureWebServer::RequestSP a_spRequest)
	{
		Log::Debug("TestWebServer", "OnTestWSS()");
		Test(a_spRequest.get() != NULL);
		Test(a_spRequest->m_RequestType == "GET");

		WebServer::Headers::iterator iWebSocketKey = a_spRequest->m_Headers.find("Sec-WebSocket-Key");
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
		Log::Debug("TestWebServer", "OnServerFrame() OpCode: %d, Data: %s", a_spFrame->m_Op, 
			a_spFrame->m_Op == IWebSocket::TEXT_FRAME ? a_spFrame->m_Data.c_str() : StringUtil::Format( "%u bytes", a_spFrame->m_Data.size()).c_str() );
		if (a_spFrame->m_Op == IWebSocket::BINARY_FRAME)
			a_spFrame->m_pSocket->SendBinary(a_spFrame->m_Data);
		else if (a_spFrame->m_Op == IWebSocket::TEXT_FRAME)
			a_spFrame->m_pSocket->SendText(a_spFrame->m_Data);
	}

	void OnClientFrame( IWebSocket::FrameSP a_spFrame )
	{
		Log::Debug("TestWebServer", "OnClientFrame() OpCode: %d, Data: %s", a_spFrame->m_Op, 
			a_spFrame->m_Op == IWebSocket::TEXT_FRAME ? a_spFrame->m_Data.c_str() : StringUtil::Format( "%u bytes", a_spFrame->m_Data.size()).c_str() );
		m_bWSTested = true;
	}
	void OnSecureClientFrame( IWebSocket::FrameSP a_spFrame )
	{
		Log::Debug( "TestWebServer", "OnSecureClientFrame() OpCode: %d, Data: %s", a_spFrame->m_Op, a_spFrame->m_Data.c_str() );
		m_bWSSTested = true;
	}
	void OnError()
	{
		Log::Debug("TestWebServer", "OnError()");
	}

	void OnState(IWebClient * a_pConnector)
	{
		Log::Debug("TestWebServer", "OnState(): %d", a_pConnector->GetState());
		if (a_pConnector->GetState() == WebClient::CLOSED || a_pConnector->GetState() == IWebClient::DISCONNECTED )
			m_bClientClosed = true;
	}

	void OnResponse(WebClient::RequestData * a_pResponse)
	{
		Log::Debug("TestWebServer", "OnResponse(): Version: %s, Status: %u, Content: %s",
			a_pResponse->m_Version.c_str(), a_pResponse->m_StatusCode, a_pResponse->m_Content.c_str());

		Test(a_pResponse->m_StatusCode == 200);
		m_bHTTPTested = true;
	}
	void OnSecureResponse(WebClient::RequestData * a_pResponse)
	{
		Log::Debug("TestWebServer", "OnSecureResponse(): Version: %s, Status: %u, Content: %s",
			a_pResponse->m_Version.c_str(), a_pResponse->m_StatusCode, a_pResponse->m_Content.c_str());

		Test(a_pResponse->m_StatusCode == 200);
		m_bHTTPSTested = true;
	}

	bool m_bHTTPTested;
	bool m_bHTTPSTested;
	bool m_bWSTested;
	bool m_bWSSTested;
	bool m_bClientClosed;
};

TestWebServer TEST_WEB_SERVER;
