/* ***************************************************************** */
/*                                                                   */
/* IBM Confidential                                                  */
/* OCO Source Materials                                              */
/*                                                                   */
/* (C) Copyright IBM Corp. 2001, 2014                                */
/*                                                                   */
/* The source code for this program is not published or otherwise    */
/* divested of its trade secrets, irrespective of what has been      */
/* deposited with the U.S. Copyright Office.                         */
/*                                                                   */
/* ***************************************************************** */

#include "JsonHelpers.h"
#include "utils/MD5.h"

//! What character is used a seperator for paths
#define PARAMS_PATH_SEPERATOR		'/'

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
		for (size_t i = 0; i < a_Merge.size(); ++i)
		{
			if (!a_bReplace && a_MergeInto.size() >= i)
				continue;
			Merge(a_MergeInto[i], a_Merge[i], a_bReplace);
		}

		// if a_bReplace is true, then remove array elements if needed..
		if (a_bReplace && a_MergeInto.size() > a_Merge.size())
			a_MergeInto.resize(a_Merge.size());
	}
	else
		a_MergeInto = a_Merge;
}

std::string JsonHelpers::Hash(const Json::Value & a_Json)
{
	return MD5<std::string>(Json::FastWriter().write( a_Json ) );
}
