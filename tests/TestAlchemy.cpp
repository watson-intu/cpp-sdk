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
#include "services/Alchemy/Alchemy.h"

#include <fstream>

class TestAlchemy : UnitTest
{
public:
	//! Construction
	TestAlchemy() : UnitTest("TestAlchemy"),
		m_bDetectFacesTested(false),
		m_bClassifyImageTested(false)
	{}

	bool m_bDetectFacesTested;
	bool m_bClassifyImageTested;

	virtual void RunTest()
	{
		// read in all the file data..
		std::ifstream input("./etc/tests/AlchemyTest.jpg", std::ios::in | std::ios::binary);
		Test(input.is_open());
		std::string imageData;
		imageData.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
		input.close();

		Config config;
		Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

		ThreadPool pool(1);

		Alchemy alchemy;
		Test(alchemy.Start());

		alchemy.DetectFaces(imageData,
			DELEGATE(TestAlchemy, OnDetectFaces, const Json::Value &, this) );
		alchemy.ClassifyImage(imageData,
			DELEGATE(TestAlchemy, OnClassifyImage, const Json::Value &, this) );

		Time start;
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && (!m_bDetectFacesTested || !m_bClassifyImageTested) )
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}

		Test(m_bDetectFacesTested);
		Test(m_bClassifyImageTested);
	}

	void OnDetectFaces(const Json::Value & json)
	{
		Log::Debug("AlchemyTest", "OnDetectFaces(): %s", json.toStyledString().c_str());

		Test(!json.isNull());
		Test(json["imageFaces"].size() > 0);

		m_bDetectFacesTested = true;
	}

	void OnClassifyImage(const Json::Value & json)
	{
		Log::Debug("AlchemyTest", "OnClassifyImage(): %s", json.toStyledString().c_str());

		Test(!json.isNull());
		Test(json["imageKeywords"].size() > 0);

		m_bClassifyImageTested = true;
	}


};

TestAlchemy TEST_ALCHEMY;
