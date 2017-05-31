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


#include "JsonHelpers.h"
#include "utils/MD5.h"
#include "utils/Log.h"
#include "tinyxml/tinyxml.h"

#include <fstream>

//! What character is used a seperator for paths
#define PARAMS_PATH_SEPERATOR		'/'

bool JsonHelpers::Load(const std::string & a_File, Json::Value & a_Json )
{
	try {
		std::ifstream input( a_File.c_str() );
		if (!input.is_open())
		{
			Log::Error("JsonHelpers::Load", "Failed to open %s", a_File.c_str() );
			return false;
		}

		Json::Reader reader(Json::Features::strictMode());
		if (!reader.parse(input, a_Json))
		{
			Log::Error("JsonHelpers::Load", "Failed to parse %s: %s", 
				a_File.c_str(), reader.getFormattedErrorMessages().c_str() );
			return false;
		}
		input.close();
		return true;
	}
	catch( const std::exception & ex )
	{
		Log::Error("JsonHelpers::Load", "Caught exception loading %s: %s", a_File.c_str(), ex.what() );
	}
	return false;
}

//! Save JSON into the given file
bool JsonHelpers::Save(const std::string & a_File, const Json::Value & a_Json )
{
	try {
		std::string json_string( Json::StyledWriter().write(a_Json) );
		std::ofstream output( a_File.c_str() );
		output << json_string;
		return true;
	}
	catch( const std::exception & ex )
	{
		Log::Error("JsonHelpers::Save", "Caught exception saving %s: %s", a_File.c_str(), ex.what() );
	}
	return false;
}

//! Validate a given path, returns false if any member of the path doesn't exist.
bool JsonHelpers::ValidPath(const Json::Value & a_Json, const std::string & a_Path)
{
	const Json::Value * root = &a_Json;

	size_t start = 0;
	size_t seperator = a_Path.find_first_of(PARAMS_PATH_SEPERATOR);
	while (seperator != std::string::npos)
	{
		std::string key(a_Path.substr(start, seperator - start));
		if (root->isArray())
		{
			size_t index = atoi(key.c_str());
			if (index >= root->size())
				return false;
			root = &(*root)[index];
		}
		else if (root->isObject())
		{
			if (!root->isMember(key))
				return false;
			root = &(*root)[key];
		}
		else
		{
			return false;
		}

		start = seperator + 1;
		seperator = a_Path.find_first_of(PARAMS_PATH_SEPERATOR, start);
	}

	std::string key(start > 0 ? a_Path.substr(start) : a_Path);
	if (root->isArray())
	{
		size_t index = atoi(key.c_str());
		if (index >= root->size())
			return false;
		return true;
	}
	else if (root->isObject())
	{
		if (!root->isMember(key))
			return false;
		return true;
	}

	return false;
}

void JsonHelpers::MakeJSON( const TiXmlElement * a_pElement, Json::Value & a_Value )
{
	const TiXmlAttribute * pAttribute = a_pElement->FirstAttribute();
	while (pAttribute != NULL)
	{
		const std::string & key = pAttribute->NameTStr();
		const std::string & value = pAttribute->ValueStr();
		if (key[0] != 0)
			a_Value[key] = value;
		pAttribute = pAttribute->Next();
	}

	const char * pText = a_pElement->GetText();
	if (pText != NULL)
		a_Value["_text"] = pText;

	std::map < std::string, int > indexes;

	const TiXmlElement * pChild = a_pElement->FirstChildElement();
	while (pChild != NULL)
	{
		const std::string & elementName = pChild->ValueStr();
		MakeJSON(pChild, a_Value[elementName][indexes[elementName]++]);
		pChild = pChild->NextSiblingElement();
	}
}

//! Get a const reference to a Json::Value following the provided path. Returns a NULL json
//! if the path isn't valid.
const Json::Value & JsonHelpers::Resolve(const Json::Value & a_Json, const std::string & a_Path)
{
	static Json::Value NULL_VALUE;
	const Json::Value * result = &a_Json;

	size_t start = 0;
	size_t seperator = a_Path.find_first_of(PARAMS_PATH_SEPERATOR);
	while (seperator != std::string::npos)
	{
		std::string key(a_Path.substr(start, seperator - start));
		if (result->isArray())
		{
			size_t index = atoi(key.c_str());
			if (index >= result->size())
				return NULL_VALUE;
			result = &(*result)[atoi(key.c_str())];
		}
		else if (result->isObject())
		{
			if (!result->isMember(key))
				return NULL_VALUE;
			result = &(*result)[key];
		}
		else
			return NULL_VALUE;

		start = seperator + 1;
		seperator = a_Path.find_first_of(PARAMS_PATH_SEPERATOR, start);
	}

	std::string key(start > 0 ? a_Path.substr(start) : a_Path);
	if (result->isArray())
	{
		size_t index = atoi(key.c_str());
		if (index >= result->size())
			return NULL_VALUE;
		return (*result)[index];
	}
	else if (result->isObject())
	{
		if (!result->isMember(key))
			return NULL_VALUE;
		return (*result)[key];
	}

	return NULL_VALUE;
}

//! Resolve a full path to a value within this ParamsMap. If the value
//! doesn't exist then it a null value will be created and returned. 
Json::Value & JsonHelpers::Resolve(Json::Value & a_Json, const std::string & a_Path)
{
	Json::Value * result = &a_Json;

	size_t start = 0;
	size_t seperator = a_Path.find_first_of(PARAMS_PATH_SEPERATOR);
	while (seperator != std::string::npos)
	{
		std::string key(a_Path.substr(start, seperator - start));
		if (result->isArray())
			result = &(*result)[atoi(key.c_str())];
		else
			result = &(*result)[key];

		start = seperator + 1;
		seperator = a_Path.find_first_of(PARAMS_PATH_SEPERATOR, start);
	}

	std::string key(start > 0 ? a_Path.substr(start) : a_Path);
	if (result->isArray())
	{
		size_t index = atoi(key.c_str());
		return (*result)[index];
	}

	return (*result)[key];
}

//! Merge one JSON into the other, if a_bReplace is true, then any duplicates are replaced, otherwise
//! we will skip elements if they already exist
void JsonHelpers::Merge(Json::Value & a_MergeInto, const Json::Value & a_Merge, bool a_bReplace /*= true*/)
{
	if (a_Merge.isObject())
	{
		for (Json::ValueConstIterator iElement = a_Merge.begin(); iElement != a_Merge.end(); ++iElement)
		{
			const std::string & key = iElement.key().asString();
			const Json::Value & value = *iElement;
			if (a_bReplace || !a_MergeInto.isMember(key) || value.isArray() || value.isObject())
				Merge(a_MergeInto[key], value, a_bReplace);
		}
	}
	else if (a_Merge.isArray())
	{
		if ( a_MergeInto.isArray() )
		{
			// append our array onto the existing array
			for(size_t i=0;i<a_Merge.size();++i)
				Merge( a_MergeInto[ a_MergeInto.size() ], a_Merge[i], a_bReplace );
		}
		else
			a_MergeInto = a_Merge;
	}
	else if(! a_Merge.isNull() )
		a_MergeInto = a_Merge;
}

std::string JsonHelpers::Hash(const Json::Value & a_Json, const char * a_pFilter /*= NULL*/ )
{
	if ( a_pFilter != NULL )
	{
		Json::Value filtered( a_Json );
		Filter( filtered, a_pFilter );

		return MakeMD5(Json::FastWriter().write( filtered ) );
	}

	return MakeMD5(Json::FastWriter().write( a_Json ) );
}

void JsonHelpers::Filter( Json::Value & a_Json, const char * a_pFilter )
{
	if ( a_Json.isObject() )
	{
		a_Json.removeMember( a_pFilter );

		for (Json::ValueIterator iElement = a_Json.begin(); iElement != a_Json.end(); ++iElement)
			Filter( *iElement, a_pFilter );
	}
	else if (a_Json.isArray())
	{
		for(size_t i=0;i<a_Json.size();++i)
			Filter( a_Json[ i ], a_pFilter );
	}
}

