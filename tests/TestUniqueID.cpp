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
#include "utils/UniqueID.h"

class TestUniqueID : UnitTest
{
public:
	//! Construction
	TestUniqueID() : UnitTest("TestUniqueID")
	{}

	virtual void RunTest()
	{
		UniqueID id1;
		Test( id1.Get().size() == 36 );

		UniqueID id2;
		Test( id2.Get().size() == 36 );

		// test that the IDs are unique
		Test( id1.Get() != id2.Get() );			

		// test encoding & decoding of IDs to binary
		UniqueID id4( id2 );
		UniqueID id3( id4.Encode() );
		id3.Decode();

		Test( id3.Get() == id2.Get() );
	}

};

TestUniqueID TEST_UNIQUEID;
