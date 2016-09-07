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

#include "ToneAnalyzer.h"

REG_SERIALIZABLE( ToneAnalyzer );
RTTI_IMPL( DocumentTones, ISerializable );
RTTI_IMPL( ToneAnalyzer, IService );

ToneAnalyzer::ToneAnalyzer() : IService("ToneAnalyzerV1"), m_Version("2016-05-19")
{}

void ToneAnalyzer::Serialize(Json::Value & json)
{
    IService::Serialize(json);
    json["m_Version"] = m_Version;
}

void ToneAnalyzer::Deserialize(const Json::Value & json)
{
    IService::Deserialize(json);

    if(json.isMember("m_Version"))
        m_Version = json["m_Version"].asString();
}

//! IService interface
bool ToneAnalyzer::Start()
{
    if (! IService::Start() )
        return false;

    if (! StringUtil::EndsWith( m_pConfig->m_URL, "tone-analyzer/api" ) )
    {
        Log::Error( "ToneAnalyzer", "Configured URL not ended with tone-analyzer/api" );
        return false;
    }

    return true;
}

void ToneAnalyzer::GetTone( const std::string & a_Text, OnMessage a_Callback )
{
    Headers headers;
    headers["Content-Type"] = "application/json";
    std::string params = "/v3/tone?version=" + m_Version;
    Json::Value req;
    req["text"] = a_Text;
    new RequestObj<DocumentTones>( this, params, "POST", headers, req.toStyledString(),  a_Callback );
}

