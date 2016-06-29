/**
* Copyright 2015 IBM Corp. All Rights Reserved.
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

#include "RelationshipExtraction.h"

REG_SERIALIZABLE( RelationshipExtraction );
RTTI_IMPL( RelationshipExtraction, IService );


RelationshipExtraction::RelationshipExtraction() :
	IService("RelationshipExtractionV1"),
	m_Language("ie-en-news")
{}

//! ISerializable
void RelationshipExtraction::Serialize(Json::Value & json)
{
	IService::Serialize(json);
}

void RelationshipExtraction::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
}

//! IService interface
bool RelationshipExtraction::Start()
{
	if (!IService::Start())
		return false;

	if (!StringUtil::EndsWith(m_pConfig->m_URL, "relationship-extraction-beta/api"))
	{
		Log::Error("RelationshipExtraction", "Configured URL not ended with relationship-extraction-beta/api");
		return false;
	}

	return true;
}

void RelationshipExtraction::GetServiceStatus(ServiceStatusCallback a_Callback)
{
	if (m_pConfig != NULL)
		new ServiceStatusChecker(this, a_Callback);
	else
		a_Callback(ServiceStatus(m_ServiceId, false));
}

bool RelationshipExtraction::Parse(const std::string & a_Text, OnParse a_Callback)
{
	std::string body = "txt=" + StringUtil::UrlEscape(a_Text);
	body += "&sid=" + m_Language;
	body += "&rt=json";

	Headers headers;
	headers["Content-Type"] = "application/x-www-form-urlencoded; charset=UTF-8";

	new RequestJson(this, "/v1/sire/0", "POST", headers, body, a_Callback,
		new CacheRequest(m_Language, StringHash::DJB(a_Text.c_str())));
	return true;
}


//! Creates an object responsible for service status checking
RelationshipExtraction::ServiceStatusChecker::ServiceStatusChecker(RelationshipExtraction * a_pService, ServiceStatusCallback a_Callback)
	: m_pService(a_pService), m_Callback(a_Callback)
{
	m_pService->Parse( "Hello World", DELEGATE(ServiceStatusChecker, OnCheckService,const Json::Value &, this));
}

//! Callback function invoked when service status is checked
void RelationshipExtraction::ServiceStatusChecker::OnCheckService(const Json::Value & a_Response )
{
	if (m_Callback.IsValid())
		m_Callback(ServiceStatus(m_pService->m_ServiceId, !a_Response.isNull() ));

	delete this;
}

bool RelationshipExtraction::SplitParseParts(std::string & a_Parse, std::vector<std::string> & a_Parts)
{
	while (true)
	{
		size_t nStart = a_Parse.find_first_of('[');
		if (nStart == std::string::npos)
			return a_Parts.size() > 0;

		size_t nEnd = nStart + 1;

		int count = 1;
		while (count > 0)
		{
			if (nEnd >= a_Parse.size())
				return a_Parts.size() > 0;

			char c = a_Parse[nEnd++];
			if (c == '[')
				count += 1;
			else if (c == ']')
				count -= 1;
		}

		std::string sInner(a_Parse.substr(nStart, nEnd - nStart));
		a_Parts.push_back(sInner);
		a_Parse.erase(nStart, nEnd - nStart);
	}
}

bool RelationshipExtraction::ParseIntoTree(const std::string & a_Parse, Json::Value & a_Tree)
{
	size_t nStart = a_Parse.find_first_not_of('[');
	if (nStart == std::string::npos)
		return false;

	size_t nTypeEnd = a_Parse.find_first_of(' ', nStart);
	if (nTypeEnd == std::string::npos)
		return false;

	std::string type = a_Parse.substr(nStart, nTypeEnd - nStart);
	a_Tree["type"] = type;

	size_t nMarkerSize = type.size() + 1;
	nStart = nTypeEnd;
	size_t nEnd = a_Parse.size() - nMarkerSize;

	std::string sInner(StringUtil::Trim(a_Parse.substr(nStart, nEnd - nStart)));
	//a_Tree["inner"] = sInner;

	Json::Value children;

	std::vector<std::string> parts;
	if (SplitParseParts(sInner, parts))
	{
		for (size_t i = 0; i<parts.size(); ++i)
			ParseIntoTree(parts[i], children[i]);
	}

	int wordIndex = 0;
	std::string lemma;

	nStart = 0;
	size_t nSeperator = sInner.find_first_of('_');
	while (nSeperator != std::string::npos)
	{
		size_t nEnd = sInner.find_first_of(' ', nSeperator);
		if (nEnd == std::string::npos)
			nEnd = sInner.size();
		std::string word(sInner.substr(nStart, nSeperator - nStart));
		std::string word_type(sInner.substr(nSeperator + 1, nEnd - (nSeperator + 1)));
		if (lemma.size() > 0)
			lemma += " ";
		lemma += word;

		a_Tree["words"][wordIndex]["word"] = StringUtil::Trim(word);
		a_Tree["words"][wordIndex++]["type"] = word_type;

		nStart = nEnd + 1;
		nSeperator = sInner.find_first_of('_', nStart);
	}

	if (lemma.size() > 0)
		a_Tree["lemma"] = StringUtil::Trim(lemma);
	if (!children.isNull())
		a_Tree["children"] = children;
	return true;
}
