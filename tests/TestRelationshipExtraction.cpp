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
#include "services/RelationshipExtraction/RelationshipExtraction.h"

class TestRelationshipExtraction : UnitTest
{
public:
	//! Construction
	TestRelationshipExtraction() : UnitTest("TestRelationshipExtraction"),
		m_bParseTested(false)
	{}

	bool m_bParseTested;

	virtual void RunTest()
	{
		Config config;
		Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

		ThreadPool pool(1);

		RelationshipExtraction re;
		//re.SetCacheEnabled(false);		// turn off cache for unit tests

		Test(re.Start());
		Test(re.Parse( "Wave to the crowd", 
			DELEGATE(TestRelationshipExtraction, OnParse, const Json::Value &, this)) != NULL);

		Time start;
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_bParseTested)
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}

		Test(m_bParseTested);
	}

	void OnParse(const Json::Value & value)
	{
		Test(!value.isNull());
		m_bParseTested = true;
	}
};

TestRelationshipExtraction TEST_DIALOG;
