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
#include "services/VisualRecognition/VisualRecognition.h"

#include <fstream>

class TestVisualRecognition : UnitTest
{
public:
	//! Construction
	TestVisualRecognition() : UnitTest("TestVisualRecognition"),
		m_bDetectFacesTested(false),
		m_bClassifyImageTested(false),
		m_bIdentifyTextTested(false)
	{}

	bool m_bDetectFacesTested;
	bool m_bClassifyImageTested;
	bool m_bIdentifyTextTested;

	virtual void RunTest()
	{
		Config config;
		Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);
		ThreadPool pool(1);

		VisualRecognition vr;
		if ( config.IsConfigured( vr.GetServiceId() ) )
		{
			Test( vr.Start() );

			// read in all the file data..
			std::ifstream input("./etc/tests/VisualRecognitionTest.jpg", std::ios::in | std::ios::binary);
			Test(input.is_open());
			std::string imageData;
			imageData.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
			input.close();

			Log::Status("TestVisualRecognition", "Started VisualRecognition");
			vr.DetectFaces(imageData,
				DELEGATE(TestVisualRecognition, OnDetectFaces, const Json::Value &, this) );
			Spin( m_bDetectFacesTested );
			Test( m_bDetectFacesTested );

			vr.ClassifyImage(imageData, "default",
				DELEGATE(TestVisualRecognition, OnClassifyImage, const Json::Value &, this) );
			Spin( m_bClassifyImageTested );
			Test( m_bClassifyImageTested );

			vr.IdentifyText(imageData,
				DELEGATE(TestVisualRecognition, OnIdentifyText, const Json::Value &, this) );
			Spin( m_bIdentifyTextTested );
			Test( m_bIdentifyTextTested );

			Test( vr.Stop() );
		}
		else
		{
			Log::Status( "TestVisualRecognition", "Skipping test." );
		}
	}

	void OnDetectFaces(const Json::Value & json)
	{
		Log::Debug("VisualRecognitionTest", "OnDetectFaces(): %s", json.toStyledString().c_str());

		Test(!json.isNull());
		Test(json["images"].size() > 0);

		m_bDetectFacesTested = true;
	}

	void OnClassifyImage(const Json::Value & json)
	{
		Log::Debug("VisualRecognitionTest", "OnClassifyImage(): %s", json.toStyledString().c_str());

		Test(!json.isNull());
		Test(json["images"].size() > 0);

		m_bClassifyImageTested = true;
	}

	void OnIdentifyText(const Json::Value & json)
	{
		Log::Debug("VisualRecognitionTest", "OnIdentifyText(): %s", json.toStyledString().c_str());

		Test(!json.isNull());
		Test(json["images"].size() > 0);

		m_bIdentifyTextTested = true;
	}


};

TestVisualRecognition TEST_VISUALRECOGNITION;
