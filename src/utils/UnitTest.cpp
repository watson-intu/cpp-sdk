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

int UnitTest::RunTests( const std::vector<std::string> & a_Tests )
{
	std::vector<std::string> tests( a_Tests );
	if ( tests.size() == 0 )
	{
		for( TestList::iterator iTest = GetTestList().begin(); iTest != GetTestList().end(); ++iTest )
			tests.push_back( (*iTest)->GetName() );
	}

	int executed = 0;
	int failed = 0;
	for(size_t i=0;i<tests.size();++i)
	{
		UnitTest * pRunTest = NULL;

#ifndef _DEBUG
		try {
#endif
			for( TestList::iterator iTest = GetTestList().begin(); pRunTest == NULL && iTest != GetTestList().end(); ++iTest )
			{
				if ( (*iTest)->GetName() == tests[i] )
					pRunTest = *iTest;
			}

			if ( pRunTest != NULL )
			{
				// TODO: replace printf with logging system..
				executed += 1;

				Log::Status( "UnitTest", "Running Test %s...", pRunTest->GetName().c_str() );
				pRunTest->RunTest();

				Log::Status( "UnitTest", "...Test %s COMPLETED.", pRunTest->GetName().c_str() );
			}
			else
			{
				Log::Error( "UnitTest", "Failed to find test %s...", a_Tests[i].c_str() );
				failed += 1;
			}
#ifndef _DEBUG
		}
		catch( const std::exception & e )
		{
			Log::Error( "UnitTest", "Caught Exception: %s", e.what() );
			failed += 1;
			Log::Error("UnitTest", "...Test %s FAILED.",  pRunTest->GetName().c_str());
		}
		catch(...)
		{
			failed += 1;
			Log::Error( "UnitTest", "...Test %s FAILED.", pRunTest->GetName().c_str() );
		}
#endif
	}

	Log::Status( "UnitTest", "Executed %d tests, %d tests failed.", executed, failed );

	return failed;
}

