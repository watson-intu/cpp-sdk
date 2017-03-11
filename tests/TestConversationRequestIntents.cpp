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
#include <iostream>
#include "utils/UnitTest.h"
#include "utils/Log.h"
#include "utils/Time.h"
#include "utils/Config.h"
#include "utils/UniqueID.h"
#include "services/Conversation/Conversation.h"

// Calls Conversation on an input file of request intents and outputs a json with entity mappings

class TestConversationRequestIntents : UnitTest {
public:
    //! Construction
    TestConversationRequestIntents() : UnitTest("TestConversationRequestIntents"),
        m_bConversationTested(false),
        m_WorkspaceId("f47fd8b3-e6d6-46d3-b921-8cc257660147"),
        m_ConversationId(""),
        m_IntentOverrideTag("m_IntentOverride"),
        m_ConversationInput("./etc/shared/self_conversation_v1.csv"),
        m_ConversationOutput("./etc/shared/self_requests.json")
    {}

    bool m_bConversationTested;
    std::string m_WorkspaceId;
    std::string m_ConversationId;
    std::string m_IntentOverrideTag;
	std::string m_ConversationInput;
	std::string m_ConversationOutput;
	std::string m_RequestName;
    Json::Value m_Context;
	Json::Value m_ConversationResults;

    virtual void RunTest() 
	{
        Config config;
        Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

		ThreadPool pool(1);

        Conversation conversation;
        if ( config.IsConfigured( conversation.GetServiceId() ) )
		{
			Test( conversation.Start() );
			Log::Debug("TestConversationRequestIntents","Conversation Started");

			std::ifstream input(m_ConversationInput.c_str());
			for( std::string line; getline( input, line ); )
			{
				m_bConversationTested = false;

				std::vector<std::string> parts;
				StringUtil::Split( line, ",", parts );

				m_RequestName = parts[1];
				conversation.Message(m_WorkspaceId, m_Context, parts[0], m_IntentOverrideTag,
				                     DELEGATE(TestConversationRequestIntents, OnMessage, ConversationResponse *, this));
				Spin(m_bConversationTested);
				Test(m_bConversationTested);
			}

			input.close();
			std::ofstream output(m_ConversationOutput.c_str());
			output << m_ConversationResults.toStyledString();
			output.close();

			Log::Debug("TestConversationRequestIntents", m_ConversationResults.toStyledString().c_str());

			Test( conversation.Stop() );
		}
		else
		{
			Log::Status( "TestConversationRequestIntents", "Skipping test." );
		}
    }

    void OnMessage(ConversationResponse * a_pConversationResponse)
    {
		Test( a_pConversationResponse != NULL );

        // Test that an Intent is returned
        std::vector<ConversationIntent> m_Intents = a_pConversationResponse->m_Intents;
        Test(m_Intents.size() > 0);
        std::string m_TopIntent = m_Intents[0].m_Intent;
        Test(!m_TopIntent.empty());

	    if (m_TopIntent == "request")
	    {
		    // Test Entity Extraction
		    std::vector<ConversationEntities> m_Entities = a_pConversationResponse->m_Entities;
		    if (m_Entities.size() > 0)
		    {
			    Json::Value json;
			    for (size_t i = 0; i < m_Entities.size(); ++i) {
				    std::string m_Entity = m_Entities[i].m_Entity;
				    std::string m_EntityValue = m_Entities[i].m_Value;
				    Test(!m_Entity.empty() && !m_EntityValue.empty());

				    json[m_Entity] = m_EntityValue;
			    }
			    json["request"] = m_RequestName;

			    m_ConversationResults.append(json);
		    }
	    }

	    m_bConversationTested = true;
    }

};

TestConversationRequestIntents TEST_CONVERSATION_REQUEST_INTENTS;