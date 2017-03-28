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

#include "LanguageTranslator.h"
#include "utils/Form.h"

REG_SERIALIZABLE( LanguageTranslator );
RTTI_IMPL( Translations, ISerializable );
RTTI_IMPL( Translation, ISerializable );
RTTI_IMPL( Languages, ISerializable );
RTTI_IMPL( Language, ISerializable );
RTTI_IMPL( IdentifiedLanguages, ISerializable );
RTTI_IMPL( IdentifiedLanguage, ISerializable );
RTTI_IMPL( LanguageTranslator, IService );

LanguageTranslator::LanguageTranslator() : IService("LanguageTranslator")
{}

void LanguageTranslator::Serialize(Json::Value & json)
{
    IService::Serialize(json);
}

void LanguageTranslator::Deserialize(const Json::Value & json)
{
    IService::Deserialize(json);
}

//! IService interface
bool LanguageTranslator::Start()
{
    if (! IService::Start() )
        return false;

    if (! StringUtil::EndsWith( m_pConfig->m_URL, "language-translator/api" ) )
    {
        Log::Error( "LanguageTranslator", "Configured URL not ended with language-translatore/api" );
        return false;
    }

    return true;
}

void LanguageTranslator::GetServiceStatus( ServiceStatusCallback a_Callback )
{
    if ( IsConfigured() )
        new ServiceStatusChecker(this, a_Callback);
    else
        a_Callback(ServiceStatus(m_ServiceId, false));
}

//! Creates an object responsible for service status checking
LanguageTranslator::ServiceStatusChecker::ServiceStatusChecker
        (LanguageTranslator* a_pLTService, ServiceStatusCallback a_Callback)
        : m_pLTService(a_pLTService), m_Callback(a_Callback)
{
    m_pLTService->IdentifiableLanguages(DELEGATE(ServiceStatusChecker, OnCheckService, Languages*, this));
}

//! Callback function invoked when service status is checked
void LanguageTranslator::ServiceStatusChecker::OnCheckService(Languages* a_pLanguages)
{
    if (m_Callback.IsValid())
        m_Callback(ServiceStatus(m_pLTService->m_ServiceId, a_pLanguages != NULL));

    delete a_pLanguages;
    delete this;
}


void LanguageTranslator::Translation(const std::string & a_Source,
                                      const std::string & a_Target,
                                      const std::string & a_Text,
                                      OnTranslation a_Callback)
{
    std::string parameters = "/v2/translate";
    parameters += "?source=" + a_Source;
    parameters += "&target=" + a_Target;
    parameters += "&text=" + StringUtil::UrlEscape(a_Text);

    Headers headers;
    headers["Content-Type"] = "application/x-www-form-urlencoded";
    headers["Accept"] = "application/json";

    new RequestObj<Translations>( this, parameters, "GET", headers, EMPTY_STRING, a_Callback,
		new CacheRequest( a_Source + "_" + a_Target, StringHash::DJB(a_Text.c_str()) ) );
}

void LanguageTranslator::IdentifiableLanguages(OnLanguage a_Callback)
{
    new RequestObj<Languages>( this, "/v2/identifiable_languages", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void LanguageTranslator::Identify(const std::string & a_Text,
                                   OnIdentifiedLanguages a_Callback)
{
    Headers headers;
    headers["Content-Type"] = "text/plain";
    headers["Accept"] = "application/json";
    new RequestObj<IdentifiedLanguages>( this, "/v2/identify", "POST", headers, a_Text, a_Callback,
		new CacheRequest( "id_lang", StringHash::DJB( a_Text.c_str() ) ) );
}
