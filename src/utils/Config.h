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


#ifndef WDC_CONFIG_H
#define WDC_CONFIG_H

#include "IService.h"
#include "ISerializable.h"
#include "Library.h"
#include "ServiceConfig.h"
#include "UtilsLib.h"

class UTILS_API Config : public ISerializable
{
public:
	RTTI_DECL();

	//! Types
	typedef std::list<std::string>						LibraryList;
	typedef boost::shared_ptr<IService>					IServiceSP;
	typedef std::list<IServiceSP>						ServiceList;

	//! Singleton
	static Config * Instance();

	Config(const std::string & a_staticDataPath = "./etc/",
		const std::string & a_InstanceDataPath = "./" ) : 
		m_StaticDataPath( a_staticDataPath ),
		m_InstanceDataPath( a_InstanceDataPath ),
		m_bServicesActive( false )
	{
		sm_pInstance = this;
	}
	~Config()
	{
		m_Services.clear();
		UnloadLibs();

		if ( sm_pInstance == this )
			sm_pInstance = NULL;
	}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! Accessors
	const std::string & GetStaticDataPath() const
	{
		return m_StaticDataPath;
	}

	const std::string &	GetInstanceDataPath() const
	{
		return m_InstanceDataPath;
	}

	const LibraryList & GetLibraryList() const
	{
		return m_Libs;
	}
	const ServiceList &	GetServiceList() const
	{
		return m_Services;
	}
	ServiceConfig * FindServiceConfig( const std::string & a_ServiceId ) const
	{
		for(size_t i=0;i<m_ServiceConfigs.size();++i)
			if ( m_ServiceConfigs[i]->m_ServiceId == a_ServiceId )
				return m_ServiceConfigs[i].get();
		return NULL;
	}

	bool IsConfigured( const std::string & a_ServiceId, AuthType a_AuthType = AUTH_BASIC ) const
	{
		ServiceConfig * pConfig = FindServiceConfig( a_ServiceId );
		if ( pConfig != NULL )
			return pConfig->IsConfigured( a_AuthType );
		return false;
	}

	//! Returns the given service by it's type.
	template<typename T>
	T * FindService( const std::string & a_ServiceId = EMPTY_STRING ) const
	{
		for (ServiceList::const_iterator iService = m_Services.begin(); iService != m_Services.end(); ++iService)
		{
			T * pService = DynamicCast<T>((*iService).get());
			if ( pService == NULL )
				continue;
			if ( !a_ServiceId.empty() && a_ServiceId != pService->GetServiceId() )
				continue;		// service ID doesn't match
			return pService;
		}

		return NULL;
	}

	//! Returns vector of services
	template<typename T>
	int FindServices(std::vector<T*> & a_Services)
	{
		for (ServiceList::const_iterator iService = m_Services.begin(); iService != m_Services.end(); ++iService)
		{
			T * pService = DynamicCast<T>((*iService).get());
			if (pService != NULL)
				a_Services.push_back(pService);
		}

		return a_Services.size();
	}

	//! Get the given service by type, returns an existing service if found or creates it if needed.
	template<typename T>
	T * GetService()
	{
		T * pService = FindService<T>();
		if (pService == NULL)
		{
			IService::SP spService( new T() );
			if (AddServiceInternal(spService))
				pService = (T *)spService.get();
		}

		return pService;
	}
	bool AddService( const IServiceSP & a_spService )
	{
		return AddServiceInternal( a_spService );
	}
	bool RemoveService( const IServiceSP & a_spService )
	{
		for(ServiceList::iterator iService = m_Services.begin(); iService != m_Services.end(); ++iService )
		{
			if ( (*iService) == a_spService )
			{
				if (! a_spService->Stop() )
					return false;

				m_Services.erase( iService );
				return true;
			}
		}

		return false;
	}

	//! Mutators
	bool AddServiceConfig( const ServiceConfig & a_Credential, bool a_bUpdateOnly = false );
	bool RemoveServiceConfig( const std::string & a_ServiceId );

	//! load all dynamic libs
	virtual void LoadLibs();
	//! unload all dynamic libs
	virtual void UnloadLibs();
	//! Disable the specific library
	virtual bool DisableLib( const std::string & a_Lib );
	//! Enable the specific library
	virtual bool EnableLib( const std::string & a_Lib );
	//! Start all configured services
	virtual bool StartServices();
	//! Stop all configured services
	virtual bool StopServices();

protected:
	//! Types
	typedef std::list<Library *>			LoadedLibraryList;
	typedef std::vector<ServiceConfig::SP>	ServiceConfigs;

	bool AddServiceInternal( const IServiceSP & a_pService);

	//! Data
	std::string		m_StaticDataPath;
	std::string		m_InstanceDataPath;
	LibraryList		m_Libs;				// list of libraries to load dynamically
	LibraryList		m_DisabledLibs;		// libraries that we should not load
	ServiceList		m_Services;			// list of available services
	bool			m_bServicesActive;
	ServiceConfigs	m_ServiceConfigs;
	LoadedLibraryList
					m_LoadedLibs;

	static Config *	sm_pInstance;
};

#endif


