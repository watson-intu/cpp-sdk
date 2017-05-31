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
#include "utils/Factory.h"

namespace TestFactoryClasses {

	class Base : public IWidget
	{
	public:
		Base() 
		{}

		virtual int GetDepth()
		{
			return 0;
		}
	};

	typedef Factory<Base>	BaseFactory;

	static BaseFactory & GetFactory()
	{
		static BaseFactory factory;
		return factory;
	}

	REG_FACTORY(Base, GetFactory());
	
	class TestA : public Base
	{
	public:
		TestA() : m_C(2)
		{}

		int m_C;

		virtual int GetDepth()
		{
			return 1;
		}
	};

	REG_FACTORY(TestA, GetFactory());

	class TestB : public Base
	{
	public:
		TestB() : m_D(5)
		{}

		int m_D;

		virtual int GetDepth()
		{
			return 2;
		}
	};

	REG_FACTORY(TestB, GetFactory());

};

using namespace TestFactoryClasses;

class TestFactory : UnitTest
{
public:
	//! Construction
	TestFactory() : UnitTest("TestFactory")
	{}

	virtual void RunTest()
	{
		Base * pObject1 = GetFactory().CreateObject("TestB");
		Test(pObject1 != NULL);
		Test(pObject1->GetDepth() == 2);
		Base * pObject2 = GetFactory().CreateObject("TestA");
		Test(pObject2 != NULL);
		Test(pObject2->GetDepth() == 1);
		Base * pObject3 = GetFactory().CreateObject("Base");
		Test(pObject3 != NULL);
		Test(pObject3->GetDepth() == 0);

		delete pObject1;
		delete pObject2;
		delete pObject3;
		pObject1 = NULL;
		pObject2 = NULL;
		pObject3 = NULL;
	}

};

TestFactory TEST_FACTORY;


