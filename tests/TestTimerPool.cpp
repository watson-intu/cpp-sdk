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


#include "UnitTest.h"
#include "utils/Log.h"
#include "utils/TimerPool.h"
#include "utils/Time.h"

#include "boost/thread.hpp"

class TestTimerPool : UnitTest
{
public:
	//! Construction
	TestTimerPool() : UnitTest("TestTimerPool"), m_EndTest( false ), m_MainTimerTested( false ), m_RecurringCounts( 0 )
	{}

	virtual void RunTest()
	{
		m_ThreadPool = new ThreadPool();
		m_TimerPool = new TimerPool();

		TimerPool::ITimer::SP pMainTimer = m_TimerPool->StartTimer<int>( DELEGATE( TestTimerPool, MainInvoke, int, this ), 0, 2.0f, true, false );
		Test( pMainTimer != NULL );

		while(! m_MainTimerTested )
		{
			m_ThreadPool->ProcessMainThread();
			tthread::this_thread::sleep_for(tthread::chrono::milliseconds(10));
		}

		TimerPool::ITimer::SP pThreadTimer = m_TimerPool->StartTimer<int>( DELEGATE( TestTimerPool, ThreadInvoke, int, this), 0, 0.5, false, true );
		while( m_RecurringCounts < 20 )
		{
			m_ThreadPool->ProcessMainThread();
			tthread::this_thread::sleep_for(tthread::chrono::milliseconds(10));
		}

		delete m_ThreadPool;
		delete m_TimerPool;
		m_ThreadPool = NULL;
		m_TimerPool = NULL;
	}

	void ThreadInvoke(int v )
	{
		Log::Debug( "TestTimerPool", "Thread arg = %d", v );
		m_RecurringCounts += 1;
	}

	void MainInvoke( int v )
	{
		Log::Debug( "TestTimerPool", "Main arg = %d", v );
		m_MainTimerTested = true;
	}

	ThreadPool * m_ThreadPool;
	TimerPool * m_TimerPool;

	volatile bool m_EndTest;
	volatile bool m_MainTimerTested;
	int m_RecurringCounts;
};

TestTimerPool TEST_TIMERPOOL;


