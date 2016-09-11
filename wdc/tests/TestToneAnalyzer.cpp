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
#include "services/ToneAnalyzer/ToneAnalyzer.h"

class TestToneAnalyzer : UnitTest
{
public:
    //! Construction
    TestToneAnalyzer() : UnitTest("TestToneAnalyzer"),
                                   m_bToneTested(false)
    {}

    bool m_bToneTested;

    virtual void RunTest()
    {
        Config config;
        Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

        ThreadPool pool(1);

        ToneAnalyzer tone;

        Test(tone.Start());
        tone.GetTone( "how is your day going?",
                      DELEGATE(TestToneAnalyzer, OnTone, DocumentTones *, this));

        Spin(m_bToneTested);
        Test(m_bToneTested);
    }

    void OnTone(DocumentTones * a_Callback)
    {
        Tone tone = a_Callback->m_ToneCategories[0].m_Tones[0];
        Log::Debug("TestToneAnalyzer", "Found top tone as: %s", tone.m_ToneName.c_str());
        Log::Debug("TestToneAnalyzer", "Found top tone id as: %s", tone.m_ToneId.c_str());
        Log::Debug("TestToneAnalyzer", "Found top confidence as: %f", tone.m_Score);
        m_bToneTested = true;
    }
};

TestToneAnalyzer TEST_TONE_ANALYZER;
