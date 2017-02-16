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

#define ENABLE_OPENSSL_INCLUDES

//! Define to 1 to enable lots of debugging output
#define ENABLE_DEBUGGING			0
#define ENABLE_KEEP_ALIVE			0

#include "IWebClient.h"
#include "WebSocketFramer.h"

#include "boost/thread/thread.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/asio.hpp"		// not including SSL at this level on purpose
#include "boost/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/asio/ssl.hpp"

#include "utf8_v2_3_4/source/utf8.h"

#include "Log.h"
#include "ThreadPool.h"
#include "WatsonException.h"
#include "WebClientService.h"

#include <string>

#if ENABLE_DELEGATE_DEBUG
#define WARNING_DELEGATE_TIME (0.1)
#define ERROR_DELEGATE_TIME	(0.5)
#endif

RTTI_IMPL( IWebClient, IWebSocket );

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
std::string						IWebClient::sm_ClientId;

IWebClient::ConnectionMap &	IWebClient::GetConnectionMap()
{
	static ConnectionMap * pMAP = new ConnectionMap();
	return *pMAP;
}

Factory<IWebClient> & IWebClient::GetFactory()
{
	static Factory<IWebClient> FACTORY;
	return FACTORY;
}

IWebClient::SP IWebClient::Create( const URL & a_URL )
{
	std::string hashId = StringUtil::Format( "%s://%s:%d", 
		a_URL.GetProtocol().c_str(), a_URL.GetHost().c_str(), a_URL.GetPort() );

	ConnectionMap::iterator iConnections = GetConnectionMap().find( hashId );
	while( iConnections != GetConnectionMap().end() )
	{
		ConnectionList & connections = iConnections->second;
		IWebClient::SP spConnection = connections.front();
		connections.pop_front();

		if ( connections.begin() == connections.end() )
		{
			GetConnectionMap().erase( iConnections );
			iConnections = GetConnectionMap().end();
		}

		if ( spConnection->GetState() == IWebClient::CONNECTED )
			return spConnection;
	}

	bool bSecure = (_stricmp( a_URL.GetProtocol().c_str(), "https" ) == 0 || 
		_stricmp( a_URL.GetProtocol().c_str(), "wss" ) == 0 );
	SP spClient = SP( GetFactory().CreateObject( bSecure ? "SecureWebClient" : "WebClient" ) );
	if ( spClient )
		spClient->SetURL( a_URL );

	return spClient;
}

void IWebClient::Free( const SP & a_spClient )
{
	if ( a_spClient )
	{
		a_spClient->ClearDelegates();

		if ( a_spClient->GetState() == CONNECTED )
		{
			const URL & url = a_spClient->GetURL();
			std::string hashId = StringUtil::Format( "%s://%s:%d", 
				url.GetProtocol().c_str(), url.GetHost().c_str(), url.GetPort() );
			GetConnectionMap()[ hashId ].push_back( a_spClient );
		}
	}
}

//----------------------------------------------

template<typename socket_type>
class WDC_API WebClientT : public IWebClient
{
public:
	//! Types
	typedef boost::shared_ptr<WebClientT>			SP;
	typedef boost::weak_ptr<WebClientT>				WP;

	SP shared_from_this()
	{
		return boost::static_pointer_cast<WebClientT>( IWebClient::shared_from_this() );
	}

	WebClientT() :
		m_eState(CLOSED),
		m_pSocket(NULL),
		m_WebSocket(false),
		m_RequestType("GET"),
		m_ContentLen( 0xffffffff ), 
		m_bChunked( false ),
		m_SendError( false ),
		m_SendCount( 0 )
	{}

	~WebClientT()
	{
		Cleanup();
	}

	virtual SocketState GetState() const
	{
		return m_eState;
	}

	virtual const URL & GetURL() const
	{
		return m_URL;
	}

	virtual const Headers & GetHeaders() const
	{
		return m_Headers;
	}

	virtual void SetURL(const URL & a_URL)
	{
		m_URL = a_URL;
	}

	virtual void SetStateReceiver(Delegate<IWebClient *> a_StateReceiver)
	{
		m_StateReceiver = a_StateReceiver;
	}

	virtual void SetDataReceiver(Delegate<RequestData *> a_Receiver)
	{
		m_DataReceiver = a_Receiver;
	}

	virtual void SetHeader(const std::string & a_Key, const std::string & a_Value)
	{
		m_Headers[ a_Key ] = a_Value;
	}

	virtual void SetHeaders(const Headers & a_Headers, bool a_bMerge = false )
	{
		if ( a_bMerge )
		{
			for( Headers::const_iterator iHeader = a_Headers.begin(); iHeader != a_Headers.end(); ++iHeader )
				m_Headers[iHeader->first] = iHeader->second;
		}
		else
			m_Headers = a_Headers;
	}

	virtual void SetRequestType(const std::string & a_ReqType)
	{
		m_RequestType = a_ReqType;
	}

	virtual void SetBody( const std::string & a_Body )
	{
		m_Body = a_Body;
	}

	virtual void SetFrameReceiver( Delegate<FrameSP> a_Receiver )
	{
		m_OnFrame = a_Receiver;
	}

	virtual void SetErrorHandler( Delegate<IWebSocket *> a_Handler)
	{
		m_OnError = a_Handler;
	}

	virtual bool Send() 
	{
		WebClientService * pService = WebClientService::Instance();
		if ( pService == NULL )
			return false;		// this would only happen if we are in the middle of shutting down..

		bool bWebSocket = _stricmp( m_URL.GetProtocol().c_str(), "ws" ) == 0 
			|| _stricmp( m_URL.GetProtocol().c_str(), "wss" ) == 0;
		if ( m_eState != CONNECTED || !m_URL.CanUseConnection( m_ConnectedURL ) || bWebSocket )
		{
			m_WebSocket = bWebSocket;
			m_ConnectedURL = m_URL;

			Cleanup();
			CreateSocket();
			SetState(CONNECTING);

			Log::DebugMed("WebClientT", "Connecting to %s:%u", m_URL.GetHost().c_str(), m_URL.GetPort());

			WebClientService::Instance()->GetService().post( 
				boost::bind( &WebClientT::BeginConnect, shared_from_this() ) );
		}
		else
		{
			// we are already connected, just send the request..
			SendRequest();
		}

		return true;
	}

	virtual bool Close()
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
		Log::DebugLow( "WebClientT", "Closing socket. (%p)", this );
		m_pSocket->lowest_layer().close();
		return true;
	}

	virtual bool Shutdown()
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

	//! IWebSocket interface
	virtual void ClearDelegates()
	{
		m_StateReceiver.Reset();
		m_DataReceiver.Reset();
		m_OnFrame.Reset();
		m_OnError.Reset();
	}

	virtual void SendBinary(const std::string & a_BinaryData)
	{
		if (m_eState != CONNECTED && m_eState != CONNECTING)
			Log::Error( "WebClientT", "SendBinary() called with WebClientT in wrong state." );
		else if (! m_WebSocket )
			Log::Error( "WebClientT", "SendBinary() invoked for non-WebSocket." );
		else
			WS_Send(BINARY_FRAME, a_BinaryData );
	}

	virtual void SendText(const std::string & a_TextData)
	{
		if ( utf8::find_invalid( a_TextData.begin(), a_TextData.end()) != a_TextData.end() )
			Log::Error( "WebClientT", "Invalid characters string passed to SendText()." );
		else if ( m_eState != CONNECTED && m_eState != CONNECTING )
			Log::Error( "WebClientT", "SendText() called with WebClientT in wrong state." );
		else if (! m_WebSocket )
			Log::Error( "WebClientT", "SendText() invoked for non-WebSocket." );
		else
			WS_Send(TEXT_FRAME, a_TextData );
	}

	virtual void SendPing(const std::string & a_PingData)
	{
		if (m_eState != CONNECTED && m_eState != CONNECTING)
			Log::Error( "WebClientT", "SendBinary() called with WebClientT in wrong state." );
		else if (! m_WebSocket )
			Log::Error( "WebClientT", "SendBinary() invoked for non-WebSocket." );
		else
			WS_Send(PING, a_PingData );
	}

	virtual void SendPong(const std::string & a_PingData)
	{
		if (m_eState != CONNECTED && m_eState != CONNECTING)
			Log::Error( "WebClientT", "SendBinary() called with WebClientT in wrong state." );
		else if (! m_WebSocket )
			Log::Error( "WebClientT", "SendBinary() invoked for non-WebSocket." );
		else
			WS_Send(PONG, a_PingData );
	}

	virtual void SendClose(const std::string & a_Reason)
	{
		if (m_eState != CONNECTED && m_eState != CONNECTING)
			Log::Error( "WebClientT", "SendBinary() called with WebClientT in wrong state." );
		else if (! m_WebSocket )
			Log::Error( "WebClientT", "SendBinary() invoked for non-WebSocket." );
		else
			WS_Send(CLOSE, a_Reason );
	}
protected:

	void SetState(SocketState a_eState)
	{
		m_eState = a_eState;
		if ( m_StateReceiver.IsValid() )
			m_StateReceiver( this );
	}

	virtual void CreateSocket() = 0;

	void BeginConnect()
	{
		WebClientService * pService = WebClientService::Instance();
		assert( pService != NULL );

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
			Log::Error("WebClientT", "Caught exception: %s", ex.what());
		}

		if (i == boost::asio::ip::tcp::resolver::iterator())
		{
			Log::Error("WebClientT", "Failed to resolve %s", m_URL.GetHost().c_str());
			ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( WebClientT, OnDisconnected, shared_from_this() ) );
		}
		else
		{
			m_pSocket->lowest_layer().async_connect(*i,
				boost::bind(&WebClientT::HandleConnect, shared_from_this(), boost::asio::placeholders::error, i));
		}
	}

	//! This is used by SecureWebClient to start the hand-shake
	virtual bool StartHandshake()
	{
		return false;
	}

	void HandleConnect(const boost::system::error_code & error,
		boost::asio::ip::tcp::resolver::iterator i)
	{
		if (! error )
		{
			if (! StartHandshake() )
			{
				// no handshake needed for non-secure connections, go ahead and send the request
				ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( WebClientT, OnConnected, shared_from_this() ) );
			}

	#if ENABLE_KEEP_ALIVE
			try {
				// this helps recognizing disconnected sockets..
				boost::asio::socket_base::keep_alive option(true);
				m_pSocket->set_option(option);
			}
			catch( const std::exception & ex )
			{
				Log::Error("WebClientT", "Caught exception: %s", ex.what());
			}
	#endif
		}
		else if ( ++i != boost::asio::ip::tcp::resolver::iterator() )
		{
			// try the next end-point in DNS..
			try {
				m_pSocket->lowest_layer().close();
				m_pSocket->lowest_layer().async_connect( *i,
					boost::bind(&WebClientT::HandleConnect, shared_from_this(), boost::asio::placeholders::error, i));
			}
			catch( const std::exception & ex )
			{
				Log::Error("WebClientT", "Caught exception: %s", ex.what());
				ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnDisconnected, shared_from_this() ));
			}
		}
		else
		{
			// set our state to disconnected..
			Log::Error("WebClientT", "Failed to connect to %s:%d: %s", 
				m_URL.GetHost().c_str(), m_URL.GetPort(), error.message().c_str() );
			ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnDisconnected, shared_from_this() ));
		}
	}

	//! Invoked on main thread.
	virtual void OnConnected()
	{
		if ( m_eState == CONNECTING )
		{
			SetState( CONNECTED );
			SendRequest();
		}
		else
		{
			Log::Debug( "WebClientT", "State is not CONNECTING");
			if ( m_eState == CLOSING )
				ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnClose, shared_from_this()));
			else
				ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnDisconnected, shared_from_this()));
		}
	}

	void SendRequest()
	{
		RequestData * pResponse = new RequestData();

		sm_RequestsSent++;

		m_Request.clear();
		std::string & req = m_Request;
		if ( !m_WebSocket )
		{
			if ( m_Headers.find( "Accept" ) == m_Headers.end() )
				m_Headers["Accept"] = "*/*";
			if ( m_Headers.find( "Host" ) == m_Headers.end() )
				m_Headers["Host"] = m_URL.GetHost();
			m_Headers["User-Agent"] = "SelfWebClient";
			m_Headers["Connection"] = "Keep-Alive";			// change to close to avoid reusing connections
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

			Log::Error( "WebClientT", "Request is empty, closing connection." );
			ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnClose, shared_from_this() ));
		}
		else if (m_pSocket != NULL)
		{
			boost::asio::async_write(*m_pSocket,
				boost::asio::buffer(req.c_str(), req.length()),
				boost::bind(&WebClientT::HTTP_RequestSent, shared_from_this(), pResponse,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		sm_BytesSent += req.length();
	}

	void HTTP_RequestSent( RequestData * a_pReq, const boost::system::error_code& error,
		size_t bytes_transferred)
	{
		if (!error) 
		{
			// read the first line containing the status..
			if ( m_pSocket != NULL )
			{
				boost::asio::async_read_until(*m_pSocket,
					m_Response, "\r\n\r\n",
					boost::bind(&WebClientT::HTTP_ReadHeaders, shared_from_this(), a_pReq,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
		}
		else 
		{
			delete a_pReq;

			Log::Error( "WebClientT", "Error on RequestSent(): %s", error.message().c_str() );
			ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnDisconnected, shared_from_this()));
		}
	}

	void HTTP_ReadHeaders( RequestData * a_pReq, const boost::system::error_code& error, size_t bytes_transferred)
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

				// handle cookies differently, since we will received multiple Set-Cookie headers for each cookie..
				if ( StringUtil::Compare( "Set-Cookie", key, true ) == 0 )
					a_pReq->m_SetCookies.insert( Cookies::value_type( key, value ) );
				else
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
						Log::Debug( "WebClientT", "Sending %u pending frames.", m_Pending.size() );
						for (BufferList::iterator iSend = m_Pending.begin(); iSend != m_Pending.end(); ++iSend)
							WS_QueueSend(*iSend);
						m_Pending.clear();
					}

					// start reading WebSocket frames..
					WS_Read( a_pReq, error, 0 );
				}
				else
				{
					Log::Error( "WebClientT", "Websocket failed to connect, status code %u: %s", 
						a_pReq->m_StatusCode, a_pReq->m_StatusMessage.c_str() );
					m_SendError = true;
					if ( m_SendCount == 0 )
						ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnDisconnected, shared_from_this()));
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

				if ( m_bChunked )
					HTTP_ReadChunkLength( a_pReq );
				else
					HTTP_ReadContent( a_pReq, error, 0 );
			}
		}
		else 
		{
			Log::Error( "WebClientT", "HTTP_ReadHeaders: %s", error.message().c_str() );
			ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnDisconnected, shared_from_this()));
			delete a_pReq;
		}
	}

	void HTTP_ReadChunkLength( RequestData * a_pReq )
	{
		// read the chunk length... 
		boost::asio::async_read_until(*m_pSocket,
			m_Response, "\r\n",
			boost::bind(&WebClientT::HTTP_OnChunkLength, shared_from_this(), a_pReq,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}

	void HTTP_OnChunkLength( RequestData * a_pReq, const boost::system::error_code & error, size_t bytes_transferred )
	{
		if (! error )
		{
			std::istream input(&m_Response);
			std::string chunk_length;
			std::getline(input,chunk_length);

			//Log::Status( "WebClient", "Read Chunk Len: %s", chunk_length.c_str() );
			if ( chunk_length == "\r" )
			{
				// empty line, read the next line..
				HTTP_ReadChunkLength( a_pReq );
			}
			else if ( chunk_length == "0\r" )
			{
				// end of chunked content
				a_pReq->m_bDone = true;
				ThreadPool::Instance()->InvokeOnMain<RequestData *>(
					DELEGATE(WebClientT, OnResponse, RequestData *, shared_from_this()), a_pReq);
			}
			else
			{
				m_ContentLen = strtoul( chunk_length.c_str(), NULL, 16 );
				HTTP_ReadContent( a_pReq, error, 0 );
			}
		}
		else
		{
			Log::Error( "WebClientT", "Error on HTTP_OnChunkLength(): %s", error.message().c_str() );
			ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnDisconnected, shared_from_this()));
			delete a_pReq;
		}
	}

	void HTTP_ReadContent( RequestData * a_pReq, const boost::system::error_code& error, size_t bytes_transferred)
	{
		sm_BytesRecv += bytes_transferred;
		
		if (! error )
		{
			size_t max_read = (size_t)m_Response.in_avail();
			if ( max_read > m_ContentLen )
				max_read = m_ContentLen;

			std::istream input(&m_Response);
			if ( max_read > 0 )
			{
				std::string content;
				content.resize( max_read );
				input.read( &content[0], max_read );

				a_pReq->m_Content += content;
				m_ContentLen -= max_read;
			}

			if (m_ContentLen > 0) 
			{
				//// if we are not chunked and we have no content len, then go ahead and start piping 
				//// data to the user 4k at a time.
				//if (!m_bChunked && a_pReq->m_Content.size() >= (4 * 1024))
				//{
				//	RequestData * pNewReq = new RequestData( *a_pReq );
				//	ThreadPool::Instance()->InvokeOnMain<RequestData *>(
				//		DELEGATE(WebClientT, OnResponse, RequestData *, shared_from_this()), a_pReq);
				//	a_pReq = pNewReq;
				//}

				boost::asio::async_read(*m_pSocket, m_Response,
					boost::asio::transfer_exactly(m_ContentLen),
					boost::bind(&WebClientT::HTTP_ReadContent, shared_from_this(), a_pReq,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
			else if ( m_bChunked )
			{
				// send the chunk, then go try to read the next chunk length..
				RequestData * pNewReq = new RequestData( *a_pReq );
				ThreadPool::Instance()->InvokeOnMain<RequestData *>(
					DELEGATE(WebClientT, OnResponse, RequestData *, shared_from_this()), a_pReq);
				a_pReq = pNewReq;

				// read the next chunk length..
				HTTP_ReadChunkLength( a_pReq );
			}
			else
			{
				a_pReq->m_bDone = true;
				ThreadPool::Instance()->InvokeOnMain<RequestData *>(
					DELEGATE(WebClientT, OnResponse, RequestData *, shared_from_this()), a_pReq);
			}
		}
		else if ( error == boost::asio::error::eof )
		{
			a_pReq->m_bDone = true;
			ThreadPool::Instance()->InvokeOnMain<RequestData *>(
				DELEGATE(WebClientT, OnResponse, RequestData *, shared_from_this()), a_pReq);
		}
		else
		{
			Log::Error( "WebClientT", "Error on HTTP_ReadContent(): %s", error.message().c_str() );
			ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnDisconnected, shared_from_this()));
			delete a_pReq;
		}
	}

	void WS_Read( RequestData * a_pReq, const boost::system::error_code & error,
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
				pFrame->m_wpSocket = shared_from_this();

				bool bClose = pFrame->m_Op == CLOSE;
				if ( bClose )
					Log::DebugLow( "WebClientT", "Received close op: %s (%p)", pFrame->m_Data.c_str(), this );

				ThreadPool::Instance()->InvokeOnMain<IWebSocket::Frame *>(
					DELEGATE( WebClientT, OnWebSocketFrame, IWebSocket::Frame *, shared_from_this() ), pFrame );

				if ( bClose )
				{
					ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( WebClientT, OnClose, shared_from_this() ) );
					m_WebSocket = false;
					delete a_pReq;
				}
			}

			if (m_WebSocket)
			{
				if (!error)
				{
					// continue reading from the web socket..
					if (m_pSocket != NULL)
					{
						boost::asio::async_read(*m_pSocket, m_Response,
							boost::asio::transfer_at_least(1),
							boost::bind(&WebClientT::WS_Read, shared_from_this(), a_pReq,
								boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred));
					}
				}
				else
				{
					Log::DebugLow("WebClientT", "Error on WS_Read(): %s (%p)", error.message().c_str(), this );

					m_SendError = true;
					if ( m_SendCount == 0 && ThreadPool::Instance() != NULL )
						ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(WebClientT, OnDisconnected, shared_from_this()));
					delete a_pReq;
				}
			}
		}
	}

	static void Append( std::string & a_Buffer, const void * a_pData, size_t a_nBytes )
	{
		a_Buffer += std::string( (const char *)a_pData, a_nBytes );
	}

	void WS_Send( OpCode a_Op, const std::string & a_Data, bool a_bUseMask = true )
	{
		std::string * pPacket = new std::string();
		WebSocketFramer::CreateFrame( *pPacket, a_Op, a_Data, a_bUseMask);

		WS_QueueSend( pPacket );
	}

	void WS_QueueSend(std::string * a_pBuffer)
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
			Log::Debug("WebClientT", "Ignoring send because of error state.");
		}
	}

	//! This sends the data over the socket no matter what, it doesn't care if the data is overlapping
	//! in any way.
	void WS_SendNext()
	{
		boost::lock_guard<boost::recursive_mutex> lock( m_SendLock );
		if (m_Send.begin() != m_Send.end())
		{
			std::string * pFrame = m_Send.front();
			m_Send.pop_front();

	#if ENABLE_DEBUGGING
			Log::Debug("WebClientT", "Sending %u bytes (%p).", pFrame->size(), pFrame);
	#endif

			if (m_pSocket != NULL)
			{
				boost::asio::async_write(*m_pSocket,
					boost::asio::buffer(pFrame->c_str(), pFrame->size()),
					boost::bind(&WebClientT::WS_Sent, shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred,
						pFrame));

				// we need to know how many outstanding sends we have..
				m_SendCount += 1;
				sm_BytesSent += pFrame->size();
			}
		}
	}

	void WS_Sent( const boost::system::error_code& error, size_t bytes_transferred, std::string * pBuffer )
	{
		boost::lock_guard<boost::recursive_mutex> lock( m_SendLock );

		m_SendCount -= 1;
		if ( error || m_SendError )
		{
			if (!m_SendError )
			{
				Log::Error( "WebClientT", "Error sending web socket frame : %s", error.message().c_str() );
				m_SendError = true;
			}

			// once we number of outstanding sends is 0, then let the main thread know we've been disconnected.
			if ( m_SendCount == 0 && ThreadPool::Instance() != NULL )
				ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE(WebClientT, OnDisconnected, shared_from_this()) );
		}
		else
		{
	#if ENABLE_DEBUGGING
			Log::Debug( "WebClientT", "WS_Sent %u bytes (%p) (%u pending)", bytes_transferred, pBuffer, m_SendCount );
	#endif
			// send the next block, this will do nothing if nothing is queued..
			if ( m_SendCount == 0 )
				WS_SendNext();
		}

		delete pBuffer;
	}

	void OnResponse(RequestData * a_pData)
	{
		bool bClose = false;
		Headers::iterator iConnection = a_pData->m_Headers.find( "Connection" );
		if ( iConnection != a_pData->m_Headers.end() )
			bClose = _stricmp( iConnection->second.c_str(), "close") == 0;

	#if defined(WARNING_DELEGATE_TIME) && defined(ERROR_DELEGATE_TIME)
		double startTime = Time().GetEpochTime();
	#endif
		if ( m_DataReceiver.IsValid() )
			m_DataReceiver( a_pData );
	#if defined(WARNING_DELEGATE_TIME) && defined(ERROR_DELEGATE_TIME)
		double elapsed = Time().GetEpochTime() - startTime;
		if(elapsed > WARNING_DELEGATE_TIME)
		{
			if ( elapsed > ERROR_DELEGATE_TIME )
				Log::Error("ThreadPool", "Delegate %s:%d took %f seconds to invoke on main thread.", 
					m_DataReceiver.GetFile(), m_DataReceiver.GetLine(), elapsed );
			else
				Log::Warning("ThreadPool", "Delegate %s:%d took %f seconds to invoke on main thread.", 
					m_DataReceiver.GetFile(), m_DataReceiver.GetLine(), elapsed );
		}
	#endif

		// close the socket afterwards, only if 
		if ( bClose && a_pData->m_bDone )
			ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( WebClientT, OnClose, shared_from_this() ) );

		delete a_pData;
	}

	void OnWebSocketFrame( IWebSocket::Frame * a_pFrame )
	{
		FrameSP spFrame( a_pFrame );
		if ( m_OnFrame.IsValid() )
			m_OnFrame( spFrame );
	}

	virtual void OnClose()
	{
		if ( m_eState == CONNECTED || 
			m_eState == CONNECTING || 
			m_eState == CLOSING )
		{
			Log::DebugLow( "WebClientT", "OnClose() closing socket. (%p)", this );
			SetState( CLOSED );
		}
	}

	virtual void OnDisconnected()
	{
		if ( m_eState == CONNECTED || 
			m_eState == CONNECTING || 
			m_eState == CLOSING )
		{
			assert( m_SendCount == 0 );
			Log::DebugLow( "WebClientT", "OnDisconnected() closing socket. (%p)", this );

			// if Close() is called, then we set the state to close and just close the socket. The async
			// routines will think it's been disconnected and they will invoke OnDisconnected(), ignore
			// changing the state to disconnected when it was a client-side initiated close.
			if (m_eState != CLOSING)
				SetState(DISCONNECTED);
			else
				SetState(CLOSED);
		}
	}

	virtual void Cleanup()
	{
		if ( m_pSocket != NULL )
		{
			delete m_pSocket;
			m_pSocket = NULL;
		}
		m_SendError = false;
		m_Pending.clear();
		m_Send.clear();
	}


protected:
	//! Types
	typedef std::list<std::string *>		BufferList;

	//! Data
	SocketState		m_eState;
	URL				m_URL;					// URL we want to connect toos
	URL				m_ConnectedURL;			// URL we are actually connected too
	Headers			m_Headers;
	std::string		m_RequestType;
	std::string		m_Body;

	//! WebSocket data
	bool			m_WebSocket;			// set to true if we have a web socket
	Delegate<IWebClient *>
					m_StateReceiver;
	Delegate<RequestData *>
					m_DataReceiver;
	Delegate<FrameSP>
					m_OnFrame;
	Delegate<IWebSocket *>
					m_OnError;

	socket_type *	m_pSocket;

	std::string		m_Request;				// the sent request 
	boost::asio::streambuf
					m_Response;				// response buffer
	std::string		m_Incoming;				// received web socket data
	BufferList		m_Pending;				// pending sends
	BufferList		m_Send;					// send queue
	bool			m_bChunked;
	size_t			m_ContentLen;

	volatile bool	m_SendError;			// set to true when a send fails
	boost::atomic<size_t>
					m_SendCount;			// number of outstanding websocket sends
	boost::recursive_mutex
					m_SendLock;

};

class WebClient : public WebClientT<boost::asio::ip::tcp::socket>
{
public:
	RTTI_DECL();

	//! WebClientT interface
	virtual void CreateSocket()
	{
		WebClientService * pService = WebClientService::Instance();
		assert( pService != NULL );

		m_pSocket = new boost::asio::ip::tcp::socket( pService->GetService() );
	}
};

RTTI_IMPL( WebClient, IWebClient );
REG_FACTORY( WebClient, IWebClient::GetFactory() );

class SecureWebClient : public WebClientT<boost::asio::ssl::stream< boost::asio::ip::tcp::socket > >
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<SecureWebClient>			SP;
	typedef boost::weak_ptr<SecureWebClient>				WP;

	SP shared_from_this()
	{
		return boost::static_pointer_cast<SecureWebClient>( IWebClient::shared_from_this() );
	}
	
	//! Construction
	SecureWebClient() : m_pSSL( NULL )
	{}

	//! WebClientT interface
	virtual void CreateSocket()
	{
		WebClientService * pService = WebClientService::Instance();
		assert( pService != NULL );

		// make the socket..
		m_pSSL = new boost::asio::ssl::context( pService->GetService(), boost::asio::ssl::context::sslv23 );
		m_pSSL->set_verify_mode(boost::asio::ssl::context::verify_none);
		m_pSocket = new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>( pService->GetService(), *m_pSSL );
	}
	virtual bool StartHandshake()
	{
		m_pSocket->async_handshake( boost::asio::ssl::stream_base::client, 
			boost::bind( &SecureWebClient::HandleHandShake, shared_from_this(), boost::asio::placeholders::error ) );
		return true;
	}
	virtual void Cleanup()
	{
		WebClientT::Cleanup();

		if ( m_pSSL != NULL )
		{
			delete m_pSSL;
			m_pSSL = NULL;
		}
	}
protected:
	virtual void OnConnected()
	{
		WebClientT::OnConnected();
	}
	virtual void OnDisconnected()
	{
		WebClientT::OnDisconnected();
	}

	void HandleHandShake(const boost::system::error_code & error)
	{
		if (! error )
		{
			ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( SecureWebClient, OnConnected, shared_from_this() ) );
		}
		else
		{
			Log::Error( "WebClientT", "Handshake Failed with %s:%d: %s", 
				m_URL.GetHost().c_str(), m_URL.GetPort(), error.message().c_str() );
			ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( SecureWebClient, OnDisconnected, shared_from_this() ) );
		}
	}
private:
	//! Data
	boost::asio::ssl::context *		m_pSSL;
};

RTTI_IMPL( SecureWebClient, IWebClient );
REG_FACTORY( SecureWebClient, IWebClient::GetFactory() );

//----------------------------------

