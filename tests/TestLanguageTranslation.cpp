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
#include "utils/ThreadPool.h"
#include "utils/Config.h"
#include "services/LanguageTranslator/LanguageTranslator.h"

class TestLanguageTranslator : UnitTest 
{
public:
    //! Construction
    TestLanguageTranslator() : UnitTest("TestLanguageTranslator"),
                      m_bTranslate(false),
                      m_bIdentifiedLanguages(false),
                      m_bIdentifyText(false),
                      m_Counter( 0 )
    { }

    bool    m_bTranslate;
    bool    m_bIdentifiedLanguages;
    bool    m_bIdentifyText;
    int     m_Counter;

    virtual void RunTest()
    {
        Config config;
        Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

        ThreadPool pool(1);
        LanguageTranslator lt;

        if ( config.IsConfigured( lt.GetServiceId() ) )
		{
			Test( lt.Start() );
			std::string target = "es";
			std::string source = "en";
			std::string text = "hello";
			lt.Translation(source, target, text, DELEGATE(TestLanguageTranslator, OnTranslate, Translations *, this));
			lt.IdentifiableLanguages(DELEGATE(TestLanguageTranslator, OnIdentifiedLanguages, Languages *, this));
			lt.Identify(text, DELEGATE(TestLanguageTranslator, OnIdentify, IdentifiedLanguages *, this ));

			Spin(m_Counter, 3);

			Test(m_bTranslate);
			Test(m_bIdentifiedLanguages);
			Test(m_bIdentifyText);
			Test( lt.Stop() );
		}
		else
		{
			Log::Status( "TestLanguageTranslator", "Skipping test, no credentials." );
		}
    }

    void OnTranslate(Translations * a_Callback)
    {
        m_Counter++;
        m_bTranslate = true;
        Translation translation = a_Callback->m_Translations[0];
        std::string a_Translation = translation.m_Translation;
        Log::Debug("TestLanguageTranslator", "Found translation: %s", a_Translation.c_str());
    }

    void OnIdentifiedLanguages(Languages * a_Callback)
    {
        m_Counter++;
        m_bIdentifiedLanguages = true;
    }

    void OnIdentify(IdentifiedLanguages * a_Callback)
    {
        m_Counter++;
        m_bIdentifyText = true;
    }
};

TestLanguageTranslator TEST_LANGUAGE_TRANSLATION;