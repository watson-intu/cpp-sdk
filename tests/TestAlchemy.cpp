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
#include "services/Alchemy/Alchemy.h"

#include <fstream>

class TestAlchemy : UnitTest
{
public:
	//! Construction
	TestAlchemy() : UnitTest("TestAlchemy"),
		m_bGetChunkTagsTested(false),
		m_bGetPosTagsTested(false),
		m_bGetEntitiesTested(false),
		m_bGetNewsTested(false)
	{}

	bool m_bGetChunkTagsTested;
	bool m_bGetPosTagsTested;
	bool m_bGetEntitiesTested;
	bool m_bGetNewsTested;

	virtual void RunTest()
	{
		Config config;
		Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

		ThreadPool pool(1);

		Alchemy alchemy;
		if ( config.IsConfigured( alchemy.GetServiceId() ) )
		{
			Test( alchemy.Start() );
			alchemy.GetPosTags( "can you wave to the crowd?",
				DELEGATE(TestAlchemy, OnGetPosTags, const Json::Value &, this) );
			Spin( m_bGetPosTagsTested );

			alchemy.GetChunkTags( "can you wave to the crowd?",
				DELEGATE(TestAlchemy, OnGetChunkTags, const Json::Value &, this) );
			Spin( m_bGetChunkTagsTested );

			alchemy.GetEntities( "what is the weather like in Austin",
				DELEGATE(TestAlchemy, OnGetEntities, const Json::Value &, this) );
			Spin( m_bGetEntitiesTested );

			alchemy.GetNews("IBM", 1475193600, 1475881200, 10,
				DELEGATE(TestAlchemy, OnGetNews, const Json::Value &, this));
			Spin(m_bGetNewsTested);

			Test(m_bGetPosTagsTested);
			Test(m_bGetChunkTagsTested);
			Test(m_bGetEntitiesTested);
			Test(m_bGetNewsTested);

			Test( alchemy.Stop() );
		}
		else
		{
			Log::Status( "TestAlchemy", "Skipping test." );
		}
	}

	void OnGetPosTags(const Json::Value & json)
	{
		Log::Debug("AlchemyTest", "OnGetPosTags(): %s", json.toStyledString().c_str());

		Test(!json.isNull());
		m_bGetPosTagsTested = true;
	}

	void OnGetChunkTags(const Json::Value & json)
	{
		Log::Debug("AlchemyTest", "OnGetChunkTags(): %s", json.toStyledString().c_str());

		Test(!json.isNull());
		m_bGetChunkTagsTested = true;
	}

	void OnGetEntities(const Json::Value & json)
	{
		Log::Debug("AlchemyTest", "OnGetEntities(): %s", json.toStyledString().c_str());
		Test(!json.isNull());
		m_bGetEntitiesTested = true;
	}

	void OnGetNews(const Json::Value & json)
	{
		Log::Debug("AlchemyTest", "OnGetNews(): %s", json.toStyledString().c_str());
		Test(!json.isNull());
		m_bGetNewsTested = true;
	}
};

TestAlchemy TEST_ALCHEMY;
