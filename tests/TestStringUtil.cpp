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
#include "utils/StringUtil.h"

class TestStringUtil : UnitTest
{
public:
	//! Construction
	TestStringUtil() : UnitTest("TestStringUtil")
	{}

	virtual void RunTest()
	{
		std::string splitTest("Hello&World&How&are&you");

		std::vector<std::string> args;
		StringUtil::Split(splitTest, "&", args);
		Test(args.size() == 5);
	}

};

TestStringUtil TEST_STRING_UTIL;
