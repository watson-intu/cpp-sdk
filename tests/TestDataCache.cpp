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
#include "utils/DataCache.h"

class TestDataCache : UnitTest
{
public:
	//! Construction
	TestDataCache() : UnitTest("TestDataCache")
	{}

	virtual void RunTest()
	{
		DataCache cache;
		Test( cache.Initialize( "./cache/test/" ) );
		Test( cache.Save( "Test123", "Hello World" ) );

		DataCache cache2;
		Test( cache2.Initialize( "./cache/test/" ) );
		DataCache::CacheItem * pItem = cache2.Find( "Test123" );
		Test( pItem != NULL );
		Test( pItem->m_Data == "Hello World" );

		Test( cache2.FlushOldest() );
	}

};

TestDataCache TEST_DATA_CACHE;
