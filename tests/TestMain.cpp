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

#include <string.h>

#include "UnitTest.h"
#include "utils/Log.h"
#include "utils/Library.h"

#include <iostream>

int main( int argc, char ** argv )
{
	Log::RegisterReactor( new ConsoleReactor( LL_DEBUG_LOW ) );
	Log::RegisterReactor( new FileReactor( "UnitTest.log", LL_DEBUG_LOW ) );

	std::vector<std::string> tests;
	std::list<Library *> libs;
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'T':
				if ((i + 1) < argc)
				{
					tests.push_back(argv[i + 1]);
					i++;
					break;
				}
				printf("ERROR: -T is missing argument.\r\n");
				return 1;
			case 'L':
				if ((i + 1) < argc)
				{
					libs.push_back(new Library(argv[i + 1]));
					i++;
					break;
				}
				printf("ERROR: -L is missing argument.\r\n");
				return 1;
			default:
				std::cout << "Usage: unit_test [options] [test]\r\n"
					"-L <library> .. Load dynamic library\r\n"
					"-T <test> .. Run test\r\n";
				return 1;
			}
		}
	}

	return UnitTest::RunTests(tests);
}
