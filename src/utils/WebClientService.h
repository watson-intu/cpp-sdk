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


#ifndef WDC_WEB_CLIENT_SERVICE_H
#define WDC_WEB_CLIENT_SERVICE_H

#include <map>

#include "boost/asio.hpp"		// not including SSL at this level on purpose
#include "boost/thread.hpp"
#include "boost/thread/mutex.hpp"

#if defined(ENABLE_OPENSSL_INCLUDES)
#include "boost/asio/ssl.hpp"
#else

// boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *
namespace boost {
	namespace asio {
		namespace ssl{
			template<class SOCKET>
			class stream
			{};
			class context
			{};
		};
	};
};

#endif
#include "boost/atomic.hpp"

#include "TimerPool.h"
#include "UtilsLib.h"		

//! This singleton class manages the boost::asio::io_service object and a pool of already established connections.
class UTILS_API WebClientService
{
public:
	static WebClientService * Instance();
	static int sm_ThreadCount;					// how many threads to start for the WebClient

	WebClientService();
	~WebClientService();

	boost::asio::io_service & GetService()
	{
		return m_Service;
	}

private:
	//! Types
	typedef boost::asio::io_service::work	Work;
	typedef boost::thread					Thread;
	typedef boost::shared_ptr<Thread>		ThreadSP;
	typedef std::list<ThreadSP>				ThreadList;

	//! Data
	boost::asio::io_service	m_Service;
	boost::asio::io_service::work
							m_Work;
	ThreadList				m_Threads;

	TimerPool::ITimer::SP	m_spStatsTimer;

	static WebClientService *
							sm_pInstance;

	void OnDumpStats();
};


#endif

