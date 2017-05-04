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
	for( LibraryList::iterator iLib = m_Libs.begin(); iLib != m_Libs.end(); ++iLib )
		json["m_Libs"].append( *iLib );
	for( LibraryList::iterator iLib = m_DisabledLibs.begin(); iLib != m_DisabledLibs.end(); ++iLib )
		json["m_DisabledLibs"].append( *iLib );

	SerializeVector( "m_ServiceConfigs", m_ServiceConfigs, json, false );
	SerializeList("m_Services", m_Services, json);
}

void Config::Deserialize(const Json::Value & json)
{
	m_Services.clear();
	m_Libs.clear();
	m_DisabledLibs.clear();

	for( Json::ValueConstIterator iObject = json["m_Libs"].begin(); iObject != json["m_Libs"].end(); ++iObject )
		m_Libs.push_back( iObject->asString() );
	for( Json::ValueConstIterator iObject = json["m_DisabledLibs"].begin(); iObject != json["m_DisabledLibs"].end(); ++iObject )
		m_DisabledLibs.push_back( iObject->asString() );

	LoadLibs();

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
					const IService::SP & spService = *iService;
					if (! spService )
						continue;
					if ( spService->GetServiceId() == a_Credential.m_ServiceId )
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
	{
		if ( std::find( m_DisabledLibs.begin(), m_DisabledLibs.end(), *iLib) != m_DisabledLibs.end() )
			continue;		// library is disabled
		m_LoadedLibs.push_back( new Library( *iLib ) );
	}
}

void Config::UnloadLibs()
{
	for( LoadedLibraryList::iterator iLib = m_LoadedLibs.begin(); iLib != m_LoadedLibs.end(); ++iLib )
		delete *iLib;
	m_LoadedLibs.clear();
}

bool Config::DisableLib( const std::string & a_Lib )
{
	if ( std::find( m_DisabledLibs.begin(), m_DisabledLibs.end(), a_Lib ) != m_DisabledLibs.end() )
		return true;		// already disabled

	for( LoadedLibraryList::iterator iLib = m_LoadedLibs.begin(); iLib != m_LoadedLibs.end(); ++iLib )
	{
		Library * pLib = *iLib;
		if ( pLib->GetLibraryName() == a_Lib )
		{
			// try to unload this lib, this will fail if any objects are still created..
			if (! pLib->Unload() )
			{
				Log::Status( "Config", "Cannot disable library %s, cannot be unloaded.", a_Lib.c_str() );
				return false;
			}

			m_DisabledLibs.push_back( a_Lib );

			delete pLib;
			m_LoadedLibs.erase( iLib );
			return true;
		}
	}

	Log::Warning( "Config", "Failed to find library %s to disable.", a_Lib.c_str() );
	return false;
}

bool Config::EnableLib( const std::string & a_Lib )
{
	LibraryList::iterator iDisabled = std::find( m_DisabledLibs.begin(), m_DisabledLibs.end(), a_Lib );
	if ( iDisabled == m_DisabledLibs.end() )
		return false;	// library is not disabled

	m_DisabledLibs.erase( iDisabled );
	m_LoadedLibs.push_back( new Library( a_Lib ) );

	return true;
}

bool Config::StartServices()
{
	if ( m_bServicesActive )
		return false;

	m_bServicesActive = true;
	for (ServiceList::iterator iService = m_Services.begin(); iService != m_Services.end(); )
	{
		IService * pService = (*iService).get();
		if (pService == NULL)
		{
			Log::Warning("Config", "NULL service in body definition.");
			m_Services.erase( iService++ );
			continue;
		}

		if ( pService->IsEnabled() && !pService->Start())
			Log::Warning("Config", "Failed to start service %s.", pService->GetRTTI().GetName().c_str());

		++iService;
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


bool Config::AddServiceInternal( const IServiceSP & a_spService)
{
	if (! a_spService )
		return false;

	if (m_bServicesActive)
	{
		if ( a_spService->IsEnabled() && !a_spService->Start())
			Log::Warning("Config", "Failed to start service %s.", a_spService->GetRTTI().GetName().c_str());
	}

	m_Services.push_back(a_spService);
	return true;
}

