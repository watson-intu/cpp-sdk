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

#include "Alchemy.h"

REG_SERIALIZABLE( Alchemy );
RTTI_IMPL( Alchemy, IService );


Alchemy::Alchemy() : IService("AlchemyV1")
{}

//! ISerializable
void Alchemy::Serialize(Json::Value & json)
{
	IService::Serialize(json);
	SerializeVector("m_ReturnParameters", m_ReturnParameters, json);
}

void Alchemy::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
	DeserializeVector("m_ReturnParameters", json, m_ReturnParameters);

	if (m_ReturnParameters.size() == 0)
	{
		m_ReturnParameters.push_back("enriched.url.title");
		m_ReturnParameters.push_back("enriched.url.url");
		m_ReturnParameters.push_back("enriched.url.text");
	}
}

//! IService interface
bool Alchemy::Start()
{
	if (!IService::Start())
		return false;

	if (!StringUtil::EndsWith(m_pConfig->m_URL, "calls"))
	{
		Log::Error("Alchemy", "Configured URL not ended with calls");
		return false;
	}
	if (m_pConfig->m_User.size() == 0)
	{
		Log::Error("Alchemy", "API-Key expected in user field.");
		return false;
	}

	return true;
}

void Alchemy::GetServiceStatus(ServiceStatusCallback a_Callback)
{
	a_Callback(ServiceStatus(m_ServiceId, true));
}

void Alchemy::GetChunkTags(const std::string & a_Text,
	Delegate<const Json::Value &> a_Callback )
{
	std::string parameters = "/text/TextGetChunkTags";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&outputMode=json";
	parameters += "&text=" + StringUtil::UrlEscape( a_Text );

	new RequestJson(this, parameters, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback,
		new CacheRequest( "GetChunkTags", StringHash::DJB(a_Text.c_str()) ) );
}

void Alchemy::GetPosTags(const std::string & a_Text,
	Delegate<const Json::Value &> a_Callback )
{
	std::string parameters = "/text/TextGetPOSTags";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&outputMode=json";
	parameters += "&text=" + StringUtil::UrlEscape( a_Text );

	new RequestJson(this, parameters, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback, 
		new CacheRequest( "GetPosTags", StringHash::DJB(a_Text.c_str()) ) );
}

void Alchemy::GetEntities(const std::string & a_Text, Delegate<const Json::Value &> a_Callback)
{
	std::string parameters = "/text/TextGetRankedNamedEntities";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&outputMode=json";
	parameters += "&text=" + StringUtil::UrlEscape( a_Text );

	new RequestJson(this, parameters, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback,
		new CacheRequest( "TextGetRankedNamedEntities", StringHash::DJB(a_Text.c_str()) ) );
}

void Alchemy::GetNews(const std::string & a_Subject, time_t a_StartDate, time_t a_EndDate, int a_NumberOfArticles,
	Delegate<const Json::Value &> a_Callback)
{
	std::string searchCriteria;
	for (int i = 0; i < m_ReturnParameters.size(); ++i)
		searchCriteria += m_ReturnParameters[i] + ",";
	
	searchCriteria = searchCriteria.substr(0, searchCriteria.size() - 1);

	std::string parameters = "/data/GetNews";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&return=" + searchCriteria;
	parameters += "&start=" + StringUtil::Format("%u", a_StartDate);
	parameters += "&end=" + StringUtil::Format("%u", a_EndDate);
	parameters += "&q.enriched.url.enrichedTitle.entities.entity=|";
	parameters += "text=" + StringUtil::UrlEscape(a_Subject);
	parameters += ",type=company|&count=" + StringUtil::Format("%d", a_NumberOfArticles);
	parameters += "&outputMode=json";

	new RequestJson(this, parameters, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback);
}

