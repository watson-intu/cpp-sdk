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

#ifndef WDC_PROFILE_DATAMODELS_H
#define WDC_PROFILE_DATAMODELS_H

#include "utils/ISerializable.h"

struct WDC_API Trait : public ISerializable
{
    std::string m_TraitId;
    std::string m_Name;
    std::string m_Category;
    double m_Percentile;
    std::vector<Trait> m_Children;

    virtual void Serialize(Json::Value & json)
    {
        json["trait_id"] = m_TraitId;
        json["name"] = m_Name;
        json["category"] = m_Category;
        json["percentile"] = m_Percentile;
        SerializeVector("children", m_Children, json);
    }

    virtual void Deserialize(const Json::Value & json)
    {
        if( json.isMember("trait_id") )
            m_TraitId = json["trait_id"].asString();
        if( json.isMember("name") )
            m_Name = json["name"].asString();
        if( json.isMember("category") )
            m_Category = json["category"].asString();
        if( json.isMember("percentile") )
            m_Percentile = json["percentile"].asDouble();
        if( json.isMember("children") )
            DeserializeVector("children", json, m_Children);
    }
};

struct WDC_API ConsumptionPreference : public ISerializable
{
    std::string m_ConsumptionPreferenceId;
    std::string m_Name;
    double m_Score;

    virtual void Serialize(Json::Value & json)
    {
        json["consumption_preference_id"] = m_ConsumptionPreferenceId;
        json["name"] = m_Name;
        json["score"] = m_Score;
    }

    virtual void Deserialize(const Json::Value & json)
    {
        if( json.isMember("consumption_preference_id") )
            m_ConsumptionPreferenceId = json["consumption_preference_id"].asString();
        if( json.isMember("name") )
            m_Name = json["name"].asString();
        if( json.isMember("score") )
            m_Score = json["score"].asDouble();
    }
};

struct WDC_API ConsumptionPreferenceCategory : public ISerializable
{
    std::string m_ConsumptionPreferenceCategoryId;
    std::string m_Name;
    std::vector<ConsumptionPreference> m_ConsumptionPreferences;

    virtual void Serialize(Json::Value & json)
    {
        json["consumption_preference_category_id"] = m_ConsumptionPreferenceCategoryId;
        json["name"] = m_Name;
        SerializeVector("consumption_preferences", m_ConsumptionPreferences, json);
    }

    virtual void Deserialize(const Json::Value & json)
    {
        if( json.isMember("consumption_preference_category_id") )
            m_ConsumptionPreferenceCategoryId = json["consumption_preference_category_id"].asString();
        if( json.isMember("name") )
            m_Name = json["name"].asString();

        DeserializeVector("consumption_preferences", json, m_ConsumptionPreferences);
    }
};

struct WDC_API Profile : public ISerializable
{
    RTTI_DECL();

    double m_WordCount;
    std::string m_WordCountMessage;
    std::string m_ProcessedLanguage;
    std::vector<Trait> m_Personality;
    std::vector<Trait> m_Needs;
    std::vector<Trait> m_Values;
    std::vector<ConsumptionPreferenceCategory> m_ConsumptionPreferences;

    virtual void Serialize(Json::Value & json)
    {
        json["word_count"] = m_WordCount;
        json["word_count_message"] = m_WordCountMessage;
        json["processed_language"] = m_ProcessedLanguage;

        SerializeVector("personality", m_Personality, json);
        SerializeVector("needs", m_Needs, json);
        SerializeVector("values", m_Values, json);
        SerializeVector("consumption_preferences", m_ConsumptionPreferences, json);
    }

    virtual void Deserialize(const Json::Value & json)
    {
        if( json.isMember("word_count"))
            m_WordCount = json["word_count"].asDouble();
        if( json.isMember("word_count_message"))
            m_WordCountMessage = json["word_count_message"].asString();
        if( json.isMember("processed_language"))
            m_ProcessedLanguage = json["processed_language"].asString();

        DeserializeVector("personality",json, m_Personality);
        DeserializeVector("needs",json, m_Needs);
        DeserializeVector("values",json, m_Values);
        DeserializeVector("consumption_preferences",json, m_ConsumptionPreferences);

    }
};

#endif //WDC_PROFILE_DATAMODELS_H
