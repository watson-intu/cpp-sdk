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

#include "ISerializable.h"

#include <fstream>
#include <streambuf>
#include <string>

RTTI_IMPL_BASE(ISerializable);


Factory<ISerializable> & ISerializable::GetSerializableFactory()
{
	static Factory<ISerializable> factory;
	return factory;
}

ISerializable * ISerializable::DeserializeObject(const std::string & a_json,
	ISerializable * a_pObject /*= NULL*/ )
{
	Json::Value root;
	Json::Reader reader( Json::Features::strictMode() );
	if (reader.parse(a_json, root))
		return DeserializeObject(root, a_pObject);

	Log::Error( "ISerializable", "Failed to parse json: %s", reader.getFormattedErrorMessages().c_str() );
	return NULL;
}

//! deserialize object from json 
ISerializable * ISerializable::DeserializeObject(const Json::Value & a_json, 
	ISerializable * a_pObject /*= NULL*/ )
{
	bool bCreated = false;
	if ( a_pObject == NULL )
	{
		std::string sClassName( a_json["Type_"].asString() );
		a_pObject = GetSerializableFactory().CreateObject(sClassName);
		if ( a_pObject == NULL )
			Log::Error( "ISerializable", "Failed to find factory for {%s}", sClassName.c_str() );
		else
			bCreated = true;
	}
	else if ( a_json["Type_"].isString() )
	{
		if ( a_pObject->GetRTTI().GetName() != a_json["Type_"].asString() )
			Log::Warning( "ISerializable", "Type mis-match for deserialize." );
	}

	if ( a_pObject != NULL )
	{
		a_pObject->Deserialize(a_json);
		return a_pObject;
	}

	return a_pObject;
}

Json::Value ISerializable::SerializeObject(ISerializable * a_pObject, bool a_bWriteType /*= true*/ )
{
	Json::Value json;
	if ( a_pObject != NULL )
	{
		if ( a_bWriteType )
		{
			// look to see if this type overridden a base type, if we find one, then use the overridden type name
			const RTTI * pType = &a_pObject->GetRTTI();
			const RTTI * pBaseType = pType;
			while( pBaseType != NULL )
			{
				if ( GetSerializableFactory().IsOverride( pBaseType->GetName() ) )
				{
					pType = pBaseType;
					break;
				}
				pBaseType = pBaseType->GetBaseClass();
			}

			if ( pType != NULL )
				json["Type_"] = pType->GetName();
		}

		a_pObject->Serialize(json);
	}

	return json;
}

ISerializable * ISerializable::DeserializeFromFile(const std::string & a_File, ISerializable * a_pObject /*= NULL*/ )
{
	std::ifstream input(a_File.c_str());
	if (input.is_open())
	{
		std::string json = std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
		return DeserializeObject(json, a_pObject);
	}

	return NULL;
}

bool ISerializable::SerializeToFile( const std::string & a_File, ISerializable * a_pObject, bool a_bWriteType /*= true*/, bool a_bFormatJson /*= false*/ )
{
	Json::Value json = SerializeObject(a_pObject, a_bWriteType);
	std::string json_string = a_bFormatJson ? Json::StyledWriter().write( json ) : Json::FastWriter().write( json );

	std::ofstream output(a_File.c_str());
	output << json_string;

	return true;
}

