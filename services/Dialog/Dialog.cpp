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

#include <fstream>

#include "Dialog.h"
#include "utils/Form.h"
#include "utils/Path.h"

REG_SERIALIZABLE( Dialog );
RTTI_IMPL( DialogEntry, ISerializable );
RTTI_IMPL( Dialogs, ISerializable );
RTTI_IMPL( Dialog, IService );


Dialog::Dialog() : IService( "DialogV1" ),
	m_MaxDialogAge( 30 * 24 )
{}

void Dialog::Serialize(Json::Value & json)
{
	IService::Serialize( json );

	json["m_MaxDialogAge"]  = m_MaxDialogAge;
}

void Dialog::Deserialize(const Json::Value & json)
{
	IService::Deserialize( json );

	if ( json.isMember("m_MaxDialogAge") )
		m_MaxDialogAge = json["m_MaxDialogAge"].asDouble();
}

bool Dialog::Start()
{
	if (! IService::Start() )
		return false;

	if (! StringUtil::EndsWith( m_pConfig->m_URL, "dialog/api" ) )
	{
		Log::Error( "Dialog", "Configured URL not ended with dialog/api" );
		return false;
	}

	return true;
}

void Dialog::GetServiceStatus( ServiceStatusCallback a_Callback )
{
	if (m_pConfig != NULL)
		new ServiceStatusChecker(this, a_Callback);
	else
		a_Callback(ServiceStatus(m_ServiceId, false));
}

//! Creates an object responsible for service status checking
Dialog::ServiceStatusChecker::ServiceStatusChecker(Dialog* a_pDlgService, ServiceStatusCallback a_Callback)
	: m_pDlgService(a_pDlgService), m_Callback(a_Callback)
{
	m_pDlgService->GetDialogs(DELEGATE(ServiceStatusChecker, OnCheckService, Dialogs*, this));
}

//! Callback function invoked when service status is checked
void Dialog::ServiceStatusChecker::OnCheckService(Dialogs* a_pDialogs)
{
	if (m_Callback.IsValid())
		m_Callback(ServiceStatus(m_pDlgService->m_ServiceId, a_pDialogs != NULL));

	delete a_pDialogs;
	delete this;
}

void Dialog::GetDialogs(OnGetDialogs a_Callback)
{
	new RequestObj<Dialogs>( this, "/v1/dialogs", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void Dialog::DownloadDialog(const std::string & a_DialogId, OnDownloadDialog a_Callback, DialogFormat a_Format /*= XML*/)
{
	Headers headers;
	if (a_Format == XML)
		headers["Accept"] = "application/wds+xml";
	else if (a_Format == JSON)
		headers["Accept"] = "application/wds+json";
	else
		headers["Accept"] = "application/octet-stream";

	new RequestData( this, "/v1/dialogs/" + a_DialogId, "GET", headers, EMPTY_STRING, a_Callback );
}

bool Dialog::UploadDialog( const std::string & a_DialogName,
	const std::string & a_DialogFilePath,
	OnUploadDialog a_Callback )
{
	// get just the filename part..
    std::string filename( StringUtil::GetFilename(a_DialogFilePath));

	// read in all the file data..
	std::ifstream input(a_DialogFilePath.c_str(), std::ios::in | std::ios::binary);
	if (!input.is_open()) 
	{
		Log::Error( "Dialog", "Failed to open input file %s.", a_DialogFilePath.c_str() );
		return false;
	}

	std::string fileData;
	fileData.assign( std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() );

	UploadDialog( a_DialogName, filename, fileData, a_Callback );
	return true;
}

void Dialog::UploadDialog( const std::string & a_DialogName,
	const std::string & a_DialogFileName, 
	const std::string & a_DialogData,  
	OnUploadDialog a_Callback )
{
	Form form;
	form.AddFormField( "name", a_DialogName );
	form.AddFilePart( "file", a_DialogFileName, a_DialogData );
	form.Finish();

	Headers headers;
	headers["Content-Type"] = form.GetContentType();

	new RequestJson( this, "/v1/dialogs", "POST", headers, form.GetBody(), a_Callback, NULL, 10 * 60.0f );
}

void Dialog::DeleteDialog( const std::string & a_DialogId, OnDeleteDialog a_Callback )
{
	new RequestJson( this, "/v1/dialogs/" + a_DialogId, "DELETE", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void Dialog::Converse(
	const std::string & a_DialogId, 
	const std::string & a_Input,
	OnConverse a_Callback,
	int a_ConversationId /*= 0*/,
	int a_ClientId /*= 0*/,
	bool a_bUseCache /*= true*/ )
{
	new ConverseReq(this, a_DialogId, a_Input, a_Callback, a_ConversationId, a_ClientId, a_bUseCache);
}

Dialog::ConverseReq::ConverseReq(Dialog * a_pDialog,
	const std::string & a_DialogId,
	const std::string & a_Input,
	OnConverse a_Callback,
	int a_ConversationId /*= 0*/,
	int a_ClientId /*= 0*/,
	bool a_bUseCache /*= true */) : 
	m_pDialog( a_pDialog ), 
	m_DialogId( a_DialogId ),
	m_InputHash( StringHash::DJB( a_Input.c_str() ) ),
	m_Callback( a_Callback ),
	m_bUseCache( a_bUseCache )
{
	std::string response;
	if (m_bUseCache && m_pDialog->GetCachedResponse(m_DialogId, m_InputHash, response))
	{
		Json::Value items;
		if (Json::Reader(Json::Features::strictMode()).parse(response, items) && items.size() > 0)
		{
			// pick a random result and provide that via the callback..
			m_Callback(items[rand() % items.size()]);
			m_Callback.Reset();		// reset now, so the OnResponse() callback doesn't provide a 2nd result..
		}
	}

	std::string params = "/v1/dialogs/" + a_DialogId + "/conversation";

	std::string body = "input=" + StringUtil::UrlEscape(a_Input);
	if (a_ConversationId != 0)
		body += "&conversation_id=" + StringUtil::Format("%u", a_ConversationId);
	if (a_ClientId != 0)
		body += "&client_id=" + StringUtil::Format("%d", a_ClientId);

	Headers headers;
	headers["Content-Type"] = "application/x-www-form-urlencoded; charset=UTF-8";

	new RequestJson(m_pDialog, params, "POST", headers, body,
		DELEGATE( ConverseReq, OnResponse, const Json::Value &, this ) );
}

static bool CompareArrays(const Json::Value & a_A, const Json::Value & a_B)
{
	if (a_A.size() != a_B.size())
		return false;
	for (size_t i = 0; i < a_A.size(); ++i)
	{
		if (a_A[i].asString().compare( a_B[i].asString() ) != 0 )
			return false;
	}
	return true;
}

void Dialog::ConverseReq::OnResponse(const Json::Value & a_Response)
{
	if (!a_Response.isNull() && m_bUseCache )
	{
		DataCache * pCache = m_pDialog->GetDataCache(m_DialogId);
		if (pCache != NULL)
		{
			bool bAppend = true;

			Time now;
			Json::Value items;
			DataCache::CacheItem * pItem = pCache->Find(m_InputHash);
			if (pItem != NULL)
			{
				if (!Json::Reader(Json::Features::strictMode()).parse(pItem->m_Data, items))
					Log::Warning("Dialog", "Failed to parse dialog cache item %8.8x.", m_InputHash);

				// check for an existing match, if found then don't push it..
				for (size_t i = 0; i < items.size(); ++i)
				{
					if (CompareArrays( items[i]["response"], a_Response["response"] ) )
					{
						items[i]["timestamp"] = now.GetEpochTime();
						bAppend = false;
						break;
					}
				}

				// remove expired items..
				for (size_t i = 0; i < items.size();)
				{
					double age = (now.GetEpochTime() - items[i]["timestamp"].asDouble()) / 3600.0;
					if (age > m_pDialog->m_MaxDialogAge)
					{
						// swap with the end, this avoids moving all the elements down..
						if (i < (items.size() - 1))
							items[i] = items[items.size() - 1];
						items.resize(items.size() - 1);
					}
					else
						i += 1;		// next item..
				}
			}

			if (bAppend)
			{
				items.append(a_Response);
				items[items.size() - 1]["timestamp"] = now.GetEpochTime();
			}

			pCache->Save(m_InputHash, items.toStyledString());
		}
	}

	if ( m_Callback.IsValid() )
		m_Callback(a_Response);
	delete this;
}

