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

#include <fstream>

#include "Conversation.h"
#include "utils/Form.h"

REG_SERIALIZABLE( Conversation );
RTTI_IMPL( ConversationMessageResponse, ISerializable );
RTTI_IMPL( Conversation, IService );

Conversation::Conversation() : IService("ConversationV1"), m_APIVersion( "2016-07-11" )
{}

void Conversation::Serialize(Json::Value & json)
{
    IService::Serialize(json);
	json["m_APIVersion"] = m_APIVersion;
}

void Conversation::Deserialize(const Json::Value & json)
{
    IService::Deserialize(json);
	if ( json.isMember("m_APIVersion") )
		m_APIVersion = json["m_APIVersion"].asString();
}

//! IService interface
bool Conversation::Start()
{
    if (! IService::Start() )
        return false;

    if (! StringUtil::EndsWith( m_pConfig->m_URL, "conversation/api" ) )
    {
        Log::Error( "Conversation", "Configured URL not ended with conversation/api" );
        return false;
    }

    return true;
}


//! Send Question / Statement / Command
void Conversation::Message( const std::string & a_WorkspaceId, const Json::Value & a_Context, const std::string & a_Text, OnMessage a_Callback )
{
    Headers headers;
    headers["Content-Type"] = "application/json";
    std::string params = "/v1/workspaces/" + a_WorkspaceId + "/message?version=" + m_APIVersion;

    Json::Value req;
    req["text"] = a_Text;
	Json::Value input;
	input["input"] = req;
    if( a_Context.isMember("conversation_id") )
        input["context"] = a_Context;

    new RequestObj<ConversationMessageResponse>( this, params, "POST", headers, input.toStyledString(),  a_Callback );
}

