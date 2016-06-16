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

#include "TimerPool.h"
#include "ThreadPool.h"
#include "../utils/Log.h"
#include "Time.h"

const double MIN_INTERVAL_TIME = 0.01;		// the minimum amount of time for a recurring timer

TimerPool * TimerPool::sm_pInstance = NULL;

TimerPool::TimerPool()
	: m_bShutdown( false ), m_pTimerThread( NULL )
{
	if ( sm_pInstance != NULL )
		Log::Error( "TimerPool", "Multiple instances of TimerPool created." );
	sm_pInstance = this;

	m_pTimerThread = new boost::thread( TimerThread, this );
}

TimerPool::~TimerPool()
{
	if ( sm_pInstance == this )
		sm_pInstance = NULL;

	m_TimerQueueLock.lock();
	m_bShutdown = true;
	m_TimerQueueLock.unlock();
	m_WakeTimer.notify_all();

	m_pTimerThread->join();
	delete m_pTimerThread;
}

bool TimerPool::StopTimer( ITimer::SP a_spTimer )
{
	boost::lock_guard<boost::mutex> lock( m_TimerQueueLock );
	if ( !a_spTimer )
		return false;

	// Cannot use built in erase(key_value)... would remove all timers with same next signal time
	for( TimerMultiSet::iterator iTimerStruct = m_TimerQueue.begin(); iTimerStruct != m_TimerQueue.end(); ++iTimerStruct )
	{
		if ( (iTimerStruct->m_pTimer).lock() == a_spTimer)
		{
			m_TimerQueue.erase( iTimerStruct );
			return true;
		}
	}

	return false;
}

void TimerPool::StopAllTimers()
{
	boost::lock_guard<boost::mutex> lock(m_TimerQueueLock);
	m_TimerQueue.clear();
}

void TimerPool::InsertTimer( ITimer::SP a_pTimer, bool a_bNewTimer )
{
	if (a_bNewTimer)
		m_TimerQueueLock.lock();

	// Insert new timer struct
	TimerMultiSet::iterator itInserted = m_TimerQueue.insert( TimerMultiSetStruct(a_pTimer) );
	bool bNewFirstTimer = itInserted == m_TimerQueue.begin();

	// new timer inserted, so wake our timer thread..
	if ( bNewFirstTimer && a_bNewTimer )
		m_WakeTimer.notify_one();
	if (a_bNewTimer)
		m_TimerQueueLock.unlock();
}


void TimerPool::InvokeTimer(ITimer::WP a_wpTimer)
{
	ITimer::SP spTimer = a_wpTimer.lock();
	if (spTimer)
		spTimer->Invoke();
}

void TimerPool::TimerThread( void * arg )
{
	TimerPool * pPool = (TimerPool *)arg;

	boost::unique_lock<boost::mutex> lock(pPool->m_TimerQueueLock);

	TimerMultiSet & timers = pPool->m_TimerQueue;
	while(! pPool->m_bShutdown )
	{
		if ( timers.begin() != timers.end() )
		{
			ITimer::SP spTimer = (timers.begin()->m_pTimer).lock();
			if (!spTimer)
			{
				// timer object destroyed, just remove it from the list..
				timers.erase(timers.begin());
				continue;
			}

			double sleepTime = spTimer->m_NextSignal.GetEpochTime() - Time().GetEpochTime();
			if (sleepTime > 0.0)
			{
				spTimer.reset();		// release the shared_ptr while sleeping..
				pPool->m_WakeTimer.timed_wait(lock, boost::posix_time::milliseconds( (int64_t)(sleepTime * 1000) ) );

				if (pPool->m_bShutdown)
					break;
				if (timers.begin() == timers.end())
					continue;

				spTimer = (timers.begin()->m_pTimer).lock();		// re-acquire the shared_ptr
				if (!spTimer)
					continue;
			}

			double now = Time().GetEpochTime();
			while( true )
			{
				if (timers.begin() == timers.end())
					break;
				spTimer = (timers.begin()->m_pTimer).lock();
				if (!spTimer)
				{
					timers.erase(timers.begin());
					continue;
				}
				if (now < spTimer->m_NextSignal.GetEpochTime())
					break;				// not time yet, stop enumerating timers..

				// remove and invoke the timer..
				timers.erase(timers.begin());

				if (spTimer->m_InvokeOnMain)
					ThreadPool::Instance()->InvokeOnMain<ITimer::WP>(DELEGATE(TimerPool, InvokeTimer, ITimer::WP, pPool ), spTimer);
				else
					ThreadPool::Instance()->InvokeOnThread<ITimer::WP>(DELEGATE(TimerPool, InvokeTimer, ITimer::WP, pPool ), spTimer);

				if (spTimer->m_Recurring)
				{
					double fInterval = spTimer->m_Interval;
					if ( fInterval < MIN_INTERVAL_TIME )
						fInterval = MIN_INTERVAL_TIME;		

					spTimer->m_NextSignal = spTimer->m_NextSignal.GetEpochTime() + fInterval;
					pPool->InsertTimer(spTimer, false);
				}
			}
		}
		else
		{
			pPool->m_WakeTimer.timed_wait( lock, boost::posix_time::milliseconds(1000));
		}
	}

	lock.unlock();
}
