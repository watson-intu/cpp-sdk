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
#include "utils/Path.h"

class TestPath : UnitTest
{
public:
	//! Construction
	TestPath() : UnitTest("TestPath")
	{}

	virtual void RunTest()
	{
		Path p( "C:/work/file.txt" );
		Test( p.GetDirectory() == "C:/work/" );
		Test( p.GetFile() == "file" );
		Test( p.GetExtension() == ".txt" );

		p.Parse( "file.txt" );
		Test( p.GetDirectory() == "" );
		Test( p.GetFile() == "file" );
		Test( p.GetExtension() == ".txt" );

		p.Parse( "file" );
		Test( p.GetDirectory() == "" );
		Test( p.GetFile() == "file" );
		Test( p.GetExtension() == "" );

		p.Parse( ".txt" );
		Test( p.GetDirectory() == "" );
		Test( p.GetFile() == "" );
		Test( p.GetExtension() == ".txt" );
	}

};

TestPath TEST_PATH;
