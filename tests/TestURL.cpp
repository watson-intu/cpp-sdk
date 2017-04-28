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
#include "utils/URL.h"

class TestURL : UnitTest
{
public:
	//! Construction
	TestURL() : UnitTest("TestURL")
	{}

	virtual void RunTest()
	{
		URL url( "https://stream.watsonplatform.net/api" );
		Test( url.GetProtocol() == "https" );
		Test( url.GetPort() == 443 );
		Test( url.GetHost() == "stream.watsonplatform.net" );
		Test( url.GetEndPoint() == "api" );
		Test( url.GetURL() == "https://stream.watsonplatform.net/api" );

		URL url2("https://stream.watsonplatform.net:9080/api");
		Test(url2.GetProtocol() == "https");
		Test(url2.GetPort() == 9080);
		Test(url2.GetHost() == "stream.watsonplatform.net");
		Test(url2.GetEndPoint() == "api");
		Test(url2.GetURL() == "https://stream.watsonplatform.net:9080/api");

		URL url3("https://stream.watsonplatform.net:9080");
		Test(url3.GetProtocol() == "https");
		Test(url3.GetPort() == 9080);
		Test(url3.GetHost() == "stream.watsonplatform.net");
		Test(url3.GetEndPoint() == "");
		Test(url3.GetURL() == "https://stream.watsonplatform.net:9080");

		// test default protocol of http
		URL url4("stream.watsonplatform.net:9080");
		Test(url4.GetProtocol() == "http" );
		Test(url4.GetPort() == 9080);
		Test(url4.GetHost() == "stream.watsonplatform.net");
		Test(url4.GetEndPoint() == "");
		Test(url4.GetURL() == "http://stream.watsonplatform.net:9080");

		URL url5("stream.watsonplatform.net");
		Test(url5.GetProtocol() == "http");
		Test(url5.GetPort() == 80);
		Test(url5.GetHost() == "stream.watsonplatform.net");
		Test(url5.GetEndPoint() == "");
		Test(url5.GetURL() == "http://stream.watsonplatform.net");
	}

};

TestURL TEST_URL;
