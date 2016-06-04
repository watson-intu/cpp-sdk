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
#include "utils/Delegate.h"

class TestDelegate : UnitTest
{
public:
	//! Construction
	TestDelegate() : m_DelegateInvoked( false ), UnitTest( "TestDelegate" )
	{}

	virtual void RunTest()
	{
		//Delegate<int> d = Delegate<int>::Create<TestDelegate, &TestDelegate::Test >( this );
		Delegate<int> d = DELEGATE( TestDelegate, Test, int, this );
		d( 42 );
		Test( m_DelegateInvoked );

		Delegate<int> nullDelegate = Delegate<int>();
		nullDelegate( 42 );		// NOP
	}

	void Test( int a )
	{
		if ( a == 42 )
			m_DelegateInvoked = true;
	}

private:
	bool m_DelegateInvoked;
};

TestDelegate TEST_DELEGATE;
