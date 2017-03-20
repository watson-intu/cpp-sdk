/**
* Copyright 2017 IBM Corp. All Rights Reserved.
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

#ifndef SELF_NATURALLANGUAGEUNDERSTANDING_H
#define SELF_NATURALLANGUAGEUNDERSTANDING_H

#include "utils/Delegate.h"
#include "utils/DataCache.h"
#include "services/IService.h"
#include "WDCLib.h"		// include last always

class WDC_API NaturalLanguageUnderstanding : public IService
{
public:
RTTI_DECL();

//! Types
typedef Delegate<const Json::Value &>	OnClassifyImage;
typedef Delegate<const Json::Value &>	OnDetectFaces;

//! Construction
NaturalLanguageUnderstanding();

//! ISerializable
virtual void Serialize(Json::Value & json);
virtual void Deserialize(const Json::Value & json);

//! IService interface
virtual bool Start();

//! Parse the provided text and returns the tags for the parts of the sentence.
void GetEntities(const std::string & a_Text,
                 Delegate<const Json::Value &> a_Callback);
bool FindCity(const Json::Value & json, std::string & a_City);


private:

//!Data
std::vector<std::string>	m_ReturnParameters;
std::string		m_APIVersion;
std::string		m_Language;


};

#endif //SELF_NATURALLANGUAGEUNDERSTANDING_H
