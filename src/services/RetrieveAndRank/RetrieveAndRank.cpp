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

#include "RetrieveAndRank.h"

REG_SERIALIZABLE( RetrieveAndRank );
RTTI_IMPL( RetrieveAndRankResponse, ISerializable );
RTTI_IMPL( RetrieveAndRank, IService );

void RetrieveAndRank::Serialize(Json::Value & json)
{
    IService::Serialize(json);

    json["m_SolrId"] = m_SolrId;
    json["m_WorkspaceId"] = m_WorkspaceId;
}

void RetrieveAndRank::Deserialize(const Json::Value & json)
{
    IService::Deserialize(json);

    if (json.isMember("m_SolrId"))
        m_SolrId = json["m_SolrId"].asString();
    if (json.isMember("m_WorkspaceId"))
        m_WorkspaceId = json["m_WorkspaceId"].asString();
}

//! IService interface
bool RetrieveAndRank::Start()
{
    if (! IService::Start() )
        return false;

    if (! StringUtil::EndsWith( m_pConfig->m_URL, "retrieve-and-rank/api" ) )
    {
        Log::Error( "RetrieveAndRank", "Configured URL not ended with retrieve-and-rank/api" );
        return false;
    }

    return true;
}

void RetrieveAndRank::Select( const std::string & a_SolrId, const std::string & a_WorkspaceId, const std::string & a_Text,
                              OnMessage a_Callback )
{
    std::string params = "/v1/solr_clusters/" + a_SolrId + "/solr/" + a_WorkspaceId + "/select?q=" + a_Text + "&wt=json";
    new RequestObj<RetrieveAndRankResponse>( this, params, "GET", m_Headers, EMPTY_STRING,  a_Callback );
}

void RetrieveAndRank::Select( const std::string & a_Text, OnMessage a_Callback )
{
    std::string params = "/v1/solr_clusters/" + m_SolrId + "/solr/" + m_WorkspaceId + "/select?q=" + a_Text + "&wt=json";
    new RequestObj<RetrieveAndRankResponse>( this, params, "GET", m_Headers, EMPTY_STRING,  a_Callback );
}

void RetrieveAndRank::Ask(const std::string & a_SolrId, const std::string & a_WorkspaceId, const std::string & a_Text,
	const std::string & a_Source, OnMessage a_Callback)
{
	std::string params = "/v1/solr_clusters/" + a_SolrId + "/solr/" + a_WorkspaceId + "/fcselect?&q=" +
		StringUtil::UrlEscape(a_Text) + "&source=" + a_Source + "&wt=json";
	new RequestObj<RetrieveAndRankResponse>(this, params, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

