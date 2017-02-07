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
#include "services/PersonalityInsights/PersonalityInsights.h"

class TestPersonalityInsights : UnitTest
{
public:
    //! Construction
    TestPersonalityInsights() : UnitTest("TestPersonalityInsights"),
                                m_bProfileTested(false)
    {}

    bool m_bProfileTested;

    virtual void RunTest()
    {
        Config config;
        Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

        ThreadPool pool(1);
        PersonalityInsights profile;
        Log::Debug("TestPersonalityInsights", "The service id is %s ", profile.GetServiceId().c_str());

        if ( config.IsConfigured( profile.GetServiceId() ) )
		{
			Test( profile.Start() );

            profile.GetProfile( "Well, thank you very much, Jim, for this opportunity. I want to thank Governor Romney and the University\n"
                                        "of Denver for your hospitality.\n"
                                        "\n"
                                        "There are a lot of points I want to make tonight, but the most important one is that 20 years ago I\n"
                                        "became the luckiest man on Earth because Michelle Obama agreed to marry me.\n"
                                        "\n"
                                        "And so I just want to wish, Sweetie, you happy anniversary and let you know that a year from\n"
                                        "now we will not be celebrating it in front of 40 million people.\n"
                                        "\n"
                                        "You know, four years ago we went through the worst financial crisis since the Great Depression.\n"
                                        "Millions of jobs were lost, the auto industry was on the brink of collapse. The financial system\n"
                                        "had frozen up.\n"
                                        "\n"
                                        "And because of the resilience and the determination of the American people, we've begun to\n"
                                        "fight our way back. Over the last 30 months, we've seen 5 million jobs in the private sector\n"
                                        "created. The auto industry has come roaring back. And housing has begun to rise.\n"
                                        "\n"
                                        "But we all know that we've still got a lot of work to do. And so the question here tonight is not\n"
                                        "where we've been, but where we're going.\n"
                                        "\n"
                                        "Governor Romney has a perspective that says if we cut taxes, skewed towards the wealthy, and\n"
                                        "roll back regulations, that we'll be better off. I've got a different view.\n"
                                        "\n"
                                        "I think we've got to invest in education and training. I think it's important for us to develop new\n"
                                        "sources of energy here in America, that we change our tax code to make sure that we're helping\n"
                                        "small businesses and companies that are investing here in the United States, that we take some of\n"
                                        "the money that we're saving as we wind down two wars to rebuild America and that we reduce\n"
                                        "our deficit in a balanced way that allows us to make these critical investments.\n"
                                        "\n"
                                        "Now, it ultimately is going to be up to the voters — to you — which path we should take. Are we\n"
                                        "going to double on top-down economic policies that helped to get us into this mess or do we\n"
                                        "embrace a new economic patriotism that says America does best when the middle class does\n"
                                        "best? And I'm looking forward to having that debate.",
						  DELEGATE(TestPersonalityInsights, OnGetProfile, Profile *, this));

			Spin(m_bProfileTested);
			Test(m_bProfileTested);

			Test( profile.Stop() );
		}
		else
		{
			Log::Status( "TestPersonalityInsights", "Skipping test now." );
		}
    }

    void OnGetProfile(Profile * a_Callback)
    {
        Log::Debug("TestPersonalityInsights", "Word count is: %f", a_Callback->m_WordCount);
        Personality personality = a_Callback->m_Personality[0];
        Log::Debug("TestPersonalityInsights", "Found top trait as: %s", personality.m_TraitId.c_str());
        m_bProfileTested = true;
    }
};

TestPersonalityInsights TEST_PROFILE;
