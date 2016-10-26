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

//! Define to 1 to enable lots of debugging output
#define ENABLE_DEBUGGING			0
#define ENABLE_KEEP_ALIVE			0

#include "WebClient.h"
#include "WebSocketFramer.h"

#include "boost/thread/thread.hpp"
#include "boost/algorithm/string.hpp"
#include "utf8_v2_3_4/source/utf8.h"

#include "Log.h"
#include "ThreadPool.h"
#include "WatsonException.h"

#include <string>

RTTI_IMPL( IWebClient, IWebSocket );
RTTI_IMPL( WebClient, IWebClient );


// include the OpenSSL libs
#ifdef _WIN32
#ifdef _DEBUG
#pragma comment( lib, "libeay32MTd.lib" )
#pragma comment( lib, "ssleay32MTd.lib" )
#else
#pragma comment( lib, "libeay32MT.lib" )
#pragma comment( lib, "ssleay32MTd.lib" )
#endif
#endif

boost::atomic<unsigned int>		IWebClient::sm_RequestsSent;
boost::atomic<unsigned int>		IWebClient::sm_BytesSent;
boost::atomic<unsigned int>		IWebClient::sm_BytesRecv;

WebClientService * WebClientService::sm_pInstance = NULL;

IWebClient * IWebClient::Create()
{
	return new WebClient();
}

std::string WebClient::sm_ClientId;

//----------------------------------------------

WebClient::WebClient() :
	m_eState(CLOSED),
	m_pSocket(NULL),
	m_pStream(NULL),
	m_pSSL(NULL),
	m_WebSocket(false),
	m_RequestType("GET"),
	m_ContentLen( 0xffffffff ), 
	m_bChunked( false ),
	m_SendError( false ),
	m_SendCount( 0 )
{}

WebClient::~WebClient()
{
	// the user should wait until the state becomes closed or disconnected before trying to destroy, this object.
	// IF you hit this assert, then you need to make sure to keep the WebClient object around until it's in the correct state
	// because it's handling asynchronous callbacks.
	if ( m_eState != CLOSED && m_eState != DISCONNECTED )
		Log::Error( "WebClient", "WebClient in bad state when destroyed." );

	assert( m_pSocket == NULL );
	assert( m_pStream == NULL );
	assert( m_pSSL == NULL );
}

void WebClient::SetFrameReceiver( Delegate<FrameSP> a_Receiver )
{
	m_OnFrame = a_Receiver;
}

void WebClient::SetErrorHandler( Delegate<IWebSocket *> a_Handler)
{
	m_OnError = a_Handler;
}

void WebClient::SendBinary(const std::string & a_BinaryData)
{
	if (m_eState != CONNECTED && m_eState != CONNECTING)
		Log::Error( "WebClient", "SendBinary() called with WebClient in wrong state." );
	else if (! m_WebSocket )
		Log::Error( "WebClient", "SendBinary() invoked for non-WebSocket." );
	else
		WS_Send(BINARY_FRAME, a_BinaryData );
}

void WebClient::SendText(const std::string & a_TextData)
{
	if ( utf8::find_invalid( a_TextData.begin(), a_TextData.end()) != a_TextData.end() )
		Log::Error( "WebClient", "Invalid characters string passed to SendText()." );
	else if ( m_eState != CONNECTED && m_eState != CONNECTING )
		Log::Error( "WebClient", "SendText() called with WebClient in wrong state." );
	else if (! m_WebSocket )
		Log::Error( "WebClient", "SendText() invoked for non-WebSocket." );
	else
		WS_Send(TEXT_FRAME, a_TextData );
}

void WebClient::SendPing(const std::string & a_PingData)
{
	if (m_eState != CONNECTED && m_eState != CONNECTING)
		Log::Error( "WebClient", "SendBinary() called with WebClient in wrong state." );
	else if (! m_WebSocket )
		Log::Error( "WebClient", "SendBinary() invoked for non-WebSocket." );
	else
		WS_Send(PING, a_PingData );
}

void WebClient::SendPong(const std::string & a_PingData)
{
	if (m_eState != CONNECTED && m_eState != CONNECTING)
		Log::Error( "WebClient", "SendBinary() called with WebClient in wrong state." );
	else if (! m_WebSocket )
		Log::Error( "WebClient", "SendBinary() invoked for non-WebSocket." );
	else
		WS_Send(PONG, a_PingData );
}

void WebClient::SendClose(const std::string & a_Reason)
{
	if (m_eState != CONNECTED && m_eState != CONNECTING)
		Log::Error( "WebClient", "SendBinary() called with WebClient in wrong state." );
	else if (! m_WebSocket )
		Log::Error( "WebClient", "SendBinary() invoked for non-WebSocket." );
	else
		WS_Send(CLOSE, a_Reason );
}

bool WebClient::Send() 
{
	WebClientService * pService = WebClientService::Instance();
	if ( pService == NULL )
		return false;		// this would only happen if we are in the middle of shutting down..

	// close this socket if previously open..
	if ( m_eState != CLOSED && m_eState != DISCONNECTED )
	{
		Log::Error( "WebClient", "Send() called with Connector in bad state." );
		return false;
	}

	assert( m_pStream == NULL );
	assert( m_pSocket == NULL );

	m_SendError = false;
	m_Pending.clear();
	m_Send.clear();
	sm_RequestsSent++;

	// make the socket..
	if ( _stricmp( m_URL.GetProtocol().c_str(), "https" ) == 0 || 
		_stricmp( m_URL.GetProtocol().c_str(), "wss" ) == 0 )
	{
		m_pSSL = new boost::asio::ssl::context( pService->GetService(), boost::asio::ssl::context::sslv23 );
		m_pSSL->set_verify_mode(boost::asio::ssl::context::verify_none);
		m_pStream = new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>( pService->GetService(), *m_pSSL );
		m_pSocket = &m_pStream->next_layer();
	}
	else if ( _stricmp( m_URL.GetProtocol().c_str(), "http" ) == 0 ||
		_stricmp( m_URL.GetProtocol().c_str(), "ws" ) == 0 )
	{
		m_pSocket = new boost::asio::ip::tcp::socket( pService->GetService() );
	}

	if ( m_pSocket == NULL )
	{
		Log::Error( "WebClient", "Unsupported protocol %s.", m_URL.GetProtocol().c_str() );
		return false;
	}

	m_WebSocket = _stricmp( m_URL.GetProtocol().c_str(), "ws" ) == 0 
		|| _stricmp( m_URL.GetProtocol().c_str(), "wss" ) == 0;

	Log::DebugMed("WebClient", "Connecting to %s:%u", m_URL.GetHost().c_str(), m_URL.GetPort());
	SetState(CONNECTING);

	WebClientService::Instance()->GetService().post( 
		boost::bind( &WebClient::BeginConnect, this ) );

	return true;
}

bool WebClient::Close()
{
	if ( m_pSocket == NULL )
		return false;

	if ( m_eState == IWebClient::CLOSING
		|| m_eState == IWebClient::CLOSED
		|| m_eState == IWebClient::DISCONNECTED )
		return false;

	// just set the state and close the socket, the routines in the other thread
	// will invoke OnDisconnected() which will ignore the state change.
	SetState( CLOSING );
	Log::DebugLow( "WebClient", "Closing socket. (%p)", this );
	m_pSocket->close();
	return true;
}

bool WebClient::Shutdown()
{
	Close();

	if ( ThreadPool::Instance() != NULL )
	{
		while( GetState() == IWebClient::CLOSING )
		{
			boost::this_thread::sleep( boost::posix_time::milliseconds(1) );
			ThreadPool::Instance()->ProcessMainThread();
		}
	}

	return true;
}

bool WebClient::Request(const URL & a_URL,
	const Headers & a_Headers,
	const std::string & a_RequestType,
	const std::string & a_Body,
	Delegate<RequestData *> a_DataReceiver,
	Delegate<IWebClient *> a_StateReceiver )
{
	SetURL( a_URL );
	SetHeaders( a_Headers );
	SetRequestType( a_RequestType );
	SetBody( a_Body );
	SetDataReceiver( a_DataReceiver );
	SetStateReceiver( a_StateReceiver );

	return Send();
}


void WebClient::SetState(SocketState a_eState)
{
	m_eState = a_eState;
	if ( m_StateReceiver.IsValid() )
		m_StateReceiver( this );
}

void WebClient::BeginConnect()
{
	WebClientService * pService = WebClientService::Instance();

	// resolve DNS first before we bother making the socket/stream objects..
	boost::asio::ip::tcp::resolver::iterator i = boost::asio::ip::tcp::resolver::iterator();
	try {
		if ( pService != NULL )
		{
			boost::asio::ip::tcp::resolver resolver(pService->GetService());
			boost::asio::ip::tcp::resolver::query q(m_URL.GetHost(), StringUtil::Format("%u", m_URL.GetPort()));
			i = resolver.resolve(q);
		}
	}
	catch (const std::exception & ex)
	{
		Log::Error("WebClient", "Caught exception: %s", ex.what());
	}

	if (i == boost::asio::ip::tcp::resolver::iterator())
	{
		Log::Error("WebClient", "Failed to resolve %s", m_URL.GetHost().c_str());
		ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( WebClient, OnDisconnected, this ) );
	}
	else
	{
		m_pSocket->async_connect(*i,
			boost::bind(&WebClient::HandleConnect, this, boost::asio::placeholders::error, i));
	}
}

void WebClient::HandleConnect(const boost::system::error_code & error,
	boost::asio::ip::tcp::resolver::iterator i)
{
	if (! error )
	{
		if ( m_pStream != NULL )
		{
			m_pStream->async_handshake( boost::asio::ssl::stream_base::client, 
				boost::bind( &WebClient::HandleHandShake, this, boost::asio::placeholders::error ) );
		}
		else
		{
			// no handshake needed for non-secure connections, go ahead and send the request
			ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( WebClient, OnConnected, this ) );
		}

#if ENABLE_KEEP_ALIVE
		try {
			// this helps recognizing disconnected sockets..
			boost::asio::socket_base::keep_alive option(true);
			m_pSocket->set_option(option);
		}
		catch( const std::exception & ex )
		{
			Log::Error("WebClient", "Caught exception: %s", ex.what());
		}
#endif
	}
	else if ( ++i != boost::asio::ip::tcp::resolver::iterator() )
	{
		// try the next end-point in DNS..
		try {
			m_pSocket->close();
			m_pSocket->async_connect( *i,
				boost::bind(&WebClient::HandleConnect, this, boost::asio::placeholders::error, i));
		}
		catch( const std::exception & ex )
		{
			Log::Error("WebClient", "Caught exception: %s", ex.what());
			ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClient, OnDisconnected, this));
		}
	}
	else
	{
		// set our state to disconnected..
		Log::Error("WebClient", "Failed to connect to %s:%d: %s", 
			m_URL.GetHost().c_str(), m_URL.GetPort(), error.message().c_str() );
		ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClient, OnDisconnected, this));
	}
}

void WebClient::HandleHandShake(const boost::system::error_code & error)
{
	if (! error )
	{
		ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( WebClient, OnConnected, this ) );
	}
	else
	{
		Log::Error( "WebClient", "Handshake Failed with %s:%d: %s", 
			m_URL.GetHost().c_str(), m_URL.GetPort(), error.message().c_str() );
		ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClient, OnDisconnected, this));
	}
}

//! Invoked on main thread.
void WebClient::OnConnected()
{
	if ( m_eState == CONNECTING )
	{
		SetState( CONNECTED );

		RequestData * pResponse = new RequestData();

		std::string & req = m_Request;
		if ( !m_WebSocket )
		{
			if ( m_Headers.find( "Accept" ) == m_Headers.end() )
				m_Headers["Accept"] = "*/*";
			if ( m_Headers.find( "Host" ) == m_Headers.end() )
				m_Headers["Host"] = m_URL.GetHost();
			m_Headers["User-Agent"] = "SelfWebClient";
			m_Headers["Connection"] = "close";
			if ( sm_ClientId.size() > 0 )
				m_Headers["ClientId"] = sm_ClientId;

			req = m_RequestType + " /" + m_URL.GetEndPoint() + " HTTP/1.1\r\n";
			for( Headers::iterator iHeader = m_Headers.begin(); iHeader != m_Headers.end(); ++iHeader )
				req += iHeader->first + ": " + iHeader->second + "\r\n";
			if ( m_RequestType == "POST" || m_RequestType == "PUT" )
				req += StringUtil::Format( "Content-Length: %u", m_Body.size() );
			req += "\r\n\r\n";
			if ( m_RequestType == "POST" || m_RequestType == "PUT" )
				req += m_Body;
		}
		else
		{
			if (m_Headers.find("Host") == m_Headers.end())
				m_Headers["Host"] = m_URL.GetHost();

			m_Headers["Upgrade"] = "websocket";
			m_Headers["Connection"] = "Upgrade";
			m_Headers["Sec-WebSocket-Key"] = "x3JJHMbDL1EzLkh9GBhXDw==";
			m_Headers["Sec-WebSocket-Version"] = "13";
			m_Headers["User-Agent"] = "SelfWebClient";
			if ( sm_ClientId.size() > 0 )
				m_Headers["ClientId"] = sm_ClientId;
			//m_Headers["Sec-WebSocket-Protocol"] = "chat";

			req = m_RequestType + " /" + m_URL.GetEndPoint() + " HTTP/1.1\r\n";
			for (Headers::iterator iHeader = m_Headers.begin(); iHeader != m_Headers.end(); ++iHeader)
				req += iHeader->first + ": " + iHeader->second + "\r\n";
			req += "\r\n";
		}

		if ( req.size() == 0 )
		{
			delete pResponse;

			Log::Error( "WebClient", "Request is empty, closing connection." );
			ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClient, OnClose, this));
		}
		else if (m_pStream != NULL)
		{
			boost::asio::async_write(*m_pStream,
				boost::asio::buffer(req.c_str(), req.length()),
				boost::bind(&WebClient::HTTP_RequestSent, this, pResponse,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else if (m_pSocket != NULL)
		{
			boost::asio::async_write(*m_pSocket,
				boost::asio::buffer(req.c_str(), req.length()),
				boost::bind(&WebClient::HTTP_RequestSent, this, pResponse,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		sm_BytesSent += req.length();
	}
	else
	{
		Log::Debug( "WebClient", "State is not CONNECTING");
		if ( m_eState == CLOSING )
			ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClient, OnClose, this));
		else
			ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClient, OnDisconnected, this));
	}
}

void WebClient::HTTP_RequestSent( RequestData * a_pReq, const boost::system::error_code& error,
	size_t bytes_transferred)
{
	if (!error) 
	{
		// read the first line containing the status..
		if ( m_pStream != NULL )
		{
			boost::asio::async_read_until(*m_pStream,
				m_Response, "\r\n\r\n",
				boost::bind(&WebClient::HTTP_ReadHeaders, this, a_pReq,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else if ( m_pSocket != NULL )
		{
			boost::asio::async_read_until(*m_pSocket,
				m_Response, "\r\n\r\n",
				boost::bind(&WebClient::HTTP_ReadHeaders, this, a_pReq,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
	}
	else 
	{
		delete a_pReq;

		Log::Error( "WebClient", "Error on RequestSent(): %s", error.message().c_str() );
		ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClient, OnDisconnected, this));
	}
}

void WebClient::HTTP_ReadHeaders( RequestData * a_pReq, const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error) 
	{
		sm_BytesRecv += bytes_transferred;
		std::istream input( &m_Response );
		input >> a_pReq->m_Version;
		input >> a_pReq->m_StatusCode;
		std::getline(input, a_pReq->m_StatusMessage);

		std::string header;
		while( std::getline( input, header) && header != "\r" )
		{
			size_t seperator = header.find_first_of( ':' );
			if (seperator == std::string::npos)
				continue;
			std::string key = StringUtil::Trim(header.substr(0, seperator), " \r\n");
			std::string value = StringUtil::Trim(header.substr(seperator + 1), " \r\n");
			a_pReq->m_Headers[key] = value;
		}

		// if this is a web socket then we follow a different path at this point..
		if ( m_WebSocket )
		{
			m_Incoming.clear();

			if ( m_DataReceiver.IsValid() )
				m_DataReceiver( a_pReq );

			// TODO: Should check the Sec-WebSocket-Accept hash using SHA1
			Headers::iterator iWebSocket = a_pReq->m_Headers.find( "Upgrade" );
			if ( a_pReq->m_StatusCode == 101 && 
				iWebSocket != a_pReq->m_Headers.end() && _stricmp( iWebSocket->second.c_str(), "WebSocket" ) == 0 )
			{
				// send all pending packets now..
				if ( m_Pending.begin() != m_Pending.end() )
				{
					Log::Debug( "WebClient", "Sending %u pending frames.", m_Pending.size() );
					for (BufferList::iterator iSend = m_Pending.begin(); iSend != m_Pending.end(); ++iSend)
						WS_QueueSend(*iSend);
					m_Pending.clear();
				}

				// start reading WebSocket frames..
				WS_Read( a_pReq, error, 0 );
			}
			else
			{
				Log::Error( "WebClient", "Websocket failed to connect, status code %u: %s", 
					a_pReq->m_StatusCode, a_pReq->m_StatusMessage.c_str() );
				m_SendError = true;
				if ( m_SendCount == 0 )
					ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClient, OnDisconnected, this));
				delete a_pReq;
			}
		}
		else
		{
			Headers::iterator iTransferEnc = a_pReq->m_Headers.find("Transfer-Encoding");
			if ( iTransferEnc != a_pReq->m_Headers.end() )
				m_bChunked = _stricmp( iTransferEnc->second.c_str(), "chunked" ) == 0;
			else
				m_bChunked = false;

			if (! m_bChunked )		// if we are chunked, then Content-Length is ignored.
			{
				Headers::iterator iContentLen = a_pReq->m_Headers.find( "Content-Length" );
				if ( iContentLen != a_pReq->m_Headers.end() )
				{
					m_ContentLen = strtoul( iContentLen->second.c_str(), NULL, 10 );
					a_pReq->m_Content.reserve( m_ContentLen );		// speed up the load by reserving the space we know we'll need.
				}
			}

			HTTP_ReadContent( a_pReq, error, 0 );
		}
	}
	else 
	{
		Log::Error( "WebClient", "Error on ReadResponseHeaders(): %s", error.message().c_str() );
		ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClient, OnDisconnected, this));
		delete a_pReq;
	}
}

void WebClient::HTTP_ReadContent( RequestData * a_pReq, const boost::system::error_code& error, size_t bytes_transferred)
{
	sm_BytesRecv += bytes_transferred;
	size_t max_read = (size_t)m_Response.in_avail();
	size_t bytes_remaining = m_ContentLen - a_pReq->m_Content.size();
	if ( max_read > bytes_remaining )
		max_read = bytes_remaining;

	std::istream input(&m_Response);
	if ( max_read > 0 )
	{
		std::string content;
		content.resize( max_read );
		input.read( &content[0], max_read );

		a_pReq->m_Content += content;
	}

	if (!error && bytes_remaining > 0) 
	{
		// if we are not chunked and we have no content len, then go ahead and start piping 
		// data to the user 4k at a time.
		if (!m_bChunked && m_ContentLen == 0xffffffff 
			&& a_pReq->m_Content.size() >= (4 * 1024))
		{
			RequestData * pNewReq = new RequestData();
			pNewReq->m_Version = a_pReq->m_Version;
			pNewReq->m_StatusCode = a_pReq->m_StatusCode;
			pNewReq->m_StatusMessage = a_pReq->m_StatusMessage;
			pNewReq->m_Headers = a_pReq->m_Headers;

			ThreadPool::Instance()->InvokeOnMain<RequestData *>(
				DELEGATE(WebClient, OnResponse, RequestData *, this), a_pReq);
			a_pReq = pNewReq;
		}

		if (m_pStream != NULL)
		{
			boost::asio::async_read(*m_pStream, m_Response,
				boost::asio::transfer_at_least(1),
				boost::bind(&WebClient::HTTP_ReadContent, this, a_pReq,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else if (m_pSocket != NULL)
		{
			boost::asio::async_read(*m_pSocket, m_Response,
				boost::asio::transfer_at_least(1),
				boost::bind(&WebClient::HTTP_ReadContent, this, a_pReq,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
	}
	else 
	{
		if ( m_bChunked )
		{
			std::string & content = a_pReq->m_Content;

			size_t offset = 0;
			while( offset < content.size() )
			{
				size_t eof = content.find_first_of( "\r\n", offset );
				std::string length = content.substr( offset, eof - offset );
				// remove the chunk line from the content..
				content.erase( content.begin() + offset, content.begin() + eof + 2 );
				size_t chunk_len = strtoul( length.c_str(), NULL, 16 );
				offset += chunk_len;

				if ( length == "0" )
				{
					// TODO: parse trailing headers, for now just strip out the rest..
					if ( offset < content.size() )
						content.erase( content.begin() + offset, content.end() );
					break;
				}
				
				// after each chunk should be a \r\n, remove it..
				if ( offset < content.size() )
					content.erase( content.begin() + offset, content.begin() + offset + 2 );
			}
		}

		a_pReq->m_bDone = true;
		ThreadPool::Instance()->InvokeOnMain<RequestData *>(
			DELEGATE(WebClient, OnResponse, RequestData *, this), a_pReq);
	}
}

void WebClient::WS_Read( RequestData * a_pReq, const boost::system::error_code & error,
	size_t bytes_transferred)
{
	sm_BytesRecv += bytes_transferred;
	if ( m_WebSocket )
	{
		// append received data onto the m_Incoming buffer..
		if ( m_Response.in_avail() > 0 )
		{
			std::istream input( &m_Response );

			std::string received;
			received.resize( (size_t)m_Response.in_avail() );
			input.read( &received[0], received.size() );

			m_Incoming += received;
		}

		// a web socket frame is at least 2 bytes in size..
		IWebSocket::Frame * pFrame = NULL;
		while( m_WebSocket && (pFrame = WebSocketFramer::ParseFrame( m_Incoming )) != NULL )
		{
			pFrame->m_pSocket = this;

			bool bClose = pFrame->m_Op == CLOSE;
			ThreadPool::Instance()->InvokeOnMain<IWebSocket::Frame *>(
				DELEGATE( WebClient, OnWebSocketFrame, IWebSocket::Frame *, this ), pFrame );

			if ( bClose )
			{
				Log::DebugLow( "WebClient", "Received close op: %s (%p)", pFrame->m_Data.c_str(), this );
				ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( WebClient, OnClose, this ) );
				m_WebSocket = false;
				delete a_pReq;
			}
		}

		if (m_WebSocket)
		{
			if (!error)
			{
				// continue reading from the web socket..
				if (m_pStream != NULL)
				{
					boost::asio::async_read(*m_pStream, m_Response,
						boost::asio::transfer_at_least(1),
						boost::bind(&WebClient::WS_Read, this, a_pReq,
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred));
				}
				else if (m_pSocket != NULL)
				{
					boost::asio::async_read(*m_pSocket, m_Response,
						boost::asio::transfer_at_least(1),
						boost::bind(&WebClient::WS_Read, this, a_pReq,
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred));
				}
			}
			else
			{
				Log::DebugLow("WebClient", "Error on WS_Read(): %s (%p)", error.message().c_str(), this );

				m_SendError = true;
				if ( m_SendCount == 0 )
					ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClient, OnDisconnected, this));
				delete a_pReq;
			}
		}
	}
}

static void Append( std::string & a_Buffer, const void * a_pData, size_t a_nBytes )
{
	a_Buffer += std::string( (const char *)a_pData, a_nBytes );
}

void WebClient::WS_Send( OpCode a_Op, const std::string & a_Data, bool a_bUseMask /*= true*/ )
{
	std::string * pPacket = new std::string();
	WebSocketFramer::CreateFrame( *pPacket, a_Op, a_Data, a_bUseMask);

	WS_QueueSend( pPacket );
}

void WebClient::WS_QueueSend(std::string * a_pBuffer)
{
	boost::lock_guard<boost::recursive_mutex> lock( m_SendLock );
	if (!m_SendError)
	{
		if (m_eState == CONNECTED)
		{
			// queue the data up first, then check if we have any active sends, if not then send the data..
			m_Send.push_back(a_pBuffer);
			if (m_SendCount == 0)
				WS_SendNext();
		}
		else
		{
			// stash for later..
			m_Pending.push_back(a_pBuffer);
		}
	}
	else
	{
		// we ignore any sends once m_SendError is true, this lets the m_SendCount get down
		// to 0 so we can notify the main thread we are disconnected.
		Log::Debug("WebClient", "Ignoring send because of error state.");
	}
}

//! This sends the data over the socket no matter what, it doesn't care if the data is overlapping
//! in any way.
void WebClient::WS_SendNext()
{
	boost::lock_guard<boost::recursive_mutex> lock( m_SendLock );
	if (m_Send.begin() != m_Send.end())
	{
		std::string * pFrame = m_Send.front();
		m_Send.pop_front();

#if ENABLE_DEBUGGING
		Log::Debug("WebClient", "Sending %u bytes (%p).", pFrame->size(), pFrame);
#endif

		if (m_pStream != NULL)
		{
			boost::asio::async_write(*m_pStream,
				boost::asio::buffer(pFrame->c_str(), pFrame->size()),
				boost::bind(&WebClient::WS_Sent, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					pFrame));

			// we need to know how many outstanding sends we have..
			m_SendCount += 1;
			sm_BytesSent += pFrame->size();
		}
		else if (m_pSocket != NULL)
		{
			boost::asio::async_write(*m_pSocket,
				boost::asio::buffer(pFrame->c_str(), pFrame->size()),
				boost::bind(&WebClient::WS_Sent, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					pFrame));

			// we need to know how many outstanding sends we have..
			m_SendCount += 1;
			sm_BytesSent += pFrame->size();
		}
	}
}

void WebClient::WS_Sent( const boost::system::error_code& error, size_t bytes_transferred, std::string * pBuffer )
{
	boost::lock_guard<boost::recursive_mutex> lock( m_SendLock );

	m_SendCount -= 1;
	if ( error || m_SendError )
	{
		if (!m_SendError )
		{
			Log::Error( "WebClient", "Error sending web socket frame : %s", error.message().c_str() );
			m_SendError = true;
		}

		// once we number of outstanding sends is 0, then let the main thread know we've been disconnected.
		if ( m_SendCount == 0 )
			ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE(WebClient, OnDisconnected, this) );
	}
	else
	{
#if ENABLE_DEBUGGING
		Log::Debug( "WebClient", "WS_Sent %u bytes (%p) (%u pending)", bytes_transferred, pBuffer, m_SendCount );
#endif
		// send the next block, this will do nothing if nothing is queued..
		if ( m_SendCount == 0 )
			WS_SendNext();
	}

	delete pBuffer;
}

void WebClient::OnResponse(RequestData * a_pData)
{
	bool bClose = false;
	Headers::iterator iConnection = a_pData->m_Headers.find( "Connection" );
	if ( iConnection != a_pData->m_Headers.end() )
		bClose = _stricmp( iConnection->second.c_str(), "close") == 0;

	if ( m_DataReceiver.IsValid() )
		m_DataReceiver( a_pData );

	// close the socket afterwards, only if 
	if ( bClose && a_pData->m_bDone )
		OnClose();

	delete a_pData;
}

void WebClient::OnWebSocketFrame( IWebSocket::Frame * a_pFrame )
{
	FrameSP spFrame( a_pFrame );
	if ( m_OnFrame.IsValid() )
		m_OnFrame( spFrame );
}

void WebClient::OnClose()
{
	if ( m_eState == CONNECTED || 
		m_eState == CONNECTING || 
		m_eState == CLOSING )
	{
		Log::DebugLow( "WebClient", "OnClose() closing socket. (%p)", this );
		m_pSocket->close();

		if ( m_pStream != NULL )
		{
			delete m_pStream;
			delete m_pSSL;
			m_pStream = NULL;
			m_pSocket = NULL;
			m_pSSL = NULL;
		}
		if ( m_pSocket != NULL )
		{
			delete m_pSocket;
			m_pSocket = NULL;
		}

		m_SendError = false;
		SetState( CLOSED );
	}
}

void WebClient::OnDisconnected()
{
	if ( m_eState == CONNECTED || 
		m_eState == CONNECTING || 
		m_eState == CLOSING )
	{
		assert( m_SendCount == 0 );

		Log::DebugLow( "WebClient", "OnDisconnected() closing socket. (%p)", this );
		m_pSocket->close();

		if (m_pStream != NULL)
		{
			delete m_pStream;
			delete m_pSSL;
			m_pStream = NULL;
			m_pSocket = NULL;
			m_pSSL = NULL;
		}
		if (m_pSocket != NULL)
		{
			delete m_pSocket;
			m_pSocket = NULL;
		}

		m_SendError = false;

		// if Close() is called, then we set the state to close and just close the socket. The async
		// routines will think it's been disconnected and they will invoke OnDisconnected(), ignore
		// changing the state to disconnected when it was a client-side initiated close.
		if (m_eState != CLOSING)
			SetState(DISCONNECTED);
		else
			SetState(CLOSED);
	}
}


void WebClient::SetClientId( const std::string & a_ClientId )
{
	sm_ClientId = a_ClientId;
}

const std::string & WebClient::GetClientId()
{
	return sm_ClientId;
}

//----------------------------------

//! This internal singleton class runs the io service for all Socket objects.
WebClientService * WebClientService::Instance()
{
	if (sm_pInstance == NULL)
	{
		// use a mutex to prevent a race condition with multiple threads
		// trying to make the instance at once.
		static boost::mutex lock;

		lock.lock();
		if (sm_pInstance == NULL)
			sm_pInstance = new WebClientService();
		lock.unlock();
	}

	return sm_pInstance;
}

WebClientService::WebClientService() :
	m_Work(m_Service),											// this prevents the IO service from stopping on it's own
	m_ServiceThread(boost::bind(&boost::asio::io_service::run, &m_Service))	// start a thread to run our io service
{
	if (TimerPool::Instance() != NULL)
		m_spStatsTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(WebClientService, OnDumpStats, this), 60.0, true, true);
}

WebClientService::~WebClientService()
{
	if (sm_pInstance == this)
		sm_pInstance = NULL;
	m_spStatsTimer.reset();
	m_Service.stop();
	m_ServiceThread.join();
}

void WebClientService::OnDumpStats()
{
	Log::Status("WebClient", "STAT: Requests: %u, Bytes Sent: %u, Bytes Recv: %u",
		IWebClient::sm_RequestsSent.load(),
		IWebClient::sm_BytesSent.load(),
		IWebClient::sm_BytesRecv.load());
}

