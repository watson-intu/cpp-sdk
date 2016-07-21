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
        m_TestText("thank you kindly"),
        m_dIntentSize(0),
        m_fIntentConfidence(0.0f)
    {}

    bool m_bConversationTested;
    std::string m_WorkspaceId;
    std::string m_VersionId;
    std::string m_ConversationId;
    std::string m_TestText;
    std::string m_TopIntent;
    std::string m_TextResponse;
    int m_dIntentSize;
    float m_fIntentConfidence;


    virtual void RunTest() {
        Config config;
        Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);
        ThreadPool pool(1);

        Conversation conversation;
        Test(conversation.Start());
        Log::Debug("TestConversation","Conversation Started");

        Log::Debug("TestConversation","Testing Conversation Response for input: %s", m_TestText.c_str());
        conversation.Message(m_WorkspaceId, m_VersionId, m_TestText, DELEGATE(TestConversation, OnMessage, ConversationMessageResponse *, this));

        Spin(m_bConversationTested);
        Test(m_bConversationTested);

        //! Add in other Conversation API Endpoints provided by Brandon W.
    }

    void OnMessage(ConversationMessageResponse * a_pConversationResponse)
    {
        std::string m_Intent = a_pConversationResponse->m_TopIntent;
        Test(!m_Intent.empty());
        Log::Debug("TestConversation","Intent: %s", a_pConversationResponse->m_TopIntent.c_str());

        float m_fConfidence = a_pConversationResponse->m_fTopIntentConfidence;
        Test(m_fConfidence > 0.0);
        Log::Debug("TestConversation","Confidence: %f", a_pConversationResponse->m_fTopIntentConfidence);

        std::string m_Response = a_pConversationResponse->m_TextResponse;
        Test(!m_Response.empty());
        Log::Debug("TestConversation","Response: %s", a_pConversationResponse->m_TextResponse.c_str());

        m_ConversationId = a_pConversationResponse->m_ConversationId;
        Test(!m_ConversationId.empty());
        Log::Debug("TestConversation","ConversationId: %s", a_pConversationResponse->m_ConversationId.c_str());


        m_bConversationTested = true;
    }

};

TestConversation TEST_CONVERSATION;