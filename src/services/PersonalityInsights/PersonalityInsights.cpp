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

#include "PersonalityInsights.h"

REG_SERIALIZABLE( PersonalityInsights );
RTTI_IMPL( Profile, ISerializable );
RTTI_IMPL( PersonalityInsights, IService );

PersonalityInsights::PersonalityInsights() : IService("PersonalityInsightsV1"), m_Version("2016-10-20")
{}

void PersonalityInsights::Serialize(Json::Value & json)
{
    IService::Serialize(json);
    json["m_Version"] = m_Version;
}

void PersonalityInsights::Deserialize(const Json::Value & json)
{
    IService::Deserialize(json);

    if(json.isMember("m_Version"))
        m_Version = json["m_Version"].asString();
}

//! IService interface
bool PersonalityInsights::Start()
{
    if (! IService::Start() )
        return false;

    if (! StringUtil::EndsWith( m_pConfig->m_URL, "personality-insights/api" ) )
    {
        Log::Error( "PersonalityInsights", "Configured URL not ended with personality-insights/api" );
        return false;
    }

    return true;
}

void PersonalityInsights::GetProfile( const std::string & a_Text, OnMessage a_Callback )
{
    Headers headers;
    headers["Content-Type"] = "text/plain";
    std::string params = "/v3/profile?version=" + m_Version;
    Json::Value req;
    req["text"] = a_Text;
    new RequestObj<Profile>( this, params, "POST", headers, req.toStyledString(),  a_Callback );
}

