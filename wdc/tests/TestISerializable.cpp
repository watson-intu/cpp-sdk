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

#include <string>

#include "UnitTest.h"
#include "utils/RTTI.h"
#include "utils/ISerializable.h"

namespace TestSerializableClasses {

class SubObject : public ISerializable
{
public:
	RTTI_DECL();

	SubObject() : 
		m_integer( 42 )
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json)
	{
		json["m_integer"] = m_integer;
	}

	virtual void Deserialize(const Json::Value & json)
	{
		m_integer = json["m_integer"].asInt();
	}


	//! Data
	int m_integer;
};

REG_SERIALIZABLE( SubObject );
RTTI_IMPL( SubObject, ISerializable );


class TestA : public ISerializable
{
public:
	RTTI_DECL();

	TestA() : 
		m_integer( 42 ),
		m_string("Hello World"),
		m_pSubObject( new SubObject() )
	{
		for(int i=0;i<3;++i)
			m_ObjectList.push_back( new SubObject() );
	}
	virtual ~TestA()
	{
		delete m_pSubObject;
		m_pSubObject = NULL;
		ClearObjectList();
	}

	void ClearObjectList()
	{
		for( ObjectList::iterator iObj = m_ObjectList.begin();
		iObj != m_ObjectList.end(); ++iObj )
		{
			delete *iObj;
			*iObj = NULL;
		}
		m_ObjectList.clear();
	}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json)
	{
		json["m_integer"] = m_integer;
		json["m_string"] = m_string;
		json["m_pSubObject"] = ISerializable::SerializeObject( m_pSubObject );

		int index = 0;
		for( ObjectList::iterator iObj = m_ObjectList.begin();
			iObj != m_ObjectList.end(); ++iObj )
		{
			json["m_ObjectList"][index++] = ISerializable::SerializeObject( *iObj );
		}

	}

	virtual void Deserialize(const Json::Value & json)
	{
		ClearObjectList();

		m_integer = json["m_integer"].asInt();
		m_string = json["m_string"].asString();
		ISerializable::DeserializeObject( json["m_pSubObject"], m_pSubObject );

		for( Json::ValueConstIterator iObject = json["m_ObjectList"].begin();
			iObject != json["m_ObjectList"].end(); ++iObject )
		{
			m_ObjectList.push_back( DynamicCast<SubObject>( ISerializable::DeserializeObject( *iObject ) ) );
		}
	}

	//! Types
	typedef std::list<SubObject *> ObjectList;

	//! Data
	int m_integer;
	std::string m_string;
	SubObject * m_pSubObject;
	ObjectList m_ObjectList;
};

REG_SERIALIZABLE( TestA );
RTTI_IMPL( TestA, ISerializable );


};

using namespace TestSerializableClasses;

class TestISerializable : UnitTest
{
public:
	//! Construction
	TestISerializable() : UnitTest("TestISerializable")
	{}

	virtual void RunTest()
	{
		TestA * pTestObject = new TestA();
		pTestObject->m_integer = 55;
		pTestObject->m_pSubObject->m_integer = 843;

		Test( ISerializable::SerializeToFile( "test.json", pTestObject ) );
		
		TestA * pTestObject1 = DynamicCast<TestA>( ISerializable::DeserializeFromFile( "test.json" ) );
		Test( pTestObject1 != NULL );
		Test( pTestObject->m_integer == pTestObject1->m_integer );
		Test( pTestObject->m_string == pTestObject1->m_string );
		Test( pTestObject->m_pSubObject != NULL );
		Test( pTestObject->m_pSubObject->m_integer == pTestObject1->m_pSubObject->m_integer );

		Test( ISerializable::DeserializeFromFile( "nofile.json" ) == NULL );
	}

};


TestISerializable TEST_ISERIALIZABLE;


