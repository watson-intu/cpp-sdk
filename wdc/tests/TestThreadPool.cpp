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
#include "utils/Log.h"
#include "utils/ThreadPool.h"
#include "utils/Time.h"

class TestThreadPool : UnitTest
{
public:
	//! Construction
	TestThreadPool() : UnitTest("TestThreadPool")
	{}

	virtual void RunTest()
	{
		m_Pool = new ThreadPool();

		int index = 0;
		double startTime = Time().GetEpochTime();
		while( (Time().GetEpochTime() - startTime) < 5.0 )
		{
			m_Pool->ProcessMainThread();
			m_Pool->InvokeOnThread<int>( DELEGATE(TestThreadPool, ThreadInvoke, int, this), index++ );
		}

		delete m_Pool;
		m_Pool = NULL;
	}

	void ThreadInvoke(int v )
	{
		Log::Debug( "TestThreadPool", "Thread arg = %d", v );
		m_Pool->InvokeOnMain<int>( DELEGATE(TestThreadPool, MainInvoke, int, this), v );
	}

	void MainInvoke( int v )
	{
		Log::Debug( "TestThreadPool", "Main arg = %d", v );
	}

	ThreadPool * m_Pool;
};

TestThreadPool TEST_THREADPOOL;


