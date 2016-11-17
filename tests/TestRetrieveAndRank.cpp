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

#include "utils/UnitTest.h"
#include "utils/Log.h"
#include "utils/Time.h"
#include "utils/Config.h"
#include "utils/UniqueID.h"
#include "services/RetrieveAndRank/RetrieveAndRank.h"

class TestRetrieveAndRank : UnitTest
{
public:
    TestRetrieveAndRank() : UnitTest("TestRetrieveAndRank"),
                            m_bRetrieveAndRankTested( false ),
							m_bRetreiveAndRankQuestionTested( false ),
                            m_WorkspaceId( "Version1" ),
                            m_SolrId( "scf30e777d_3f30_46ea_850b_0b1433f56fd0" ),
                            m_TestText( "Location_Gym_3" )
    {}

    bool m_bRetrieveAndRankTested;
	bool m_bRetreiveAndRankQuestionTested;
    std::string m_WorkspaceId;
    std::string m_SolrId;
    std::string m_TestText;

    virtual void RunTest()
    {
        Config config;
        Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);
        ThreadPool pool(1);

        RetrieveAndRank rr;
		RetrieveAndRank rr2("RetrieveAndRankV2");
        if ( config.IsConfigured( rr.GetServiceId() ) && config.IsConfigured( rr2.GetServiceId() ) )
		{
			Test( rr.Start() );
			Test( rr2.Start() );

			Log::Debug("TestRetrieveAndRank","Retrieve and Rank Started");

			Log::Debug("TestRetrieveAndRank","Testing TestRetrieveAndRank Response for input: %s", m_TestText.c_str());
	//        conversation.Message(m_WorkspaceId, m_Context, m_VersionId, m_TestText, DELEGATE(TestConversation, OnMessage, ConversationMessageResponse *, this));
			rr.Select(m_SolrId, m_WorkspaceId, m_TestText, DELEGATE(TestRetrieveAndRank, OnMessage, RetrieveAndRankResponse *, this));
			Spin(m_bRetrieveAndRankTested);
			Test(m_bRetrieveAndRankTested);

			rr2.Ask("scb5b04486_beb0_4a5c_8e45_442d089a94ad", "Samsung1", "tell me about battery", "unranked",
				DELEGATE(TestRetrieveAndRank, OnAskMessage, RetrieveAndRankResponse *, this ) );
			Spin(m_bRetreiveAndRankQuestionTested);
			Test(m_bRetreiveAndRankQuestionTested);
			Test( rr.Stop() );
			Test( rr2.Stop() );
		}
		else
		{
			Log::Status( "TestRetrieveAndRank", "Skipping test." );
		}
    }

    void OnMessage(RetrieveAndRankResponse * a_pRetrieveAndRankResponse)
    {
		Test( a_pRetrieveAndRankResponse != NULL );

        std::vector<Documents> m_Documents = a_pRetrieveAndRankResponse->m_Docs;
        std::string m_Body = m_Documents[0].m_Body;
        Log::Debug("TestRetrieveAndRank", "Received response from R&R Service: %s", m_Body.c_str());

        m_bRetrieveAndRankTested = true;
    }

	void OnAskMessage(RetrieveAndRankResponse * a_pRetreiveAndRankResponse)
	{
		Test( a_pRetreiveAndRankResponse != NULL );
		Log::Debug("TestRetreiveAndRank", "Received response with confidence %f", a_pRetreiveAndRankResponse->m_MaxScore);
		m_bRetreiveAndRankQuestionTested = true;
	}
};

TestRetrieveAndRank TEST_RETRIEVE_AND_RANK;