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

#include "WebClientService.h"
#include "Log.h"
#include "IWebClient.h"

WebClientService * WebClientService::sm_pInstance = NULL;
int WebClientService::sm_ThreadCount = 1;					// how many threads to start for the WebClient

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
	m_Work(m_Service)										// this prevents the IO service from stopping on it's own
{
	for(int i=0;i<sm_ThreadCount;++i)
		m_Threads.push_back(ThreadSP( new Thread(boost::bind(&boost::asio::io_service::run, &m_Service) ) ) );
	if (TimerPool::Instance() != NULL)
		m_spStatsTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(WebClientService, OnDumpStats, this), 60.0, true, true);
}

WebClientService::~WebClientService()
{
	if (sm_pInstance == this)
		sm_pInstance = NULL;
	m_spStatsTimer.reset();

	m_Service.stop();
	for (ThreadList::iterator iThread = m_Threads.begin(); iThread != m_Threads.end(); ++iThread)
		(*iThread)->join();
	m_Threads.clear();
}

void WebClientService::OnDumpStats()
{
	Log::Status("WebClient", "STAT: Requests: %u, Bytes Sent: %u, Bytes Recv: %u",
		IWebClient::sm_RequestsSent.load(),
		IWebClient::sm_BytesSent.load(),
		IWebClient::sm_BytesRecv.load());
}

