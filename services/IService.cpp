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

#include "IService.h"
#include "base64/encode.h"
#include "utils/Config.h"
#include "utils/StringUtil.h"
#include "utils/Time.h"

#undef MAX
#define MAX(a,b)		((a) > (b) ? (a) : (b))

IService::Request::Request(const std::string & a_URL,
	const std::string & a_RequestType,		// type of request GET, POST, DELETE
	const Headers & a_Headers,				// additional headers to add to the request
	const std::string & a_Body,				// the body to send if any
	ResponseCallback a_Callback,
	float a_fTimeout /*= 30.0f*/ ) :
	m_pService(NULL),
	m_pClient( IWebClient::Create() ),
	m_Body(a_Body),
	m_Complete(false),
	m_Error(false),
	m_Callback(a_Callback),
	m_CreateTime(Time().GetEpochTime()),
	m_StartTime(0.0),
	m_bDelete(false),
	m_pCachedReq(NULL)
{
	m_pClient->SetURL( a_URL );
	m_pClient->SetRequestType( a_RequestType );
	m_pClient->SetStateReceiver( DELEGATE( Request, OnState, IWebClient *, this ) );
	m_pClient->SetDataReceiver( DELEGATE( Request, OnResponseData, IWebClient::RequestData *, this ) );
	m_pClient->SetHeaders( a_Headers );
	m_pClient->SetBody( a_Body );

	//Log::Debug( "Request", "Sending request '%s'", a_URL.c_str() );
	if (! m_pClient->Send() )
	{
		m_Error = true;
		Log::Error("Request", "Failed to send web request.");
		ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(Request, OnLocalResponse, this));
	}
	else if ( TimerPool::Instance() != NULL )
	{
		m_spTimeoutTimer = TimerPool::Instance()->StartTimer( 
			VOID_DELEGATE( Request, OnTimeout, this ), a_fTimeout, true, false );
	}
	else
	{
		Log::Warning( "IService", "No TimerPool instance, timeouts are disabled." );
	}
}

IService::Request::Request( IService * a_pService,
	const std::string & a_EndPoint,
	const std::string & a_RequestType, 
	const Headers & a_Headers, 
	const std::string & a_Body,
	ResponseCallback a_Callback,
	CacheRequest * a_CacheReq/* = NULL*/,
	float a_fTimeout /*= 30.0f*/ ) :
	m_pService( a_pService ),
	m_pClient(NULL),
	m_Body( a_Body ),
	m_Complete( false ),
	m_Error( false ),
	m_Callback( a_Callback ),
	m_CreateTime( Time().GetEpochTime() ),
	m_StartTime( 0.0 ),
	m_bDelete(false),
	m_pCachedReq(a_CacheReq)
{
	m_pService->m_RequestsPending += 1;

	// firstly, check for a cached response, invoke the callback immediately if one is found.
	if (m_pCachedReq != NULL && m_pService->GetCachedResponse(m_pCachedReq->m_CacheName, m_pCachedReq->m_Id, m_Response))
	{
		// we have a cache response, but push a callback into the main queue so we can return 
		// and continue construction of this request object. If we try to invoke the callback
		// then try to destroy this object, very likely we will crash because we are still in
		// the middle of construction.
		ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(Request, OnLocalResponse, this));
		return;
	}

	m_pClient = IWebClient::Create();
	m_pClient->SetURL( a_pService->GetConfig()->m_URL + a_EndPoint );
	m_pClient->SetRequestType( a_RequestType );
	m_pClient->SetStateReceiver( DELEGATE( Request, OnState, IWebClient *, this ) );
	m_pClient->SetDataReceiver( DELEGATE( Request, OnResponseData, IWebClient::RequestData *, this ) );
	m_pClient->SetHeaders( a_pService->GetHeaders() );
	m_pClient->SetHeaders( a_Headers, true );
	m_pClient->SetBody( a_Body );

	//Log::Debug( "Request", "Sending request '%s'", a_pService->GetConfig()->m_URL + a_EndPoint.c_str() );
	if (! m_pClient->Send() )
	{
		m_Error = true;
		Log::Error( "Request", "Failed to send web request." );
		ThreadPool::Instance()->InvokeOnMain(VOID_DELEGATE(Request, OnLocalResponse, this));
	}
	else if ( TimerPool::Instance() != NULL )
	{
		float fTimeout = MAX(a_fTimeout, m_pService->m_RequestTimeout);
		m_spTimeoutTimer = TimerPool::Instance()->StartTimer( 
			VOID_DELEGATE( Request, OnTimeout, this ), fTimeout, true, false );
	}
	else
	{
		Log::Warning( "IService", "No TimerPool instance, timeouts are disabled." );
	}
}

void IService::Request::OnState( IWebClient * a_pClient )
{
	if ( a_pClient->GetState() == IWebClient::CLOSED )
	{
		//Log::Debug( "Request", "Request closed, delete this." );
		m_bDelete = true;
	}
	else if ( a_pClient->GetState() == IWebClient::CONNECTING )
	{
		//Log::Debug( "Request", "Request connecting." );
		m_StartTime = Time().GetEpochTime();
	}
	else if ( a_pClient->GetState() == IWebClient::DISCONNECTED )
	{
		Log::Error( "Request", "Request failed to connect." );
		m_Error = true;
		m_bDelete = true;
		if ( m_Callback.IsValid() )
		{
			m_Callback( this );
			m_Callback.Reset();
			if ( m_pService != NULL )
				m_pService->m_RequestsPending -= 1;
		}
	}

	// delete needs to be the last thing we do..
	if ( m_bDelete )
		delete this;
}

void IService::Request::OnResponseData( IWebClient::RequestData * a_pResponse )
{
	m_Complete = true;
	m_Error = a_pResponse->m_StatusCode < 200 || a_pResponse->m_StatusCode >= 300;

	double end = Time().GetEpochTime();
	Log::DebugMed( "Request", "REST request %s completed in %g seconds. Queued for %g seconds. Status: %d.", 
		m_pClient->GetURL().GetURL().c_str(), end - m_StartTime, m_StartTime - m_CreateTime, a_pResponse->m_StatusCode );

	m_Response.swap( a_pResponse->m_Content );
	if (m_pCachedReq != NULL && m_pService != NULL && !m_Error)
		m_pService->PutCachedResponse(m_pCachedReq->m_CacheName, m_pCachedReq->m_Id, m_Response);

	if ( m_Error )
		Log::Error( "Request", "Request Error %u: %s", a_pResponse->m_StatusCode, m_Response.c_str() );

	if ( m_Callback.IsValid() )
	{
		m_Callback( this );
		m_Callback.Reset();
		if ( m_pService != NULL )
			m_pService->m_RequestsPending -= 1;
	}
	// note the OnState() change will take care of deleting this object.
}

void IService::Request::OnLocalResponse()
{
	m_Complete = true;
	if (m_Callback.IsValid())
	{
		m_Callback(this);
		m_Callback.Reset();

		if ( m_pService != NULL )
			m_pService->m_RequestsPending -= 1;
	}
	delete this;
}

void IService::Request::OnTimeout()
{
	Log::Error( "Request", "REST request %s timed out.", m_pClient->GetURL().GetURL().c_str() );

	m_Complete = true;
	m_Error = true;
	m_spTimeoutTimer.reset();

	// closing will call OnState() which will actually take care of deleteing this object.
	if ( m_pClient->Close() )
	{
		if (m_Callback.IsValid())
		{
			m_Callback(this);
			m_Callback.Reset();

			if ( m_pService != NULL )
				m_pService->m_RequestsPending -= 1;
		}
	}
}

IService::IService(const std::string & a_ServiceId) : 
	m_ServiceId(a_ServiceId), 
	m_pConfig(NULL),
	m_bCacheEnabled(true),
	m_MaxCacheSize( 5 * 1024 * 1024 ),
	m_MaxCacheAge( 7 * 24 ),
	m_RequestTimeout( 30.0f ),
	m_RequestsPending( 0 )
{}

bool IService::Start()
{
	Config * pConfig = Config::Instance();
	if ( pConfig == NULL )
	{
		Log::Error( "IService", "No config instance." );
		return false;
	}

	m_pConfig = pConfig->FindServiceConfig( m_ServiceId );
	if ( m_pConfig == NULL )
	{
		Log::Error( "IService", "No credentials found for service %s.", m_ServiceId.c_str() );
		return false;
	}

	if ( m_pConfig->m_User.size() > 0 && m_pConfig->m_Password.size() > 0 )
	{
		// add the Authorization header..
		m_Headers["Authorization"] = StringUtil::Format( "Basic %s", 
			StringUtil::EncodeBase64( m_pConfig->m_User + ":" + m_pConfig->m_Password).c_str() );
	}

	return true;
}

bool IService::Stop()
{
	// wait for all pending request to get completed..
	Time startTime;
	while( m_RequestsPending > 0 && (Time().GetEpochTime() - startTime.GetEpochTime()) < 30.0 )
	{
		ThreadPool::Instance()->ProcessMainThread();
		boost::this_thread::sleep( boost::posix_time::milliseconds(5) );
	}

	m_DataCache.clear();
	return true;
}

void IService::Serialize(Json::Value & json)
{
	json["m_ServiceId"] = m_ServiceId;
	json["m_bCacheEnabled"] = m_bCacheEnabled;
	json["m_MaxCacheSize"] = m_MaxCacheSize;
	json["m_MaxCacheAge"] = m_MaxCacheAge;
	json["m_RequestTimeout"] = m_RequestTimeout;
}

void IService::Deserialize(const Json::Value & json)
{
	if (json.isMember("m_ServiceId"))
		m_ServiceId = json["m_ServiceId"].asString();
	if (json.isMember("m_bCacheEnabled"))
		m_bCacheEnabled = json["m_bCacheEnabled"].asBool();
	if (json.isMember("m_MaxCacheSize"))
		m_MaxCacheSize = json["m_MaxCacheSize"].asUInt();
	if (json.isMember("m_MaxCacheAge"))
		m_MaxCacheAge = json["m_MaxCacheAge"].asDouble();
	if (json.isMember("m_RequestTimeout"))
		m_RequestTimeout = json["m_RequestTimeout"].asFloat();
}

//! Default implementation of the method, always returns false.
//! Interface implementations need to override the method to provide meaningful status
void IService::GetServiceStatus(ServiceStatusCallback a_Callback)
{
	if (a_Callback.IsValid())
		a_Callback(ServiceStatus(m_ServiceId, true));
}

DataCache * IService::GetDataCache(const std::string & a_Type)
{
	if (!m_bCacheEnabled)
		return NULL;

	DataCache & cache = m_DataCache[a_Type];
	if (!cache.IsInitialized())
	{
		if (!cache.Initialize("./cache/" + m_ServiceId + "_" + a_Type + "/", m_MaxCacheSize, m_MaxCacheAge))
		{
			Log::Error("IService", "Failed to initialize the cache.");
			return NULL;
		}
	}

	return &cache;
}

bool IService::GetCachedResponse(const std::string & a_CacheName, const std::string & a_Id, std::string & a_Response)
{
	DataCache * pCache = GetDataCache(a_CacheName);
	if (pCache == NULL)
		return false;
	DataCache::CacheItem * pItem = pCache->Find(a_Id);
	if (pItem == NULL)
		return false;

	a_Response = pItem->m_Data;
	return true;
}

bool IService::GetCachedResponse(const std::string & a_CacheName, unsigned int a_Id, std::string & a_Response)
{
	DataCache * pCache = GetDataCache(a_CacheName);
	if (pCache == NULL)
		return false;
	DataCache::CacheItem * pItem = pCache->Find(a_Id);
	if (pItem == NULL)
		return false;

	a_Response = pItem->m_Data;
	return true;
}

void IService::PutCachedResponse(const std::string & a_CacheName,
	const std::string & a_Id,
	const std::string & a_Response)
{
	DataCache * pCache = GetDataCache(a_CacheName);
	if (pCache != NULL)
	{
		if (!pCache->Save(a_Id, a_Response))
			Log::Warning("IService", "Failed to save %s to cache %s.", a_Id.c_str(), a_CacheName.c_str());
	}
}
