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

#include "LanguageTranslation.h"

REG_SERIALIZABLE( LanguageTranslation );
RTTI_IMPL( Translations, ISerializable );
RTTI_IMPL( Translation, ISerializable );
RTTI_IMPL( Languages, ISerializable );
RTTI_IMPL( Language, ISerializable );
RTTI_IMPL( IdentifiedLanguages, ISerializable );
RTTI_IMPL( IdentifiedLanguage, ISerializable );
RTTI_IMPL( LanguageTranslation, IService );

LanguageTranslation::LanguageTranslation() : IService("LanguageTranslation")
{}

void LanguageTranslation::Serialize(Json::Value & json)
{
    IService::Serialize(json);
}

void LanguageTranslation::Deserialize(const Json::Value & json)
{
    IService::Deserialize(json);
}

//! IService interface
bool LanguageTranslation::Start()
{
    if (! IService::Start() )
        return false;

    if (! StringUtil::EndsWith( m_pConfig->m_URL, "language-translation/api" ) )
    {
        Log::Error( "LanguageTranslation", "Configured URL not ended with language-translation/api" );
        return false;
    }

    return true;
}

void LanguageTranslation::GetServiceStatus( ServiceStatusCallback a_Callback )
{
    if (m_pConfig != NULL)
        new ServiceStatusChecker(this, a_Callback);
    else
        a_Callback(ServiceStatus(m_ServiceId, false));
}


void LanguageTranslation::Translation(std::string & a_Source,
                                      std::string & a_Target,
                                      std::string & a_Text,
                                      OnTranslation a_Callback)
{
    new RequestObj<Translations>( this, "/v2/translate", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void LanguageTranslation::IdentifiableLanguages(OnLanguage a_Callback)
{
    new RequestObj<Languages>( this, "/v2/identifiable_languages", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void LanguageTranslation::Identify(std::string & a_Text,
                                   OnIdentifiedLanguages a_Callback)
{
    new RequestObj<IdentifiedLanguages>( this, "/v2/identify", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}
