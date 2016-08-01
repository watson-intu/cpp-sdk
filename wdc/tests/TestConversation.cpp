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

#include "tests/UnitTest.h"
#include "utils/Log.h"
#include "utils/Time.h"
#include "utils/Config.h"
#include "utils/UniqueID.h"
#include "services/Conversation/Conversation.h"

class TestConversation : UnitTest {
public:
    //! Construction
    TestConversation() : UnitTest("TestConversation"),
        m_bConversationTested(false),
        m_WorkspaceId("e8e888b1-5141-4747-9bbb-5a5440bae96a"),
        m_ConversationId(""),
        m_VersionId("2016-07-11"),
        m_TestText("please raise your right hand")
    {}

    bool m_bConversationTested;
    std::string m_WorkspaceId;
    std::string m_VersionId;
    std::string m_ConversationId;
    std::string m_TestText;
    Json::Value m_Context;

    virtual void RunTest() {
        Config config;
        Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);
        ThreadPool pool(1);

        Conversation conversation;
        Test(conversation.Start());
        Log::Debug("TestConversation","Conversation Started");

        Log::Debug("TestConversation","Testing Conversation Response for input: %s", m_TestText.c_str());
        conversation.Message(m_WorkspaceId, m_Context, m_VersionId, m_TestText, DELEGATE(TestConversation, OnMessage, ConversationMessageResponse *, this));

        Spin(m_bConversationTested);
        Test(m_bConversationTested);

        // TODO Add in other Conversation API Endpoints provided by Brandon W.
    }

    void OnMessage(ConversationMessageResponse * a_pConversationResponse)
    {
        // Test that an Intent is returned
        std::vector<ConversationIntent> m_Intents = a_pConversationResponse->m_Intents;
        Test(m_Intents.size() > 0);
        std::string m_TopIntent = m_Intents[0].m_Intent;
        Test(!m_TopIntent.empty());
        Log::Debug("TestConversation","Intent: %s", m_TopIntent.c_str());

        // Test that the confidence of the top intent is greater than 0
        float m_fConfidence = m_Intents[0].m_fConfidence;
        Test(m_fConfidence > 0.0);
        Log::Debug("TestConversation","Confidence: %f", m_fConfidence);

        // Test Entity Extraction
        std::vector<ConversationEntities> m_Entities = a_pConversationResponse->m_Entities;
        Test(m_Entities.size() > 0);
        std::string m_Entity = m_Entities[0].m_Entity;
        std::string m_EntityValue = m_Entities[0].m_Value;
        Test(!m_Entity.empty() && !m_EntityValue.empty());
        Log::Debug("TestConversation","Entity: %s, Value: %s",m_Entity.c_str(), m_EntityValue.c_str());

        // Test that a response is returned
        std::vector<std::string> m_Output = a_pConversationResponse->m_Output;
        Test(m_Output.size() > 0);
        std::string m_TextResponse = m_Output[0];
        Test(!m_TextResponse.empty());
        Log::Debug("TestConversation","Response: %s", m_TextResponse.c_str());

        // Test that the Conversation Id is set in Conversation's Context
        m_Context = a_pConversationResponse->m_Context;
        Test(m_Context.isMember("conversation_id"));


        m_bConversationTested = true;
    }

};

TestConversation TEST_CONVERSATION;