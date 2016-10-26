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

//! Define to 1 to protect thread calls against a crash..
#define ENABLE_THREAD_TRY_CATCH					0
//! Define to 1 to enable main delegate timing for finding and fixing callbacks that block the main thread
#define MEASURE_MAIN_DELEGATE_TIMES				1
//! maximum amount of time to spend in a single delegate before we throw an error
const double WARNING_DELEGATE_TIME	= 0.1;
const double ERROR_DELEGATE_TIME	= 0.5;

#include "ThreadPool.h"
#include "WatsonException.h"
#include "Log.h"
#include "Time.h"

ThreadPool * ThreadPool::sm_pInstance = NULL;

//! Construction
ThreadPool::ThreadPool( int a_Threads /*= 10*/ ) : m_ExitCode(0), m_StopMain( false ), m_ThreadCount( a_Threads ), m_ActiveThreads( 0 ), m_BusyThreads( 0 ), m_Shutdown( false )
{
	if ( sm_pInstance != NULL )
		throw WatsonException( "ThreadPool already exists." );
	sm_pInstance = this;

	for(int i=0;i<m_ThreadCount;++i)
		m_Threads.push_back( new tthread::thread( ThreadMain, this ) );
}

ThreadPool::~ThreadPool()
{
	m_Shutdown = true;
	m_WakeThread.notify_all();

	for( ThreadList::iterator iThread = m_Threads.begin(); iThread != m_Threads.end(); ++iThread )
	{
		(*iThread)->join();
		delete *iThread;
	}

	if ( sm_pInstance == this )
		sm_pInstance = NULL;
}

void ThreadPool::ProcessMainThread()
{
	// make a copy of our delegate list to invoke, clean the main queue while it's locked then invoke the 
	// delegates with no lock..
	m_MainQueueLock.lock();
	DelegateList invoke( m_MainQueue );
	m_MainQueue.clear();
	m_MainQueueLock.unlock();

	for( DelegateList::iterator iDelegate = invoke.begin(); iDelegate != invoke.end(); ++iDelegate )
	{
#if MEASURE_MAIN_DELEGATE_TIMES && ENABLE_DELEGATE_DEBUG
        double startTime = Time().GetEpochTime();
#endif
		(*iDelegate)->Invoke();

#if MEASURE_MAIN_DELEGATE_TIMES && ENABLE_DELEGATE_DEBUG
        double elapsed = Time().GetEpochTime() - startTime;
		if(elapsed > WARNING_DELEGATE_TIME)
		{
			if ( elapsed > ERROR_DELEGATE_TIME )
				Log::Error("ThreadPool", "Delegate %s:%d took %f seconds to invoke on main thread.", 
					(*iDelegate)->GetFile(), (*iDelegate)->GetLine(), elapsed );
			else
				Log::Warning("ThreadPool", "Delegate %s:%d took %f seconds to invoke on main thread.", 
					(*iDelegate)->GetFile(), (*iDelegate)->GetLine(), elapsed );
		}
#endif
		(*iDelegate)->Destroy();
	}
}

//! This function should be called by the main loop of the application to process any
//! main thread invokes.
int ThreadPool::RunMainThread()
{
	m_MainQueueLock.lock();
	m_StopMain = false;

	while( !m_StopMain )
	{
		// block until something is pushed into our main queue..
		m_WakeMain.wait( m_MainQueueLock );
		m_MainQueueLock.unlock();

		// call with no lock on the main queue..
		ProcessMainThread();

		m_MainQueueLock.lock();
	};

	m_MainQueue.clear();
	m_MainQueueLock.unlock();

	return m_ExitCode;
}

void ThreadPool::StopMainThread(int a_ExitCode)
{
	m_StopMain = true;
	m_WakeMain.notify_one();

	m_ExitCode = a_ExitCode;
}


void ThreadPool::ThreadMain( void * arg )
{
	ThreadPool * pPool = (ThreadPool *)arg;

	pPool->m_ThreadQueueLock.lock();
	pPool->m_ActiveThreads += 1;
	while(! pPool->m_Shutdown )
	{
		pPool->m_WakeThread.wait( pPool->m_ThreadQueueLock );
		pPool->m_BusyThreads += 1;

		while( pPool->m_ThreadQueue.size() > 0 )
		{
			ICallback * pCallback = pPool->m_ThreadQueue.front();
			pPool->m_ThreadQueue.pop_front();
			pPool->m_ThreadQueueLock.unlock();		// unlock while we are invoking so other threads can work.
			
#if ENABLE_THREAD_TRY_CATCH
			try {
#endif
				pCallback->Invoke();
#if ENABLE_THREAD_TRY_CATCH
			}
			catch( WatsonException ex )
			{
				Log::Error( "ThreadPool", "Caught Exception: %s", ex.Message() );
			}
			catch( ... )
			{
				Log::Error( "ThreadPool", "Unhanded Exception on thread %u", 
					(unsigned int)tthread::this_thread::get_id().GetId() );
			}
#endif
			pCallback->Destroy();

			pPool->m_ThreadQueueLock.lock();
		}

		pPool->m_BusyThreads -= 1;
	}
	pPool->m_ActiveThreads -= 1;
	pPool->m_ThreadQueueLock.unlock();
}

void ThreadPool::InvokeOnThread(ICallback * a_pCallback)
{
		tthread::lock_guard <tthread::recursive_mutex> lock(m_ThreadQueueLock);
		m_ThreadQueue.push_back(a_pCallback);
		m_WakeThread.notify_one();
}

void ThreadPool::InvokeOnMain(ICallback * a_pCallback)
{
		tthread::lock_guard <tthread::recursive_mutex> lock(m_MainQueueLock);
		m_MainQueue.push_back(a_pCallback);
		m_WakeMain.notify_one();
}
