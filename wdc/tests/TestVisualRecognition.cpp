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
		Log::Status("TestVisualRecognition", "Starting TestVisualRecognition");
		// read in all the file data..
		std::ifstream input("./etc/tests/VisualRecognitionTest.jpg", std::ios::in | std::ios::binary);
		Test(input.is_open());
		std::string imageData;
		imageData.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
		input.close();

		// Log::Status("TestVisualRecognition", "Opened VisualRecognitionTest.jpg");
		// std::ifstream inputText("./etc/tests/VisualRecognitionTestText.jpg", std::ios::in | std::ios::binary);
		// Test(inputText.is_open());
		// std::string textImageData;
		// textImageData.assign(std::istreambuf_iterator<char>(inputText), std::istreambuf_iterator<char>());
		// input.close();
		// Log::Status("TestVisualRecognition", "Opened VisualRecognitionTestText.jpg");


		Config config;
		Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

		ThreadPool pool(1);

		VisualRecognition * visualRecognition = new VisualRecognition();
		Test(visualRecognition->Start());

		Log::Status("TestVisualRecognition", "Started VisualRecognition");
		visualRecognition->DetectFaces(imageData,
			DELEGATE(TestVisualRecognition, OnDetectFaces, const Json::Value &, this) );
		visualRecognition->ClassifyImage(imageData,
			DELEGATE(TestVisualRecognition, OnClassifyImage, const Json::Value &, this) );
		// visualRecognition->IdentifyText(textImageData,
		// 	DELEGATE(TestVisualRecognition, OnIdentifyText, const Json::Value &, this) );

		Time start;
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && (!m_bDetectFacesTested || !m_bClassifyImageTested) )
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}
		
		if (m_bDetectFacesTested) {
			Log::Status("TestVisualRecognition", "Successfully tested face detection");
		}
		if (m_bClassifyImageTested) {
			Log::Status("TestVisualRecognition", "Successfully tested image classification");
		}
		// if (m_bIdentifyTextTested) {
		// 	Log::Status("TestVisualRecognition", "Successfully tested text recognition");
		// }

		Test(m_bDetectFacesTested);
		Test(m_bClassifyImageTested);
		// Test(m_bIdentifyTextTested);
	}

	void OnDetectFaces(const Json::Value & json)
	{
		Log::Debug("VisualRecognitionTest", "OnDetectFaces(): %s", json.toStyledString().c_str());

		Test(!json.isNull());
		// Test(json["images"].size() > 0);

		m_bDetectFacesTested = true;
	}

	void OnClassifyImage(const Json::Value & json)
	{
		Log::Debug("VisualRecognitionTest", "OnClassifyImage(): %s", json.toStyledString().c_str());

		Test(!json.isNull());
		// Test(json["images"].size() > 0);

		m_bClassifyImageTested = true;
	}

	// void OnIdentifyText(const Json::Value & json)
	// {
	// 	Log::Debug("VisualRecognitionTest", "OnIdentifyText(): %s", json.toStyledString().c_str());

	// 	Test(!json.isNull());
	// 	// Test(json["images"].size() > 0);

	// 	m_bIdentifyTextTested = true;
	// }


};

TestVisualRecognition TEST_VISUALRECOGNITION;
