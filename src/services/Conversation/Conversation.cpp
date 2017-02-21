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

#include "Conversation.h"
#include "utils/JsonHelpers.h"

REG_SERIALIZABLE( Conversation );
RTTI_IMPL( Conversation, IService );
REG_SERIALIZABLE( ConversationResponse );
RTTI_IMPL( ConversationResponse, ISerializable );

Conversation::Conversation() : IService("ConversationV1"), m_APIVersion( "2016-07-11" ), m_CacheTag( "[cache]" )
{}

void Conversation::Serialize(Json::Value & json)
{
    IService::Serialize(json);
	json["m_APIVersion"] = m_APIVersion;
	json["m_CacheTag"] = m_CacheTag;
}

void Conversation::Deserialize(const Json::Value & json)
{
    IService::Deserialize(json);
	if ( json.isMember("m_APIVersion") )
		m_APIVersion = json["m_APIVersion"].asString();
	if ( json.isMember("m_CacheTag") )
		m_CacheTag = json["m_CacheTag"].asString();
}

//! IService interface
bool Conversation::Start()
{
    if (! IService::Start() )
        return false;

    if (! StringUtil::EndsWith( m_pConfig->m_URL, "conversation/api" ) )
    {
        Log::Error( "Conversation", "Configured URL not ended with conversation/api" );
        return false;
    }

    return true;
}


//! Send Question / Statement / Command
void Conversation::Message( const std::string & a_WorkspaceId, 
	const Json::Value & a_Context,
	const std::string & a_Text, 
	const std::string & a_IntentOverrideTag, 
	OnMessage a_Callback,
	bool a_bUseCache /*= true*/ )
{
	new MessageReq( this, a_WorkspaceId, a_Context, a_Text, a_IntentOverrideTag, a_Callback, a_bUseCache );
}

Conversation::MessageReq::MessageReq(Conversation * a_pConversation, 
		const std::string & a_WorkspaceId,
		const Json::Value & a_Context,
		const std::string & a_Text,
		const std::string & a_IntentOverrideTag,
		OnMessage a_Callback,
		bool a_bUseCache ) : 
			m_pConversation( a_pConversation ), 
			m_WorkspaceId( a_WorkspaceId ),
			m_Callback( a_Callback ), 
			m_bUseCache( a_bUseCache )
{
	Json::Value req;
	req["text"] = a_Text;
	req["intentoverride"] = a_IntentOverrideTag;
	Json::Value input;
	input["input"] = req;

	//Log::Status( "Conversation", "Context: %s", a_Context.toStyledString().c_str() );
	// we want to hash in parts of the context to generate a hashID
	if (! a_Context.isNull() && a_Context["system"].isMember( "dialog_stack" ) )
		input["context"]["system"]["dialog_stack"] = a_Context["system"]["dialog_stack"];
	// hash before we add the full context
	m_InputHash = JsonHelpers::Hash( input );			
	// now add in the full context
	if( !a_Context.isNull() )
		input["context"] = a_Context;

	std::string response;
	if (a_bUseCache && m_pConversation->GetCachedResponse(a_WorkspaceId, m_InputHash, response))
	{
		Json::Value items;
		if (Json::Reader(Json::Features::strictMode()).parse(response, items) && items.size() > 0)
		{
			ConversationResponse * pResponse = ISerializable::DeserializeObject<ConversationResponse>( items[rand() % items.size()]["response"] );
			if ( pResponse != NULL )
			{
				// pick a random result and provide that via the callback..
				m_Callback(pResponse);
				// reset now, so the OnResponse() callback doesn't provide a 2nd result..
				m_Callback.Reset();		
			}
		}
	}

	Headers headers;
	headers["Content-Type"] = "application/json";

	std::string params = "/v1/workspaces/" + a_WorkspaceId + "/message?version=" + m_pConversation->m_APIVersion;
	new RequestObj<ConversationResponse>( m_pConversation, params, "POST", headers, input.toStyledString(),
		DELEGATE( MessageReq, OnResponse, ConversationResponse *, this) );
}

static bool ContainsText( const std::vector<std::string> & a_Responses, const std::string & a_Text )
{
	for(size_t i=0;i<a_Responses.size();++i)
		if ( StringUtil::Find( a_Responses[i], a_Text, 0, true ) != std::string::npos )
			return true;

	return false;
}

void Conversation::MessageReq::OnResponse(ConversationResponse * a_pResponse)
{
	// Cache only if m_bUseCache is true AND the response contains the m_CacheTag
	// Only responses explicitly tagged will be cached
	if ( a_pResponse != NULL && m_bUseCache && ContainsText( a_pResponse->m_Output, m_pConversation->m_CacheTag ) )
	{
		DataCache * pCache = m_pConversation->GetDataCache(m_WorkspaceId);
		if (pCache != NULL)
		{
			Time now;
			bool bAppend = true;

			Json::Value response( ISerializable::SerializeObject( a_pResponse ) );
			// save the dialog_stack from the context
			Json::Value dialog_stack( response["context"]["system"]["dialog_stack"] );
			// remove the context then re-add the dialog stack
			response.removeMember( "context" );
			response["context"]["system"]["dialog_stack"] = dialog_stack;

			//Log::Status( "Conversation", "Caching Response: %s", response.toStyledString().c_str() );
			std::string responseHash( JsonHelpers::Hash( response ) );

			Json::Value items;
			DataCache::CacheItem * pItem = pCache->Find(m_InputHash);
			if (pItem != NULL)
			{
				if (!Json::Reader(Json::Features::strictMode()).parse(pItem->m_Data, items))
					Log::Warning("Dialog", "Failed to parse dialog cache item %8.8x.", m_InputHash.c_str() );

				// check for an existing match, if found then don't push it..
				for (size_t i = 0; i < items.size(); ++i)
				{
					if ( JsonHelpers::Hash( items[i]["response"] ) == responseHash )
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
					if (age > m_pConversation->m_MaxCacheAge)
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
				Json::Value newItem;
				newItem["response"] = response;
				newItem["timestamp"] = now.GetEpochTime();
				items.append( newItem );
			}

			pCache->Save(m_InputHash, items.toStyledString());
		}
	}

	if ( m_Callback.IsValid() )
		m_Callback(a_pResponse);
	else
		delete a_pResponse;		// if no callback, then we need to delete the response

	delete this;
}

