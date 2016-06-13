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

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "UnitTest.h"
#include "utils/Delegate.h"

class TestDelegate : UnitTest
{
public:

	struct TestObject : public boost::enable_shared_from_this<TestObject>
	{
		typedef boost::shared_ptr<TestObject>		SP;
		typedef boost::weak_ptr<TestObject>			WP;

		TestObject() : m_InvokeCount( 0 )
		{}

		void TestFunction()
		{
			m_InvokeCount += 1;
		}

		int m_InvokeCount;
	};

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
		Test( nullDelegate( 42 ) );		// NOP

		TestObject::SP spObject( new TestObject() );
		VoidDelegate voidDelegate = VOID_DELEGATE( TestObject, TestFunction, spObject );
		Test( voidDelegate() );
		Test( spObject->m_InvokeCount == 1 );
		spObject.reset();
		Test(! voidDelegate() );			// test that we failed..
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
