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
	int executed = 0;
	int failed = 0;
	for( TestList::iterator iTest = GetTestList().begin(); iTest != GetTestList().end(); ++iTest )
	{
#ifndef _DEBUG
		try {
#endif
			if (a_Tests.size() > 0)
			{
				bool bFound = false;
				for (size_t i = 0; i < a_Tests.size() && !bFound; ++i)
					bFound = (*iTest)->GetName().find(a_Tests[i]) != std::string::npos;
				if (!bFound)
					continue;
			}

			// TODO: replace printf with logging system..
			executed += 1;

			Log::Status( "UnitTest", "Running Test %s...", (*iTest)->GetName().c_str() );
			(*iTest)->RunTest();

			Log::Status( "UnitTest", "...Test %s COMPLETED.", (*iTest)->GetName().c_str() );
#ifndef _DEBUG
		}
		catch( const std::exception & e )
		{
			Log::Error( "UnitTest", "Caught Exception: %s", e.what() );
			failed += 1;
			Log::Error("UnitTest", "...Test %s FAILED.", (*iTest)->GetName().c_str());
		}
		catch(...)
		{
			failed += 1;
			Log::Error( "UnitTest", "...Test %s FAILED.", (*iTest)->GetName().c_str() );
		}
#endif
	}

	return failed;
}

