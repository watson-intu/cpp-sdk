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

REG_SERIALIZABLE( VisualRecognition );
RTTI_IMPL( VisualRecognition, IService );


VisualRecognition::VisualRecognition() : IService("VisualRecognitionV3")
{}

//! ISerializable
void VisualRecognition::Serialize(Json::Value & json)
{
	IService::Serialize(json);
}

void VisualRecognition::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
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
	{
		Log::Error("VisualRecognition", "API-Key expected in user field.");
		return false;
	}

	return true;
}

void VisualRecognition::GetServiceStatus(ServiceStatusCallback a_Callback)
{
	if (m_pConfig != NULL)
		new ServiceStatusChecker(this, a_Callback);
	else
		a_Callback(ServiceStatus(m_ServiceId, false));
}

void VisualRecognition::ClassifyImage(const std::string & a_ImageData, const std::string & classifierId, OnClassifyImage a_Callback, bool a_bKnowledgeGraph )
{
	std::string parameters = "/v3/classify";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=2016-05-20";
	parameters += "&threshold=0.035";
	if (a_bKnowledgeGraph)
		parameters += "&knowledgeGraph=1";

	std::string classifierParams = "{\"classifier_ids\": [\"" + classifierId + "\"]}";

	Form form;
    form.AddFilePart("images_file", "imageToClassify.jpg", a_ImageData);
	form.AddFilePart("parameters", "myparams.json", classifierParams);
    form.Finish();

	Headers headers;
	headers["Content-Type"] = form.GetContentType();
	headers["Accept-Language"] = "en";

	new RequestJson(this, parameters, "POST", headers, form.GetBody(), a_Callback);
}

void VisualRecognition::DetectFaces(const std::string & a_ImageData, OnDetectFaces a_Callback, bool a_bKnowledgeGraph )
{
	std::string parameters = "/v3/detect_faces";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=2016-05-20";
	if (a_bKnowledgeGraph)
		parameters += "&knowledgeGraph=1";

	Headers headers;
	headers["Content-Type"] = "application/x-www-form-urlencoded";

	new RequestJson(this, parameters, "POST", headers, a_ImageData, a_Callback);
}

void VisualRecognition::IdentifyText(const std::string & a_ImageData, OnIdentifyText a_Callback, bool a_bKnowledgeGraph )
{
	std::string parameters = "/v3/recognize_text";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=2016-05-20";
	if (a_bKnowledgeGraph)
		parameters += "&knowledgeGraph=1";

	Headers headers;
	headers["Content-Type"] = "application/x-www-form-urlencoded";

	new RequestJson(this, parameters, "POST", headers, a_ImageData, a_Callback);

}

void VisualRecognition::TrainClassifierPositives(const std::string & a_ImageData, std::string & classifierId, std::string & classifierName, const std::string & imageClass, OnClassifierTrained a_Callback)
{
	if (classifierId == "")
		classifierId = "default";
	if (classifierName == "")
		classifierName = "default";

	std::string parameters = "/v3/classifiers/";
	parameters += classifierId;
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=2016-05-20";

	std::string className = imageClass + "_positive_examples";
	std::string fileName = imageClass + "_positive_examples.jpg";
	
	Form form;
    form.AddFilePart(className, fileName, a_ImageData);
	form.AddFormField("name", classifierName);
    form.Finish();

	Headers headers;
	headers["Content-Type"] = form.GetContentType();

	new RequestJson(this, parameters, "POST", headers, form.GetBody(), a_Callback);
}

// VisualRecognition as a service currently does not fully support this functionality. Leaving here until they do.
void VisualRecognition::TrainClassifierNegatives(const std::string & a_ImageData, std::string & classifierId, std::string & classifierName, const std::string & imageClass, OnClassifierTrained a_Callback)
{
	if (classifierId == "")
		classifierId = "default";
	if (classifierName == "")
		classifierName = "default"; 

	std::string parameters = "/v3/classifiers/";
	parameters += classifierId;
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=2016-05-20";

	std::string className = imageClass + "_negative_examples";
	std::string fileName = imageClass + "_negative_examples.jpg";

	Form form;
    form.AddFilePart(className, fileName, a_ImageData);
	form.AddFormField("name", classifierName);
    form.Finish();

	Headers headers;
	headers["Content-Type"] = form.GetContentType();

	new RequestJson(this, parameters, "POST", headers, form.GetBody(), a_Callback);
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

