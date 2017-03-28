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

#include "VisualRecognition.h"
#include "utils/Form.h"
#include "utils/Path.h"

const std::string DEFAULT_CLASSIFIER( "default" );

REG_SERIALIZABLE( VisualRecognition );
RTTI_IMPL( VisualRecognition, IService );


VisualRecognition::VisualRecognition() : 
	IService("VisualRecognitionV1"),
	m_APIVersion( "2016-05-20" ),
	m_ClassifyThreshold( 0.035f )
{}

//! ISerializable
void VisualRecognition::Serialize(Json::Value & json)
{
	IService::Serialize(json);
	json["m_APIVersion"] = m_APIVersion;
	json["m_ClassifyThreshold"] = m_ClassifyThreshold;
}

void VisualRecognition::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
	if ( json["m_APIVersion"].isString() )
		m_APIVersion = json["m_APIVersion"].asString();
	if ( json["m_ClassifyThreshold"].isDouble() )
		m_ClassifyThreshold = json["m_ClassifyThreshold"].asFloat();
}

//! IService interface
bool VisualRecognition::Start()
{
	if (!IService::Start())
		return false;
	if (!StringUtil::EndsWith(m_pConfig->m_URL, "visual-recognition/api"))
	{
		Log::Error("VisualRecognition", "Configured URL not ended with visual-recognition/api");
		return false;
	}	
	if (m_pConfig->m_User.size() == 0)
		Log::Warning("VisualRecognition", "API-Key expected in user field.");

	return true;
}

void VisualRecognition::GetServiceStatus(ServiceStatusCallback a_Callback)
{
	if (m_pConfig != NULL)
		new ServiceStatusChecker(this, a_Callback);
	else
		a_Callback(ServiceStatus(m_ServiceId, false));
}

void VisualRecognition::GetClassifiers( OnGetClassifier a_Callback )
{
	std::string params = "/v3/classifiers";
	params += "?apikey=" + m_pConfig->m_User;
	params += "&version=" + m_APIVersion;

	new RequestJson(this, params, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void VisualRecognition::GetClassifier( const std::string & a_ClassifierId,
	OnGetClassifier a_Callback )
{
	std::string params = "/v3/classifiers/" + a_ClassifierId;
	params += "?apikey=" + m_pConfig->m_User;
	params += "&version=" + m_APIVersion;

	new RequestJson(this, params, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void VisualRecognition::ClassifyImage(const std::string & a_ImageData,
	const std::vector<std::string> & a_Classifiers,
	OnClassifyImage a_Callback,
	bool a_bKnowledgeGraph /*= false*/ )
{
	std::string parameters = "/v3/classify";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=" + m_APIVersion;
	parameters += "&threshold=" + StringUtil::Format( "%f", m_ClassifyThreshold );
	if (a_bKnowledgeGraph)
		parameters += "&knowledgeGraph=1";

	Json::Value ids;
	for(size_t i=0;i<a_Classifiers.size();++i)
		ids["classifier_ids"][i] = a_Classifiers[i];

	Form form;
	form.AddFilePart("parameters", "myparams.json", ids.toStyledString() );
	form.AddFilePart("images_file", "imageToClassify.jpg", a_ImageData);
	form.Finish();

	Headers headers;
	headers["Content-Type"] = form.GetContentType();
	headers["Accept-Language"] = "en";

	new RequestJson(this, parameters, "POST", headers, form.GetBody(), a_Callback);
}


void VisualRecognition::DetectFaces(const std::string & a_ImageData, OnDetectFaces a_Callback )
{
	std::string parameters = "/v3/detect_faces";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=" + m_APIVersion;

	Form form;
	form.AddFilePart("images_file", "imageToClassify.jpg", a_ImageData);
	form.Finish();

	Headers headers;
	headers["Content-Type"] = form.GetContentType();
	headers["Accept-Language"] = "en";

	new RequestJson(this, parameters, "POST", headers, form.GetBody(), a_Callback);
}

void VisualRecognition::CreateClassifier( 
	const std::string & a_ClassifierName,
	const std::vector<std::string> & a_PositiveExamples,
	const std::string & a_NegativeExamples,
	OnCreateClassifier a_Callback )
{
	std::string parameters = "/v3/classifiers";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=" + m_APIVersion;

	Form form;
	form.AddFormField("name", a_ClassifierName );
	for(size_t i=0;i<a_PositiveExamples.size();++i)
	{
		if (! form.AddFilePartFromPath( Path( a_PositiveExamples[i] ).GetFile(), a_PositiveExamples[i] ) )
			Log::Error( "VisualRecognition", "Failed to load positive examples." );
	}
	if ( a_NegativeExamples.size() > 0 )
		form.AddFilePartFromPath( "negative_examples", a_NegativeExamples );
	form.Finish();

	Headers headers;
	headers["Content-Type"] = form.GetContentType();

	new RequestJson(this, parameters, "POST", headers, form.GetBody(), a_Callback);
}

void VisualRecognition::UpdateClassifier(
	const std::string & a_ClassifierId,
	const std::string & a_ClassifierName,
	const std::vector<std::string> & a_PositiveExamples,
	const std::string & a_NegativeExamples,
	OnClassifierTrained a_Callback)
{
	std::string parameters = "/v3/classifiers/";
	parameters += a_ClassifierId;
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=" + m_APIVersion;

	Form form;
	form.AddFormField("name", a_ClassifierName );
	for(size_t i=0;i<a_PositiveExamples.size();++i)
	{
		if (! form.AddFilePartFromPath( Path( a_PositiveExamples[i] ).GetFile(), a_PositiveExamples[i] ) )
			Log::Error( "VisualRecognition", "Failed to load positive examples." );
	}
	if ( a_NegativeExamples.size() > 0 )
		form.AddFilePartFromPath( "negative_examples", a_NegativeExamples );
	form.Finish();

	Headers headers;
	headers["Content-Type"] = form.GetContentType();

	new RequestJson(this, parameters, "POST", headers, form.GetBody(), a_Callback);
}

void VisualRecognition::DeleteClassifier( const std::string & a_ClassifierId,
	OnDeleteClassifier a_Callback )
{
	std::string parameters = "/v3/classifiers/";
	parameters += a_ClassifierId;
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=" + m_APIVersion;

	new Request(this, parameters, "DELETE", NULL_HEADERS, EMPTY_STRING, a_Callback );
}


VisualRecognition::ServiceStatusChecker::ServiceStatusChecker(VisualRecognition * a_pService, ServiceStatusCallback a_Callback)
	: m_pService(a_pService), m_Callback(a_Callback)
{
	if (a_Callback.IsValid())
		a_Callback(ServiceStatus(m_pService->m_ServiceId, true));
	delete this;
}

void VisualRecognition::ServiceStatusChecker::OnCheckService(const Json::Value & a_Response)
{}

