/**
* Copyright 2017 IBM Corp. All Rights Reserved.
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


#include <map>

#include "boost/asio.hpp"		
#include "boost/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/asio/ssl.hpp"
#include "boost/atomic.hpp"
#include "boost/enable_shared_from_this.hpp"

#include "Delegate.h"
#include "StringUtil.h"
#include "TimerPool.h"
#include "WebSocketFramer.h"
#include "Log.h"
#include "SHA1.h"
#include "IWebServer.h"
#include "UtilsLib.h"		// include last always

//! Server class for handling incoming REST requests and WebSocket connections. 
template<typename socket_type>
class UTILS_API WebServerT : public IWebServer
{
public:
	//! Types
	typedef socket_type									SocketType;
	typedef boost::asio::streambuf						StreamBuffer;
	typedef boost::shared_ptr<StreamBuffer>				StreamBufferSP;
	typedef boost::recursive_mutex						Mutex;

	//! This class handles a single connection to a client. The request handler
	//! may use this class to send back responses.
	class Connection : public IConnection
	{
	public:
		//! Types
		typedef boost::shared_ptr<Connection>			SP;
		typedef boost::weak_ptr<Connection>				WP;

		//! Construction
		Connection(WebServerT * a_pServer, socket_type * a_pSocket) :
			m_bClosed(false),
			m_bWebSocket(false),
			m_pServer(a_pServer),
			m_pSocket(a_pSocket),
			m_ReadBuffer(new StreamBuffer())
		{}
		virtual ~Connection()
		{
			Close();

			delete m_pSocket;
		}

		//! Accessors
		socket_type * GetSocket() const
		{
			return m_pSocket;
		}
		const StreamBufferSP & GetReadBuffer() const
		{
			return m_ReadBuffer;
		}

		//! Start a timeout for this connection, if cancel() is not called on the returned timer
		//! before it fires, then this socket will be closed automatically.
		void StartTimeout(float a_fSeconds)
		{
			TimerPool * pPool = TimerPool::Instance();
			if (pPool != NULL)
				m_spTimeoutTimer = pPool->StartTimer(VOID_DELEGATE(Connection, OnTimeout, this), a_fSeconds, true, false);
		}
		void CancelTimeout()
		{
			m_spTimeoutTimer.reset();
		}

		//! IWebSocket interface
		virtual void ClearDelegates()
		{
			m_OnFrame.Reset();
			m_OnError.Reset();
		}
		virtual void SetFrameReceiver(Delegate<FrameSP> a_Receiver)
		{
			m_OnFrame = a_Receiver;
		}
		virtual void SetErrorHandler(Delegate<IWebSocket *> a_Handler)
		{
			m_OnError = a_Handler;
		}

		virtual void SendBinary(const std::string & a_Binary)
		{
			std::string frame;
			WebSocketFramer::CreateFrame(frame, IWebSocket::BINARY_FRAME, a_Binary, false);
			SendAsync(frame);
		}

		virtual void SendText(const std::string & a_Text)
		{
			std::string frame;
			WebSocketFramer::CreateFrame(frame, IWebSocket::TEXT_FRAME, a_Text, false);
			SendAsync(frame);
		}

		virtual void SendPing(const std::string & a_PingData)
		{
			std::string frame;
			WebSocketFramer::CreateFrame(frame, IWebSocket::PING, a_PingData, false);
			SendAsync(frame);
		}

		virtual void SendPong(const std::string & a_PingData)
		{
			std::string frame;
			WebSocketFramer::CreateFrame(frame, IWebSocket::PONG, a_PingData, false);
			SendAsync(frame);
		}

		virtual void SendClose(const std::string & a_Reason)
		{
			std::string frame;
			WebSocketFramer::CreateFrame(frame, IWebSocket::CLOSE, a_Reason, false);
			SendAsync(frame);
		}


		//! IConnection interface
		virtual bool IsClosed() const
		{
			return m_bClosed;
		}

		virtual bool IsWebSocket() const
		{
			return m_bWebSocket;
		}

		virtual bool Close()
		{
			if (!m_bClosed)
			{
				m_bClosed = true;

				if (m_bWebSocket)
				{
					SendClose("Close Requested");
					m_bWebSocket = false;
				}

				try {
					m_pSocket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
					m_pSocket->lowest_layer().close();
				}
				catch (const std::exception & ex)
				{
					Log::Debug("Connection", "Caught Exception: %s", ex.what());
				}
			}
			return m_bClosed;
		}

		virtual void SendAsync(const std::string & a_Send)
		{
			m_SendLock.lock();
			bool bSend = m_Sending.begin() == m_Sending.end();
			m_Sending.push_back( a_Send );
			m_SendLock.unlock();

			if ( bSend )
				OnSend();
		}

		virtual void ReadAsync(size_t a_Bytes, Delegate< std::string * > a_ReadCallback)
		{
			size_t nBytesAvail = (size_t)m_ReadBuffer->in_avail();
			if ( a_Bytes > nBytesAvail )
			{
				boost::asio::async_read(*m_pSocket, *m_ReadBuffer,
					boost::asio::transfer_at_least(a_Bytes - nBytesAvail),
					boost::bind(&Connection::OnRead, shared_from_this(), a_ReadCallback,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred ));
			}
			else
			{
				// we already have enough bytes in the buffer, return those bytes now..
				OnRead( a_ReadCallback, boost::system::error_code(), nBytesAvail );
			}
		}

		virtual void SendResponse(int a_nStatusCode, const std::string & a_Reply, const Headers & a_Headers,
			const std::string & a_Content, bool a_bClose = true)
		{
			std::string response(StringUtil::Format("HTTP/1.1 %d %s\r\n", a_nStatusCode, a_Reply.c_str()));
			for (typename Headers::const_iterator iHeader = a_Headers.begin(); iHeader != a_Headers.end(); ++iHeader)
				response += iHeader->first + " : " + iHeader->second + "\r\n";
			response += "\r\n";
			if (a_Content.size() > 0)
				response += a_Content;

			SendAsync(response);
			if (a_bClose)
				Close();
		}

		virtual void SendResponse(int a_nStatusCode, const std::string & a_Reply,
			const std::string & a_Content, bool a_bClose = true)
		{
			std::string response(StringUtil::Format("HTTP/1.1 %d %s\r\n\r\n", a_nStatusCode, a_Reply.c_str()));
			if (a_Content.size() > 0)
				response += a_Content;

			SendAsync(response);
			if (a_bClose)
				Close();
		}

		virtual void StartWebSocket(const std::string & a_WebSocketKey)
		{
			std::stringstream output;

			std::string sha1(SHA1(a_WebSocketKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
			output << "HTTP/1.1 101 Web Socket Protocol Handshake\r\n";
			output << "Cache-Control: no-cache\r\n";
			output << "Upgrade: websocket\r\n";
			output << "Connection: Upgrade\r\n";
			output << "Sec-WebSocket-Accept: " << StringUtil::EncodeBase64(sha1) << "\r\n";
			output << "\r\n";

			SendAsync( output.str() );

			m_bWebSocket = true;

			// start reading data for this web-socket, we pass the shared_pointer for this connection
			// which will keep this socket open until a read-fails or the user calls close.
			boost::asio::async_read(*m_pSocket, *m_ReadBuffer,
				boost::asio::transfer_at_least(1),
				boost::bind(&Connection::OnReadWS, shared_from_this(), boost::asio::placeholders::error));
		}

		SP shared_from_this()
		{
			return boost::static_pointer_cast<Connection>( IWebSocket::shared_from_this() );
		}

	private:
		//! Types
		typedef std::list< FrameSP >		FrameList;
		typedef std::list< std::string >	SendList;

		//! Data
		bool			m_bClosed;
		bool			m_bWebSocket;
		std::string		m_Incoming;
		FrameList		m_Frames;
		Delegate<FrameSP>
						m_OnFrame;
		Delegate<IWebSocket *>
						m_OnError;

		WebServerT *	m_pServer;
		socket_type *	m_pSocket;

		boost::recursive_mutex
						m_SendLock;
		SendList		m_Sending;
		StreamBufferSP	m_ReadBuffer;
		TimerPool::ITimer::SP
						m_spTimeoutTimer;

		void OnReadWS(const boost::system::error_code& ec)
		{
			if (!ec)
			{
				// append received data onto the m_Incoming buffer..
				if (m_ReadBuffer->in_avail() > 0)
				{
					std::istream input(m_ReadBuffer.get());

					std::string received;
					received.resize((size_t)m_ReadBuffer->in_avail());
					input.read(&received[0], received.size());

					m_Incoming += received;
				}

				// a web socket frame is at least 2 bytes in size..
				IWebSocket::Frame * pFrame = NULL;
				while (!m_bClosed && (pFrame = WebSocketFramer::ParseFrame(m_Incoming)) != NULL)
				{
					pFrame->m_wpSocket = shared_from_this();

					if (m_OnFrame.IsValid())
					{
						IWebSocket::FrameSP spFrame(pFrame);
						ThreadPool::Instance()->InvokeOnMain(m_OnFrame, spFrame);
					}
				}

				// continue reading from this socket..
				boost::asio::async_read(*m_pSocket, *m_ReadBuffer,
					boost::asio::transfer_at_least(1),
					boost::bind(&Connection::OnReadWS, shared_from_this(), boost::asio::placeholders::error));
			}
			else
			{
				Log::Error("Connection", "OnReadWS() error: %s", ec.message().c_str());
			}
		}

		void OnRead( Delegate< std::string * > a_ReadCallback,
			const boost::system::error_code& error,
			size_t bytes_transferred )
		{
			if (!error)
			{
				size_t max_read = (size_t)m_ReadBuffer->in_avail();
				if (max_read > 0)
				{
					std::istream input(m_ReadBuffer.get());

					std::string * content = new std::string();
					content->resize(max_read);
					input.read(&(*content)[0], max_read);

					ThreadPool::Instance()->InvokeOnMain<std::string *>( a_ReadCallback, content );
				}
			}
			else
			{
				OnError(error);
			}
		}

		void OnSend()
		{
			try {
				boost::asio::async_write(*m_pSocket, 
					boost::asio::buffer( m_Sending.front().data(), m_Sending.front().size() ),
					boost::bind(&Connection::OnSent, shared_from_this(), boost::asio::placeholders::error));
			}
			catch (const std::exception & ex)
			{
				Log::Error("Connection", "SendAsync Exception: %s", ex.what());
				OnError( boost::system::error_code() );
			}
		}
		void OnSent(const boost::system::error_code & ec)
		{
			m_SendLock.lock();

			m_Sending.pop_front();
			bool bSend = m_Sending.begin() != m_Sending.end();

			m_SendLock.unlock();

			if (!ec)
			{
				if ( bSend )
					OnSend();
			}
			else
			{
				OnError(ec);
			}
		}

		void OnError(const boost::system::error_code & ec)
		{
			if (m_OnError.IsValid())
			{
				Log::Error("Connection", "OnSent() error: %s", ec.message().c_str());

				ThreadPool::Instance()->InvokeOnMain<IWebSocket *>(m_OnError, this );
				m_OnError.Reset();		// reset so we only queue an error once
			}
			Close();
		}

		void OnTimeout()
		{
			Log::Debug("Connection", "OnTimeout()");
			Close();
		}
	};

	WebServerT(const std::string & a_Interface = std::string(),
		int a_nPort = 80, int a_nThreads = 5, float a_fRequestTimeout = 30.0f) :
		m_Acceptor(m_Service),
		m_Interface(a_Interface),
		m_nPort(a_nPort),
		m_nThreads(a_nThreads),
		m_fRequestTimeout(a_fRequestTimeout),
		m_pWork(NULL)
	{}

	~WebServerT()
	{
		Stop();
	}

	//! IWebServer interface
	virtual bool Start()
	{
		if (m_pWork != NULL)
			return false;

		if (m_Service.stopped())
			m_Service.reset();

		m_pWork = new Work(m_Service);

		try {
			boost::asio::ip::tcp::endpoint endpoint;
			if (m_Interface.size() > 0)
				endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(m_Interface), m_nPort);
			else
				endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), m_nPort);

			m_Acceptor.open(endpoint.protocol());
			boost::asio::ip::tcp::acceptor::reuse_address option(true);
			m_Acceptor.set_option(option);
			m_Acceptor.bind(endpoint);
			m_Acceptor.listen();
		}
		catch (const std::exception & ex)
		{
			Log::Error("WebServer", "Caught Exception: %s", ex.what());
			return false;
		}

		Accept();

		m_Threads.clear();
		for (int i = 0; i < m_nThreads; ++i)
			m_Threads.push_back(ThreadSP(new Thread(boost::bind(&boost::asio::io_service::run, &m_Service))));

		Log::Status("WebServer", "Listening on port %d", m_nPort);
		return true;
	}

	virtual bool Stop()
	{
		// destroy the work object to end all threads..
		if (m_pWork == NULL)
			return false;
		delete m_pWork;
		m_pWork = NULL;

		// wait for all the threads to exit..
		m_Service.stop();
		for (ThreadList::iterator iThread = m_Threads.begin(); iThread != m_Threads.end(); ++iThread)
			(*iThread)->join();
		m_Threads.clear();

		// unbind the port
		try {
			m_Acceptor.close();
		}
		catch(const std::exception & ex)
		{
			Log::Warning( "WebServer", "Caught Exception: %s", ex.what() );
		}

		return true;
	}

	//! Add an end-point to this server, the provided delegate will be invoked with the 
	//! Request object when an incoming client connections makes a request that matches 
	//! the provided end-point mask.
	//! NOTE: the delegate will be invoked in a thread from this servers thread-pool.
	virtual void AddEndpoint(const std::string & a_EndPointMask,
		Delegate<RequestSP> a_RequestHandler,
		bool a_bInvokeOnMain = true)
	{
		boost::lock_guard<Mutex> lock(m_EndPointLock);
		m_EndPoints.push_back(EndPoint(a_EndPointMask, a_RequestHandler, a_bInvokeOnMain));
	}

	//! Remove a register end-point.
	virtual bool RemoveEndpoint(const std::string & a_EndPointMask)
	{
		boost::lock_guard<Mutex> lock(m_EndPointLock);
		for (typename EndPointList::iterator iEndPoint = m_EndPoints.begin(); iEndPoint != m_EndPoints.end(); ++iEndPoint)
			if ((*iEndPoint).m_EndPointMask == a_EndPointMask)
			{
				m_EndPoints.erase(iEndPoint);
				return true;
			}
		return false;
	}

	void OnAccepted(ConnectionSP a_spConnection, const boost::system::error_code & ec)
	{
		Accept();		// start accepting the next connection already..
		if (!ec)
			ReadRequest(a_spConnection);
	}

protected:
	//! Types
	typedef boost::asio::io_service			Service;
	typedef boost::asio::io_service::work	Work;
	typedef boost::asio::ip::tcp::acceptor	Acceptor;
	typedef boost::thread					Thread;
	typedef boost::shared_ptr<Thread>		ThreadSP;
	typedef std::list<ThreadSP>				ThreadList;

	//! Structure for a end-point.
	struct EndPoint
	{
		EndPoint(const std::string & a_EndPointMask, Delegate<RequestSP> a_RequestHandler, bool a_bInvokeOnMain) :
			m_EndPointMask(a_EndPointMask), m_RequestHandler(a_RequestHandler), m_bInvokeOnMain(a_bInvokeOnMain)
		{}

		std::string			m_EndPointMask;
		Delegate<RequestSP>	m_RequestHandler;
		bool				m_bInvokeOnMain;
	};
	typedef std::list<EndPoint>	EndPointList;


	//! Data
	std::string		m_Interface;			// interface we are binding onto, empty for all interfaces
	int				m_nPort;				// which port are we listening on
	int				m_nThreads;				// number of threads to start for handling incoming requests
	float			m_fRequestTimeout;		// amount of time from an open connection until we receive the request

	Service			m_Service;
	Work *			m_pWork;
	Acceptor		m_Acceptor;

	ThreadList		m_Threads;				// list of our threads
	Mutex			m_EndPointLock;
	EndPointList	m_EndPoints;			// NOTE: we may want to move to a std::map at some point

											//! Accept incoming connections, this must be provided by the base class.
	virtual void	Accept() = 0;

	void ReadRequest(ConnectionSP a_spConnection)
	{
		Connection * pConnection = static_cast<Connection *>(a_spConnection.get());
		pConnection->StartTimeout(m_fRequestTimeout);

		boost::asio::async_read_until(*pConnection->GetSocket(),
			*pConnection->GetReadBuffer(), "\r\n\r\n",
			boost::bind(&WebServerT::OnRequestRead, this, a_spConnection, 
				boost::asio::placeholders::error));
	}

	void OnRequestRead(ConnectionSP a_spConnection, const boost::system::error_code & ec)
	{
		Connection * pConnection = static_cast<Connection *>(a_spConnection.get());
		pConnection->CancelTimeout();

		if (!ec)
		{
			std::istream input( pConnection->GetReadBuffer().get() );

			std::string line;
			std::getline(input, line);

			size_t nRequestEnd = line.find(' ');
			if (nRequestEnd != std::string::npos)
			{
				size_t nEndpointEnd = line.find(' ', nRequestEnd + 1);
				if (nEndpointEnd != std::string::npos)
				{
					RequestSP spRequest(new Request());
					spRequest->m_Origin = pConnection->GetSocket()->lowest_layer().remote_endpoint().address().to_string();
					spRequest->m_RequestType = line.substr(0, nRequestEnd);
					spRequest->m_EndPoint = line.substr(nRequestEnd + 1, nEndpointEnd - nRequestEnd - 1);
					spRequest->m_Protocol = StringUtil::Trim(line.substr(nEndpointEnd + 1), " \r");
					spRequest->m_spConnection = a_spConnection;

					std::getline(input, line);
					size_t nSeperator = line.find(':');
					while (nSeperator != std::string::npos)
					{
						std::string key(StringUtil::Trim(line.substr(0, nSeperator)));
						std::string value(StringUtil::Trim(line.substr(nSeperator + 1), " \r"));
						spRequest->m_Headers[key] = value;

						std::getline(input, line);
						nSeperator = line.find(':');
					}

					// add all query parameters as headers as well.
					size_t nQuery = spRequest->m_EndPoint.find( '?' );
					if ( nQuery != std::string::npos )
					{
						std::string ep( spRequest->m_EndPoint.substr( 0, nQuery ) );
						std::string query( spRequest->m_EndPoint.substr( nQuery + 1 ) );

						std::vector<std::string> params;
						StringUtil::Split( query, "&", params );
						for(size_t i=0;i<params.size();++i)
						{
							const std::string & param = params[i];

							size_t nSeperator = param.find( '=' );
							if ( nSeperator != std::string::npos )
							{
								std::string key( param.substr( 0, nSeperator ) );
								std::string value( param.substr( nSeperator + 1 ) );
								spRequest->m_Headers[key] = StringUtil::UrlUnEscape( value );
							}
							else
								spRequest->m_Headers[param] = "";
						}

						spRequest->m_EndPoint = ep;
					}

					// hand off our stream buffer to the connect, so it can get any remaining bytes..
					ProcessRequest(spRequest);
				}
			}
		}
	}

	void ProcessRequest(RequestSP a_spRequest)
	{
		// automatically upgrade connections to web-sockets if they have a key..
		Delegate<RequestSP> spRequestHandler;
		bool bInvokeOnMain = false;

		m_EndPointLock.lock();
		for (typename EndPointList::const_iterator iEndPoint = m_EndPoints.begin(); iEndPoint != m_EndPoints.end(); ++iEndPoint)
		{
			if (StringUtil::WildMatch((*iEndPoint).m_EndPointMask.c_str(), a_spRequest->m_EndPoint.c_str()))
			{
				spRequestHandler = (*iEndPoint).m_RequestHandler;
				bInvokeOnMain = (*iEndPoint).m_bInvokeOnMain;
				break;
			}
		}
		m_EndPointLock.unlock();

		if (spRequestHandler.IsValid())
		{
			if (bInvokeOnMain)
				ThreadPool::Instance()->InvokeOnMain<RequestSP>(spRequestHandler, a_spRequest);
			else
				spRequestHandler(a_spRequest);
		}
		else
			a_spRequest->m_spConnection->SendResponse(500, "Server Error", "Unsupported end-point.");
	}

};

class WebServer : public WebServerT<boost::asio::ip::tcp::socket>
{
public:
	RTTI_DECL();

	WebServer(const std::string & a_Interface = std::string(),
		int a_nPort = 80, int a_nThreads = 5, float a_fRequestTimeout = 30.0f) :
		WebServerT<boost::asio::ip::tcp::socket>(a_Interface, a_nPort, a_nThreads, a_fRequestTimeout)
	{}

protected:
	//! WebServerBase interface
	virtual void Accept()
	{
		ConnectionSP spConnection(new Connection(this, new WebServerT<boost::asio::ip::tcp::socket>::SocketType(m_Service)));

		Connection * pConnection = static_cast<Connection *>(spConnection.get());
		m_Acceptor.async_accept(*pConnection->GetSocket(),
			boost::bind(&WebServerT<boost::asio::ip::tcp::socket>::OnAccepted, this, spConnection, boost::asio::placeholders::error));
	}
};

class SecureWebServer : public WebServerT< boost::asio::ssl::stream< boost::asio::ip::tcp::socket > >
{
public:
	RTTI_DECL();

	//! Construction
	SecureWebServer(const std::string & a_CertFile,
		const std::string & a_PrivateKeyFile,
		const std::string & a_VerifyFile = std::string(),
		const std::string & a_Interface = std::string(),
		int a_nPort = 443, int a_nThreads = 5, float a_fRequestTimeout = 30.0f) : m_pSSL(NULL),
		WebServerT< boost::asio::ssl::stream< boost::asio::ip::tcp::socket > >(a_Interface, a_nPort, a_nThreads, a_fRequestTimeout)
	{
		m_pSSL = new SSL(boost::asio::ssl::context::sslv23);
		m_pSSL->use_certificate_chain_file(a_CertFile);
		m_pSSL->use_private_key_file(a_PrivateKeyFile, boost::asio::ssl::context::pem);
		if (a_VerifyFile.size() > 0)
			m_pSSL->load_verify_file(a_VerifyFile);
	}
	~SecureWebServer()
	{
		delete m_pSSL;
	}

protected:
	//! WebServerT interface
	virtual void Accept()
	{
		ConnectionSP spConnection(new Connection(this, new SocketType(m_Service, *m_pSSL)));

		Connection * pConnection = static_cast<Connection *>(spConnection.get());
		m_Acceptor.async_accept(pConnection->GetSocket()->lowest_layer(),
			boost::bind(&SecureWebServer::OnBeginHandshake, this, spConnection, boost::asio::placeholders::error));
	}

	void OnBeginHandshake(ConnectionSP a_spConnection, const boost::system::error_code & ec)
	{
		//Immediately start accepting a new connection
		Accept();

		if (!ec)
		{
			Connection * pConnection = static_cast<Connection *>(a_spConnection.get());
			pConnection->StartTimeout(m_fRequestTimeout);

			pConnection->GetSocket()->async_handshake(boost::asio::ssl::stream_base::server,
				boost::bind(&SecureWebServer::OnHandshake, this, a_spConnection, boost::asio::placeholders::error));
		}
	}

	void OnHandshake(ConnectionSP a_spConnection, const boost::system::error_code & ec)
	{
		Connection * pConnection = static_cast<Connection *>(a_spConnection.get());
		pConnection->CancelTimeout();

		if (!ec)
			ReadRequest(a_spConnection);
	}

private:
	//! Types
	typedef boost::asio::ssl::context		SSL;

	//! Data
	SSL *			m_pSSL;
};

RTTI_IMPL_BASE(IWebServer);
RTTI_IMPL(WebServer, IWebServer);
RTTI_IMPL(SecureWebServer, IWebServer);

IWebServer * IWebServer::Create(const std::string & a_Interface /*= std::string()*/,
	int a_nPort /*= 80*/, 
	int a_nThreads /*= 5*/, 
	float a_fRequestTimeout /*= 30.0f*/)
{
	return new WebServer(a_Interface, a_nPort, a_nThreads, a_fRequestTimeout);
}

IWebServer * IWebServer::Create(const std::string & a_CertFile,
	const std::string & a_PrivateKeyFile,
	const std::string & a_VerifyFile /*= std::string()*/,
	const std::string & a_Interface /*= std::string()*/,
	int a_nPort /*= 443*/,
	int a_nThreads /*= 5*/,
	float a_fRequestTimeout /*= 30.0f*/)
{
	return new SecureWebServer(a_CertFile, a_PrivateKeyFile, a_VerifyFile, a_Interface, a_nPort, a_nThreads, a_fRequestTimeout);
}


