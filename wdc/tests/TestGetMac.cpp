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
#include "utils/GetMac.h"
#include "utils/Log.h"

class TestGetMac : UnitTest
{
public:
	//! Construction
	TestGetMac() : m_ZeroMac("00:00:00:00:00:00"), UnitTest("TestGetMac")
	{}

	virtual void RunTest()
	{
		std::string mac = GetMac::GetMyAddress();
		Log::Debug( "TestGetMac", "Mac: %s", mac.c_str() );
		Test( mac.size() > 0 );

		std::list<std::string> patterns;
		patterns.push_back("eth*");
		mac = GetMac::GetMyAddress(patterns);
		Log::Debug( "TestGetMac", "Mac: %s", mac.c_str() );
		Test( mac.size() > 0 );

		std::list<std::string> patternsToo;
		patternsToo.push_back("fake*");
		mac = GetMac::GetMyAddress(patternsToo);
		Log::Debug( "TestGetMac", "Mac: %s", mac.c_str() );
		Test( mac == m_ZeroMac );


		std::string ip = GetMac::GetIpAddress();
		Test( ip != "127.0.0.1" );
	}

private:

	//! Constants for testing the mac IDs
	const std::string m_ZeroMac;
};

TestGetMac TEST_GET_MAC;
