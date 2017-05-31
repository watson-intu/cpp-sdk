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

int UnitTest::RunTests( const TestMap & a_Tests )
{
	TestMap tests( a_Tests );
	if ( tests.size() == 0 )
	{
		for( TestList::iterator iTest = GetTestList().begin(); iTest != GetTestList().end(); ++iTest )
			tests[ (*iTest)->GetName() ] = Args();
	}

	int executed = 0;
	std::vector<std::string> failed;

	for( TestMap::const_iterator iTest = tests.begin(); iTest != tests.end(); ++iTest )
	{
		UnitTest * pRunTest = NULL;

#ifndef _DEBUG
		try {
#endif
			for( TestList::iterator iFind = GetTestList().begin();
				pRunTest == NULL && iFind != GetTestList().end(); ++iFind )
			{
				if ( (*iFind)->GetName() == iTest->first )
					pRunTest = *iFind;
			}

			if ( pRunTest != NULL )
			{
				// TODO: replace printf with logging system..
				executed += 1;

				Log::Status( "UnitTest", "Running Test %s...", pRunTest->GetName().c_str() );

				pRunTest->SetArgs( iTest->second );
				pRunTest->RunTest();

				Log::Status( "UnitTest", "...Test %s COMPLETED.", pRunTest->GetName().c_str() );
			}
			else
			{
				Log::Error( "UnitTest", "Failed to find test %s...", iTest->first.c_str() );
				failed.push_back( iTest->first );
			}
#ifndef _DEBUG
		}
		catch( const std::exception & e )
		{
			Log::Error( "UnitTest", "Caught Exception: %s", e.what() );
			failed.push_back( pRunTest->GetName() );
			Log::Error("UnitTest", "...Test %s FAILED.",  pRunTest->GetName().c_str());
		}
		catch(...)
		{
			failed.push_back( pRunTest->GetName() );
			Log::Error( "UnitTest", "...Test %s FAILED.", pRunTest->GetName().c_str() );
		}
#endif
	}

	Log::Status( "UnitTest", "Executed %d tests, %u tests failed.", executed, failed.size() );
	for(size_t i=0;i<failed.size();++i)
		Log::Error( "UnitTest", "... %s failed.", failed[i].c_str() );

	return failed.size();
}

