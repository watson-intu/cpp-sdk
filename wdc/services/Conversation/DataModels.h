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

#ifndef WDC_CONVERSATION_MODELS_H
#define WDC_CONVERSATION_MODELS_H

#include "utils/ISerializable.h"

struct Convo : public ISerializable
{
    RTTI_DECL();

    std::string m_Name;
    std::string m_WorkspaceId;
    std::string m_ConversationId;
    std::string m_URL;
    std::string m_Version;


    virtual void Serialize(Json::Value & json)
    {
        json["name"] = m_Name;
        json["workspace_id"] = m_WorkspaceId;
        json["url"] = m_URL;
        json["version"] = m_Version;
        json["conversation_id"] = m_ConversationId;
    }

    virtual void Deserialize(const Json::Value & json)
    {
        if ( json.isMember("name") )
            m_Name = json["name"].asString();
        if ( json.isMember("workspace_id") )
            m_WorkspaceId = json["workspace_id"].asString();
        if( json.isMember("conversation_id") )
            m_ConversationId = json["conversation_id"].asString();
        if( json.isMember("url") )
            m_URL = json["url"].asString();
        if( json.isMember("version"))
            m_Version = json["version"].asString();

    }
};


struct ConversationMessageResponse : public ISerializable
{
    RTTI_DECL();

    Json::Value m_Intents;
    Json::Value m_IntentJson;
    Json::Value m_Output;
    Json::Value m_Context;

    std::string m_ConversationId;
    std::string m_TextResponse;
    std::string m_TopIntent;
    float m_fTopIntentConfidence;

    virtual void Serialize(Json::Value & json)
    {
        json["output"]["text"][0] = m_TextResponse;
        json["context"]["conversation_id"] = m_ConversationId;

        m_Intents["intent"] = m_TopIntent;
        m_Intents["confidence"] = m_fTopIntentConfidence;
        json["intents"][0] = m_Intents;
    }

    virtual void Deserialize(const Json::Value & json)
    {
        if( json.isMember("intents"))
            m_Intents = json["intents"];
        if( m_Intents.size() > 0)
            m_IntentJson = m_Intents[0];
        if( m_IntentJson.isMember("intent") )
            m_TopIntent = m_IntentJson["intent"].asString();
        if( m_IntentJson.isMember("confidence") )
            m_fTopIntentConfidence = m_IntentJson["confidence"].asFloat();
        if( json.isMember("output") )
            m_Output = json["output"];
        if( m_Output.isMember("text") && m_Output["text"].size() > 0 )
            m_TextResponse = m_Output["text"][0].asString();
        if( json.isMember("context") )
            m_Context = json["context"];
        if( m_Context.isMember("conversation_id") )
            m_ConversationId = m_Context["conversation_id"].asString();

    }
};

#endif
