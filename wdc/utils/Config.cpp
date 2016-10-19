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

	LoadLibs();
	
	SerializeVector( "m_ServiceConfigs", m_ServiceConfigs, json, false );
	SerializeList("m_Services", m_Services, json);
}

void Config::Deserialize(const Json::Value & json)
{
	m_Libs.clear();
	for( Json::ValueConstIterator iObject = json["m_Libs"].begin(); iObject != json["m_Libs"].end(); ++iObject )
		m_Libs.push_back( iObject->asString() );

	DeserializeVectorNoType( "m_ServiceConfigs", json, m_ServiceConfigs );
	DeserializeList("m_Services", json, m_Services);
}

bool Config::AddServiceConfig( const ServiceConfig & a_Credential, bool a_bUpdateOnly/* = false*/ )
{
	for(size_t i=0;i<m_ServiceConfigs.size();++i)
		if ( m_ServiceConfigs[i]->m_ServiceId == a_Credential.m_ServiceId )
		{
			if ( *m_ServiceConfigs[i] != a_Credential )
			{
				*m_ServiceConfigs[i] = a_Credential;

				// notify all services using this ID that the configuration has been modified
				for( ServiceList::iterator iService = m_Services.begin(); 
					iService != m_Services.end(); ++iService )
				{
					if ( (*iService)->GetServiceId() == a_Credential.m_ServiceId )
						(*iService)->OnConfigModified();
				}
				return true;
			}

			// no changes...
			return false;
		}

	if ( a_bUpdateOnly )
		return false;
	m_ServiceConfigs.push_back( ServiceConfig::SP( new ServiceConfig( a_Credential ) ) );
	return true;
}

bool Config::RemoveServiceConfig( const std::string & a_ServiceId )
{
	for(size_t i=0;i<m_ServiceConfigs.size();++i)
	{
		if ( m_ServiceConfigs[i]->m_ServiceId == a_ServiceId )
		{
			m_ServiceConfigs.erase( m_ServiceConfigs.begin() + i );
			return true;
		}
	}
	return false;
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

bool Config::StartServices()
{
	if ( m_bServicesActive )
		return false;
	m_bServicesActive = true;
	for (ServiceList::const_iterator iService = m_Services.begin(); iService != m_Services.end(); ++iService)
	{
		IService * pService = (*iService).get();
		if (pService == NULL)
		{
			Log::Warning("Config", "NULL service in body definition.");
			continue;
		}

		if (!pService->Start())
		{
			Log::Error("Config", "Failed to start service %s.", pService->GetRTTI().GetName().c_str());
			return false;
		}
	}
	return true;
}

bool Config::StopServices()
{
	if (! m_bServicesActive )
		return false;

	for (ServiceList::const_iterator iService = m_Services.begin(); iService != m_Services.end(); ++iService)
	{
		IService * pService = (*iService).get();
		if (pService == NULL)
			continue;

		if (!pService->Stop())
			Log::Error("Config", "Failed to stop service %s.", pService->GetRTTI().GetName().c_str());
	}
	m_bServicesActive = false;

	return true;
}


bool Config::AddServiceInternal(IService * a_pService)
{
	if (a_pService == NULL)
		return false;

	if (m_bServicesActive)
	{
		if (!a_pService->Start())
		{
			Log::Error("SelfBody", "Failed to start service %s.", a_pService->GetRTTI().GetName().c_str());
			return false;
		}
	}

	m_Services.push_back(IService::SP(a_pService));
	return true;
}

