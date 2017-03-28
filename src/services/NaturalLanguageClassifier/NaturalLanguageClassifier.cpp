/**
* Copyright 2015 IBM Corp. All Rights Reserved.
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

#include <fstream>

#include "NaturalLanguageClassifier.h"
#include "utils/Form.h"

REG_SERIALIZABLE( NaturalLanguageClassifier );
RTTI_IMPL( Classifier, ISerializable );
RTTI_IMPL( Classifiers, ISerializable );
RTTI_IMPL( Class, ISerializable );
RTTI_IMPL( ClassifyResult, ISerializable );
RTTI_IMPL( NaturalLanguageClassifier, IService );


NaturalLanguageClassifier::NaturalLanguageClassifier() : IService("NaturalLanguageClassifierV1")
{}

void NaturalLanguageClassifier::Serialize(Json::Value & json)
{
	IService::Serialize(json);
}

void NaturalLanguageClassifier::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
}

//! IService interface
bool NaturalLanguageClassifier::Start()
{
	if (! IService::Start() )
		return false;

	if (! StringUtil::EndsWith( m_pConfig->m_URL, "natural-language-classifier/api" ) )
	{
		Log::Error( "NaturalLanguageClassifier", "Configured URL not ended with natural-language-classifier/api" );
		return false;
	}

	return true;
}

void NaturalLanguageClassifier::GetServiceStatus( ServiceStatusCallback a_Callback )
{
    if ( IsConfigured() )
        new ServiceStatusChecker(this, a_Callback);
    else
        a_Callback(ServiceStatus(m_ServiceId, false));
}

//! Creates an object responsible for service status checking
NaturalLanguageClassifier::ServiceStatusChecker::ServiceStatusChecker
        (NaturalLanguageClassifier* a_pNlcService, ServiceStatusCallback a_Callback)
        : m_pNlcService(a_pNlcService), m_Callback(a_Callback)
{
    m_pNlcService->GetClassifiers(DELEGATE(ServiceStatusChecker, OnCheckService, Classifiers*, this));
}

//! Callback function invoked when service status is checked
void NaturalLanguageClassifier::ServiceStatusChecker::OnCheckService(Classifiers* a_pClassifiers)
{
    if (m_Callback.IsValid())
        m_Callback(ServiceStatus(m_pNlcService->m_ServiceId, a_pClassifiers != NULL));

    delete a_pClassifiers;
    delete this;
}

//! Request a list of all available classifiers
void NaturalLanguageClassifier::GetClassifiers(OnGetClassifiers a_Callback)
{
	new RequestObj<Classifiers>( this, "/v1/classifiers", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void NaturalLanguageClassifier::FindClassifiers(const std::string & a_Find, OnGetClassifiers a_Callback)
{
	new FindRequest(this, a_Find, a_Callback);
}

//! Request a specific classifier.
void NaturalLanguageClassifier::GetClassifier( const std::string & a_ClassifierId,
	OnGetClassifier a_Callback )
{
	new RequestObj<Classifier>( this, "/v1/classifiers/" + a_ClassifierId, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

bool NaturalLanguageClassifier::TrainClassifierFile( const std::string & a_ClassifierName,
	const std::string & a_Language,
	const std::string & a_ClassifierFile,
	OnTrainClassifier a_Callback )
{
	// read in all the file data..
	std::ifstream input(a_ClassifierFile.c_str(), std::ios::in | std::ios::binary);
	if (!input.is_open()) 
	{
		Log::Error( "NaturalLanguageClassifier", "Failed to open input file %s.", a_ClassifierFile.c_str() );
		return false;
	}

	std::string fileData;
	fileData.assign( std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() );

	TrainClassifier( a_ClassifierName, a_Language, fileData, a_Callback );
	return true;
}

//! Train a new classifier.
void NaturalLanguageClassifier::TrainClassifier( const std::string & a_ClassifierName, 
	const std::string & a_Language, 
	const std::string & a_TrainingData,
	OnTrainClassifier a_Callback )
{
	Json::Value trainingMeta;
	trainingMeta["language"] = a_Language;
	trainingMeta["name"] = a_ClassifierName;

	Form form;
	form.AddFormField( "training_metadata", trainingMeta.toStyledString() );
	form.AddFormField( "training_data", StringUtil::Trim( a_TrainingData, " \r\n") );
	form.Finish();

	Headers headers;
	headers["Content-Type"] = form.GetContentType();

	new RequestObj<Classifier>( this, "/v1/classifiers", "POST", headers, form.GetBody(), a_Callback, NULL, 10 * 60 );
}

//! Classify text
void NaturalLanguageClassifier::Classify( const std::string & a_ClassifierId,
	const std::string & a_Text, OnClassify a_Callback )
{
	Headers headers;
	headers["Content-Type"] = "application/json";

	Json::Value req;
	req["text"] = a_Text;

	new RequestJson( this, "/v1/classifiers/" + a_ClassifierId + "/classify", "POST", headers, req.toStyledString(), a_Callback,
		new CacheRequest( a_ClassifierId, StringHash::DJB(a_Text.c_str() ) ) );
}

//! Remove a classifier
void NaturalLanguageClassifier::DeleteClassifer( const std::string & a_ClassifierId,
	OnDeleteClassifier a_Callback )
{
	new RequestJson( this, "/v1/classifiers/" + a_ClassifierId, "DELETE", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

//-------------------------------------------------------

NaturalLanguageClassifier::FindRequest::FindRequest(NaturalLanguageClassifier * a_pService, const std::string & a_Find, OnGetClassifiers a_Callback) :
	m_pService(a_pService), m_Find(a_Find), m_Callback(a_Callback), m_nPendingRequests( 0 ), m_pClassifiers( NULL )
{
	m_nPendingRequests += 1;
	m_pService->GetClassifiers(DELEGATE(FindRequest, OnClassifiers, Classifiers *, this));
}

void NaturalLanguageClassifier::FindRequest::OnClassifiers(Classifiers * a_pClassifiers)
{
	m_nPendingRequests -= 1;
	m_pClassifiers = a_pClassifiers;

	if (m_pClassifiers != NULL)
	{
		std::vector<Classifier> & list = m_pClassifiers->m_Classifiers;
		for (size_t i = 0; i < list.size(); ++i)
		{
			if (m_Find.size() > 0 && !StringUtil::WildMatch(m_Find.c_str(), list[i].m_Name.c_str()))
			{
				// replace with the last element, then pop the back..
				if (i < (list.size() - 1))
					list[i] = list[list.size() - 1];
				list.pop_back();

				i -= 1;
				continue;
			}

			m_nPendingRequests += 1;
			m_pService->GetClassifier(list[i].m_ClassifierId, DELEGATE(FindRequest, OnClassifier, Classifier *, this));
		}
	}

	if (m_nPendingRequests == 0)
	{
		if (m_Callback.IsValid())
			m_Callback(m_pClassifiers);
		delete this;
	}
}

void NaturalLanguageClassifier::FindRequest::OnClassifier(Classifier * a_pClassifier)
{
	m_nPendingRequests -= 1;
	if ( a_pClassifier != NULL )
	{
		std::vector<Classifier> & list = m_pClassifiers->m_Classifiers;
		for (size_t i = 0; i < list.size(); ++i)
		{
			if (list[i].m_ClassifierId == a_pClassifier->m_ClassifierId)
				list[i] = *a_pClassifier;
		}
	}
	else
	{
		Log::Error( "NaturalLanguageClassifier", "Failed to find classifier." );
	}

	if (m_nPendingRequests == 0)
	{
		if (m_Callback.IsValid())
			m_Callback(m_pClassifiers);
		delete this;
	}
}
