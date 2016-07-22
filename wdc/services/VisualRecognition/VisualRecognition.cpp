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
		return false;	
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

void VisualRecognition::ClassifyImage(const std::string & a_ImageData, OnClassifyImage a_Callback, bool a_bKnowledgeGraph )
{
	std::string parameters = "/v3/classify";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&version=2016-05-20";
	if (a_bKnowledgeGraph)
		parameters += "&knowledgeGraph=1";

	Headers headers;
	headers["Content-Type"] = "application/x-www-form-urlencoded";

	new RequestJson(this, parameters, "POST", headers, a_ImageData, a_Callback);
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

VisualRecognition::ServiceStatusChecker::ServiceStatusChecker(VisualRecognition * a_pService, ServiceStatusCallback a_Callback)
	: m_pService(a_pService), m_Callback(a_Callback)
{
	if (a_Callback.IsValid())
		a_Callback(ServiceStatus(m_pService->m_ServiceId, true));
	delete this;
}

void VisualRecognition::ServiceStatusChecker::OnCheckService(const Json::Value & a_Response)
{}

