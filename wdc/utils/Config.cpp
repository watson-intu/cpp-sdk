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

#include "Config.h"

RTTI_IMPL( Config, ISerializable );
RTTI_IMPL( ServiceConfig, ISerializable );

Config * Config::sm_pInstance = NULL;

Config * Config::Instance()
{
	return sm_pInstance;
}

void Config::Serialize(Json::Value & json)
{
	int index = 0;
	for( LibraryList::iterator iLib = m_Libs.begin(); iLib != m_Libs.end(); ++iLib )
		json["m_Libs"][index++] = *iLib;
	
	SerializeVector( "m_ServiceConfigs", m_ServiceConfigs, json, false );
}

void Config::Deserialize(const Json::Value & json)
{
	m_Libs.clear();
	for( Json::ValueConstIterator iObject = json["m_Libs"].begin(); iObject != json["m_Libs"].end(); ++iObject )
		m_Libs.push_back( iObject->asString() );

	DeserializeVectorNoType( "m_ServiceConfigs", json, m_ServiceConfigs );
}

void Config::LoadLibs()
{
	UnloadLibs();

	for( LibraryList::iterator iLib = m_Libs.begin(); iLib != m_Libs.end(); ++iLib )
		m_LoadedLibs.push_back( Library( *iLib ) );
}

void Config::UnloadLibs()
{
	m_LoadedLibs.clear();
}
