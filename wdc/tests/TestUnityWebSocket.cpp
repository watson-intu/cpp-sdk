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

class TestUnityWebSocket : UnitTest
{
public:
	//! Construction
	TestUnityWebSocket() : UnitTest("TestUnityWebSocket"), m_bStopServer(false)
	{}

	virtual void RunTest()
	{
		Time start;

		ThreadPool pool(1);

		WebServer server;
		server.AddEndpoint("/test_ws", DELEGATE(TestUnityWebSocket, OnTestWS, WebServer::RequestSP, this));
		Test(server.Start());

		Spin( m_bStopServer, 300.0f );
	}

	void OnTestWS(WebServer::RequestSP a_spRequest)
	{
		Log::Debug("TestUnityWebSocket", "OnTestWS()");
		Test(a_spRequest.get() != NULL);
		Test(a_spRequest->m_RequestType == "GET");

		WebServer::Headers::iterator iWebSocketKey = a_spRequest->m_Headers.find("Sec-WebSocket-Key");
		Test(iWebSocketKey != a_spRequest->m_Headers.end());

		a_spRequest->m_spConnection->SetFrameReceiver( DELEGATE(TestUnityWebSocket, OnServerFrame, IWebSocket::FrameSP, this ) );
		a_spRequest->m_spConnection->SetErrorHandler( VOID_DELEGATE( TestUnityWebSocket, OnError, this) );
		a_spRequest->m_spConnection->StartWebSocket(iWebSocketKey->second );
		Test(a_spRequest->m_spConnection->IsWebSocket());

		//a_spRequest->m_spConnection->SendAsync("HTTP/1.1 200 Hello World\r\nConnection: close\r\n\r\n");
	}

	void OnWebSocketResponse(WebClient::RequestData * a_pResonse)
	{
		Log::Debug("TestUnityWebSocket", "WebSocket response, status code %u : %s", 
			a_pResonse->m_StatusCode, a_pResonse->m_StatusMessage.c_str());
	}

	void OnServerFrame(IWebSocket::FrameSP a_spFrame )
	{
		Log::Debug("TestUnityWebSocket", "OnServerFrame() OpCode: %d, Data: %s", a_spFrame->m_Op, 
			a_spFrame->m_Op == IWebSocket::TEXT_FRAME ? a_spFrame->m_Data.c_str() : StringUtil::Format( "%u bytes", a_spFrame->m_Data.size()).c_str() );
		if (a_spFrame->m_Op == IWebSocket::BINARY_FRAME)
			a_spFrame->m_pSocket->SendBinary(a_spFrame->m_Data);
		else if (a_spFrame->m_Op == IWebSocket::TEXT_FRAME)
			a_spFrame->m_pSocket->SendText(a_spFrame->m_Data);
		else if ( a_spFrame->m_Op == IWebSocket::CLOSE )
			m_bStopServer = true;
	}

	void OnError()
	{
		Log::Debug("TestUnityWebSocket", "OnError()");
		m_bStopServer = true;
	}

	bool m_bStopServer;
};

TestUnityWebSocket TEST_UNITY_WEB_SOCKET;
