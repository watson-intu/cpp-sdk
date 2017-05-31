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


#ifndef WDC_JSON_HELPERS_H
#define WDC_JSON_HELPERS_H

#include <string>
#include "jsoncpp/json/json.h"
#include "UtilsLib.h"

class TiXmlElement;

//! This is a container for skill/gesture arguments.
class UTILS_API JsonHelpers
{
public:
	//! Load JSON from the given file
	static bool Load(const std::string & a_File, Json::Value & a_Json );
	//! Save JSON into the given file
	static bool Save(const std::string & a_File, const Json::Value & a_Json );
	//! Validate a given path, returns false if any member of the path doesn't exist.
	static bool ValidPath(const Json::Value & a_Json, const std::string & a_Path);
	//! Convert XML to JSON
	static void MakeJSON( const TiXmlElement * a_pElement, Json::Value & a_Value );
	//! Get a const reference to a Json::Value following the provided path. Returns a NULL json
	//! if the path isn't valid.
	static const Json::Value & Resolve(const Json::Value & a_Json, const std::string & a_Path);
	//! Resolve a full path to a value within this ParamsMap. If the value
	//! doesn't exist then it a null value will be created and returned. 
	static Json::Value & Resolve(Json::Value & a_Json, const std::string & a_Path);
	//! Merge one JSON into the other, if a_bReplace is true, then any duplicates are replaced, otherwise
	//! we will skip elements if they already exist
	static void Merge(Json::Value & a_MergeInto, const Json::Value & a_Merge, bool a_bReplace = true);
	//! Generate a hash of the provided Json.
	static std::string Hash(const Json::Value & a_Json, const char * a_pFilter = NULL );
	//! Filter the given json, removing any fields that match the given name.
	static void Filter( Json::Value & a_Json, const char * a_pFilter );
};

#endif

