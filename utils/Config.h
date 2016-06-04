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

#ifndef WDC_CONFIG_H
#define WDC_CONFIG_H

#include "ISerializable.h"
#include "Library.h"
#include "ServiceConfig.h"
#include "WDCLib.h"

class WDC_API Config : public ISerializable
{
public:
	RTTI_DECL( Config, ISerializable );

	//! Types
	typedef std::list<std::string>		LibraryList;

	//! Singleton
	static Config * Instance();

	Config() 
	{
		sm_pInstance = this;
	}
	~Config()
	{
		if ( sm_pInstance == this )
			sm_pInstance = NULL;
	}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Accessors
	const LibraryList & GetLibraryList() const
	{
		return m_Libs;
	}
	ServiceConfig * FindServiceConfig( const std::string & a_ServiceId ) const
	{
		for(size_t i=0;i<m_ServiceConfigs.size();++i)
			if ( m_ServiceConfigs[i]->m_ServiceId == a_ServiceId )
				return m_ServiceConfigs[i].get();
		return NULL;
	}

	//! Mutators
	bool AddServiceConfig( const ServiceConfig & a_Credential, bool a_bUpdateOnly = false )
	{
		for(size_t i=0;i<m_ServiceConfigs.size();++i)
			if ( m_ServiceConfigs[i]->m_ServiceId == a_Credential.m_ServiceId )
			{
				if ( *m_ServiceConfigs[i] != a_Credential )
				{
					*m_ServiceConfigs[i] = a_Credential;
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
	bool RemoveServiceConfig( const std::string & a_ServiceId )
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

	//! load all dynamic libs
	void LoadLibs();
	//! unload all dynamic libs
	void UnloadLibs();

protected:
	//! Types
	typedef std::list<Library>	LoadedLibraryList;
	typedef std::vector<ServiceConfig::SP> ServiceConfigs;

	//! Data
	LibraryList		m_Libs;				// list of libraries to load dynamically
	ServiceConfigs	m_ServiceConfigs;
	LoadedLibraryList
					m_LoadedLibs;

	static Config *	sm_pInstance;
};

#endif


