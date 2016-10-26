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

#include "services/NaturalLanguageClassifier/NaturalLanguageClassifier.h"

class TestNaturalLanguageClassifier : UnitTest
{
public:
    //! Construction
    TestNaturalLanguageClassifier() : UnitTest("TestNaturalLanguageClassifier"),
    m_GetClassifierTested(false),
    m_GetClassifiersTested(false),
    m_TrainingTested(false),
    m_DeleteTested(false),
    m_NumberOfClassifiers(0),
    m_ClassifyTested(false),
    m_ServiceStatusTested(false)
    {}
    
    std::string m_NaturalLanguageClassifierId;
    std::string m_NLCStatus;
    
    bool m_GetClassifiersTested;
    bool m_GetClassifierTested;
    bool m_ClassifyTested;
    bool m_TrainingTested;
    bool m_DeleteTested;
    int m_NumberOfClassifiers;
    bool m_ServiceStatusTested;
    
    virtual void RunTest()
    {
        Config config;
        Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);
        
        ThreadPool pool(1);

        NaturalLanguageClassifier nlc;
        if (nlc.Start())
		{
			Log::Status("TestNaturalLanguageClassifier", "Testing GetServiceStatus");
			nlc.GetServiceStatus(
				DELEGATE(TestNaturalLanguageClassifier, OnGetServiceStatus, const IService::ServiceStatus &, this));

			Spin( m_ServiceStatusTested );
			Test(m_ServiceStatusTested);

			// TEST GETTING ALL CLASSIFIERS
			Log::Status("TestNaturalLanguageClassifier", "Testing GetClassifiers");
			nlc.GetClassifiers(DELEGATE(TestNaturalLanguageClassifier, OnGetClassifiers, Classifiers *, this));
        
			Spin( m_GetClassifiersTested );
			Test(m_GetClassifiersTested);
        
			// If there is at least one classifier, it will get it's Id and Status
			if ( m_NumberOfClassifiers > 0 )
			{
				// TEST GETTING A CLASSIFIER
				// Checks if a Classifier's status is 'Available'
				// If the classifier is not available, it will train another classifier.
				// If the classifier is availble, it will test the ability to classify and delete
				Log::Status("TestNaturalLanguageClassifier", "Testing GetClassifier");
				nlc.GetClassifier(m_NaturalLanguageClassifierId,DELEGATE(TestNaturalLanguageClassifier, OnGetClassifier, Classifier *, this));

				Spin( m_GetClassifiersTested );
				Test(m_GetClassifierTested);
			}
        
			// If there are no classifiers, it will train one
			else
			{
				// TEST TRAINING A CLASSIFIER
				// Since no classifiers are available, a new classifier will be trained
				Log::Status("TestNaturalLanguageClassifier","No available classifier to test -- training new classifier");
				Log::Status("TestNaturalLanguageClassifier", "Testing TrainClassifier");
				Test(nlc.TrainClassifierFile("TestClassifier","en","./etc/tests/weather_sample.csv",DELEGATE(TestNaturalLanguageClassifier, OnTrainClassifier, Classifier *, this)));

				Spin( m_TrainingTested );
				Test(m_TrainingTested);
			}
        
			// TEST CLASSIFYING A QUESTION
			// Only Classify a question if the Classifier is Available
			if (m_NLCStatus == "Available")
			{
				// Classifies the question 'How hot is it'
				// Will pass if the JSON has field 'top_class'
				Log::Status("TestNaturalLanguageClassifier", "Testing Classify 'How hot is it'");
				nlc.Classify(m_NaturalLanguageClassifierId,"How hot is it", DELEGATE(TestNaturalLanguageClassifier, OnClassify, const Json::Value &, this));
        
				Spin( m_ClassifyTested );
				Test(m_ClassifyTested);
			}
        
			// TEST DELETING A CLASSIFIER
			// Only delete a classifier if it is Available or Unavailable (NOT Testing)
			if (m_NLCStatus == "Available" || m_NLCStatus == "Unavailable")
			{
				Log::Status("TestNaturalLanguageClassifier", "Testing DeleteClassifier");
				nlc.DeleteClassifer(m_NaturalLanguageClassifierId,DELEGATE(TestNaturalLanguageClassifier, OnDeleteClassifier, const Json::Value &, this));
        
				Spin( m_DeleteTested );
				Test(m_DeleteTested);
			}
		}
		else
		{
			Log::Status( "TestNaturalLanguageClassifier", "Skipping test." );
		}
	}
    
    void OnTrainClassifier(Classifier * a_pClassifier)
    {
        Test(a_pClassifier != NULL);
        m_NLCStatus = a_pClassifier->m_Status;
        Test(m_NLCStatus == "Training");
        m_TrainingTested = true;
    }
    
    void OnGetClassifiers(Classifiers * a_pClassifiers)
    {
        Test(a_pClassifiers != NULL);
        m_NumberOfClassifiers = a_pClassifiers->m_Classifiers.size();
        if ( m_NumberOfClassifiers > 0 )
        {
            Classifier nlClassifier = a_pClassifiers->m_Classifiers[0];
            m_NaturalLanguageClassifierId = nlClassifier.m_ClassifierId;
        }
        m_GetClassifiersTested = true;
    }
    
    void OnGetClassifier(Classifier * a_pClassifier)
    {
        Test(a_pClassifier != NULL);
        m_NLCStatus = a_pClassifier->m_Status;
        Log::Status("TestNaturalLanguageClassifier","Classifier Status: %s",m_NLCStatus.c_str());
        Test(m_NLCStatus == "Available" || m_NLCStatus == "Training" || m_NLCStatus == "Unavailable");
        m_GetClassifierTested = true;
    }
    
    void OnClassify(const Json::Value & a_Response)
    {
        Test(!a_Response.isNull());
        Test(a_Response.isMember("top_class"));
        m_ClassifyTested = true;
    }
    
    void OnDeleteClassifier(const Json::Value & value)
    {
        m_DeleteTested = true;
    }

    void OnGetServiceStatus(const IService::ServiceStatus & a_Status)
    {
        m_ServiceStatusTested = true;
    }
    
};

TestNaturalLanguageClassifier TEST_NATURALLANGUAGECLASSIFIER;
