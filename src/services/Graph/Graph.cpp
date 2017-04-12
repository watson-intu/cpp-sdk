/* ***************************************************************** */
/*                                                                   */
/* IBM Confidential                                                  */
/* OCO Source Materials                                              */
/*                                                                   */
/* (C) Copyright IBM Corp. 2001, 2014                                */
/*                                                                   */
/* The source code for this program is not published or otherwise    */
/* divested of its trade secrets, irrespective of what has been      */
/* deposited with the U.S. Copyright Office.                         */
/*                                                                   */
/* ***************************************************************** */

#include "Graph.h"
#include "utils/JsonHelpers.h"

const float LOG_STATUS_INTERVAL = 2.0f;

REG_SERIALIZABLE( Graph );
RTTI_IMPL( Graph, IService );


Graph::Graph() : IService("GraphV1"), 
	m_bReady( false ),
	m_bError( false ),
	m_nPendingOps( 0 ),
	m_nMaxConcurrent( 5 ),
	m_nActiveRequests( 0 )
{}

//! ISerializable
void Graph::Serialize(Json::Value & json)
{
	IService::Serialize(json);
	json["m_nMaxConcurrent"] = m_nMaxConcurrent;
}

void Graph::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
	if ( json["m_nMaxConcurrent"].isNumeric() )
		m_nMaxConcurrent = json["m_nMaxConcurrent"].asInt();
}

//! IService interface
bool Graph::Start()
{
	if (!IService::Start())
		return false;

	// grab a session token firstly, so our queries will be faster.
	GetSessionToken( DELEGATE( Graph, OnSessionToken, const Json::Value &, this ) );

	// wait for this service to be ready..
	while(! m_bReady && !m_bError )
	{
		ThreadPool::Instance()->ProcessMainThread();
		boost::this_thread::sleep( boost::posix_time::milliseconds( 5 ) );
	}
	
	if ( m_bError )
		return false;

	TimerPool * pTimers = TimerPool::Instance();
	if ( pTimers != NULL )
		m_spLogStatus = pTimers->StartTimer( VOID_DELEGATE( Graph, OnLogStatus, this ), LOG_STATUS_INTERVAL, true, true );

	Log::Debug("Graph", "Graph service running: %s", m_pConfig->m_URL.c_str() );
	return true;
}

bool Graph::Stop()
{
	if (! IService::Stop() )
		return false;

	m_bReady = false;
	m_bError = false;
	m_spLogStatus.reset();

	return true;
}

Graph::Schema * Graph::GetSchema( const std::string & a_GraphId )
{
	SchemaMap::iterator iSchema = m_SchemaMap.find( a_GraphId );
	if ( iSchema == m_SchemaMap.end() )
		return NULL;

	return &iSchema->second;
}

void Graph::GetSessionToken(
	Delegate<const Json::Value &> a_Callback )
{
	// make sure we have the basic authorization header..
	AddAuthenticationHeader();
	// this end-point is probably IBM graph specific, even if this fails we 
	// can continue to use basic authentication if it does.
	new RequestJson( this, "/_session", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

//! Get all available graphs
void Graph::GetGraphs( Delegate<const Json::Value &> a_Callback )
{
	new RequestJson( this, "/_graphs", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void Graph::CreateGraph( const std::string & a_Graph,
	Delegate<const Json::Value &> a_Callback )
{
	Schema & schema = m_SchemaMap[ a_Graph ];
	schema.m_GraphId = a_Graph;
	schema.m_nVersion = schema.m_nSavedVersion = 0;

	new RequestJson( this, "/_graphs/" + a_Graph, "POST", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

//! Delete the named graph.
void Graph::DeleteGraph( const std::string & a_Graph,
	Delegate<const Json::Value &> a_Callback )
{
	m_SchemaMap[ a_Graph ].m_bDropped = true;
	new RequestJson( this, "/_graphs/" + a_Graph, "DELETE", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

//! Grab the current schema
void Graph::GetSchema( const std::string & a_Graph, Delegate<const Json::Value &> a_Callback )
{
	new RequestJson( this, "/" + a_Graph + "/schema", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

//! Post a new schema
void Graph::PostSchema( const std::string & a_Graph,
	const Json::Value & a_Schema,
	Delegate<const Json::Value &> a_Callback )
{
	Headers headers;
	headers["Content-Type"] = "application/json";

	std::string schema;
	schema = a_Schema.toStyledString();

	Log::Debug("Graph", "Uploading schema: %s", schema.c_str() );
	new RequestJson( this, "/" + a_Graph + "/schema", "POST", headers, schema, a_Callback );
}

//! Add a new vertex with the given id, label and properties.
bool Graph::Createvertex( const std::string & a_Graph,
	const std::string & a_Label,
	const PropertyMap & a_Properties,
	Delegate<const Json::Value &> a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}
	pSchema->ValidateVertexLabel( a_Label );

	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/vertices";
	pReq->m_RequestType = "POST";
	pReq->m_Body["label"] = a_Label;
	pReq->m_Callback = a_Callback;

	if ( a_Properties.isObject() )
	{
		for( Json::ValueIterator iProperty = a_Properties.begin(); 
			iProperty != a_Properties.end(); ++iProperty )
		{
			pReq->m_Body["properties"][iProperty.name()] = *iProperty;
			pSchema->ValidatePropertyKey( iProperty.name(), *iProperty );
			pSchema->ValidateVertexIndex( iProperty.name() );
		}
	}

	PushRequest( pSchema, pReq );
	return true;
}

//! Get a vertex by it's ID
bool Graph::GetVertex( const std::string & a_Graph,
	const VertexId & a_Id, Delegate<const Json::Value &> a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}
	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/vertices/" + a_Id;
	pReq->m_RequestType = "GET";
	pReq->m_Callback = a_Callback;

	PushRequest( pSchema, pReq );
	return true;
}

//! Update vertex properties by it's ID
bool Graph::UpdateVertex( const std::string & a_Graph,
	const VertexId & a_Id, const PropertyMap & a_Properties,
	Delegate<const Json::Value &> a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}

	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/vertices/" + a_Id;
	pReq->m_RequestType = "POST";
	pReq->m_Callback = a_Callback;

	if( a_Properties.isObject() )
	{
		for( Json::ValueIterator iProperty = a_Properties.begin(); 
			iProperty != a_Properties.end(); ++iProperty )
		{
			pReq->m_Body["properties"][iProperty.name()] = *iProperty;
			pSchema->ValidatePropertyKey( iProperty.name(), *iProperty );
			pSchema->ValidateVertexIndex( iProperty.name() );
		}
	}

	PushRequest( pSchema, pReq );
	return true;
}

//! Find vertices with the given properties
bool Graph::FindVertex( const std::string & a_Graph,
	const PropertyMap & a_Properties,
	Delegate<const Json::Value &> a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}

	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/vertices";
	pReq->m_RequestType = "GET";
	pReq->m_Callback = a_Callback;

	bool bFirstArg = true;
	for( Json::ValueIterator iProperty = a_Properties.begin(); 
		iProperty != a_Properties.end(); ++iProperty )
	{
		pSchema->ValidateVertexIndex( iProperty.name() );
		if ( bFirstArg )
		{
			pReq->m_Endpoint += "?";
			bFirstArg = false;
		}
		else
			pReq->m_Endpoint = "&";

		pReq->m_Endpoint += iProperty.name() + "=" + StringUtil::UrlEscape( (*iProperty).asString() );
	}

	PushRequest( pSchema, pReq );
	return true;
}

//! Delete a vertex 
bool Graph::DeleteVertex( const std::string & a_Graph,
	const VertexId & a_Id, Delegate<const Json::Value &> a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}

	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/vertices/" + a_Id;
	pReq->m_RequestType = "DELETE";
	pReq->m_Callback = a_Callback;

	PushRequest( pSchema, pReq );
	return true;
}

//! Create a new edge with the given id, label and properties 
//! starting at the out vertex and to the in vertex.
bool Graph::CreateEdge( const std::string & a_Graph,
	const VertexId & a_OutId,
	const VertexId & a_InId,
	const std::string & a_Label,
	const PropertyMap & a_Properties,
	Delegate<const Json::Value & > a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}
	pSchema->ValidateEdgeLabel( a_Label );

	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/edges";
	pReq->m_RequestType = "POST";
	pReq->m_Body["outV"] = a_OutId;
	pReq->m_Body["inV"] = a_InId;
	pReq->m_Body["label"] = a_Label;
	pReq->m_Callback = a_Callback;

	if ( a_Properties.isObject() )
	{
		for( Json::ValueIterator iProperty = a_Properties.begin(); 
			iProperty != a_Properties.end(); ++iProperty )
		{
			pReq->m_Body["properties"][iProperty.name()] = *iProperty;
			pSchema->ValidatePropertyKey( iProperty.name(), *iProperty );
			pSchema->ValidateEdgeIndex( iProperty.name() );
		}
	}

	PushRequest( pSchema, pReq );
	return true;
}

//! Get an edge by it's ID
bool Graph::GetEdge( const std::string & a_Graph,
	const EdgeId & a_Id, Delegate<const Json::Value &> a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}

	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/edges/" + a_Id;
	pReq->m_RequestType = "GET";
	pReq->m_Callback = a_Callback;

	PushRequest( pSchema, pReq );
	return true;
}

//! Update an edge by it's ID
bool Graph::UpdateEdge( const std::string & a_Graph,
	const EdgeId & a_Id, const PropertyMap & a_Properties,
	Delegate<const Json::Value &> a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}

	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/edges/" + a_Id;
	pReq->m_RequestType = "POST";
	pReq->m_Callback = a_Callback;

	if ( a_Properties.isObject() )
	{
		for( Json::ValueIterator iProperty = a_Properties.begin(); 
			iProperty != a_Properties.end(); ++iProperty )
		{
			pReq->m_Body["properties"][iProperty.name()] = *iProperty;
			pSchema->ValidatePropertyKey( iProperty.name(), *iProperty );
		}
	}

	PushRequest( pSchema, pReq );
	return true;
}

//! Find an edge by it's properties
bool Graph::FindEdge( const std::string & a_Graph,
	const PropertyMap & a_Properties,
	Delegate<const Json::Value &> a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}

	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/edges";
	pReq->m_RequestType = "GET";
	pReq->m_Callback = a_Callback;

	bool bFirstArg = true;
	for( Json::ValueIterator iProperty = a_Properties.begin(); 
		iProperty != a_Properties.end(); ++iProperty )
	{
		pSchema->ValidateVertexIndex( iProperty.name() );
		if ( bFirstArg )
		{
			pReq->m_Endpoint += "?";
			bFirstArg = false;
		}
		else
			pReq->m_Endpoint = "&";

		pReq->m_Endpoint += iProperty.name() + "=" + StringUtil::UrlEscape( (*iProperty).asString() );
	}

	PushRequest( pSchema, pReq );
	return true;
}

//! Delete an edge by it's ID
bool Graph::DeleteEdge( const std::string & a_Graph,
	const EdgeId & a_Id,
	Delegate<const Json::Value &> a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}

	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/edges/" + a_Id;
	pReq->m_RequestType = "DELETE";
	pReq->m_Callback = a_Callback;

	PushRequest( pSchema, pReq );
	return true;
}

bool Graph::Query( const std::string & a_Graph,
	const std::string & a_Query,
	const PropertyMap & a_Bindings,
	Delegate<const Json::Value &> a_Callback )
{
	Schema * pSchema = GetSchema( a_Graph );
	if ( pSchema == NULL )
	{
		Log::Error( "Graph", "Invalid graph %s", a_Graph.c_str() );
		return false;
	}

	Request * pReq = new Request();
	pReq->m_Endpoint = "/" + a_Graph + "/gremlin";
	pReq->m_Callback = a_Callback;
	pReq->m_RequestType = "POST";
	pReq->m_Body["gremlin"] = a_Query;
	
	for( Json::ValueIterator iProperty = a_Bindings.begin(); 
		iProperty != a_Bindings.end(); ++iProperty )
	{
		pReq->m_Body["bindings"][iProperty.name()] = *iProperty;
	}

	Log::Debug( "Graph", "Gremlin Query: %s", pReq->m_Body.toStyledString().c_str() );
	PushRequest( pSchema, pReq );
	return true;
}

void Graph::OnSessionToken( const Json::Value & a_Result )
{
	if (! a_Result.isNull() && a_Result.isMember( "gds-token") )
	{
		Log::Status( "Graph", "Gremlin Session Token: %s", a_Result["gds-token"].asCString() );
		m_Headers["Authorization"] = std::string("gds-token " ) + a_Result["gds-token"].asString();

		GetGraphs( DELEGATE( Graph, OnGraphs, const Json::Value &, this ) );
	}
	else
	{
		Log::Error( "Graph", "Failed to get session" );
		m_bReady = true;
	}
}

void Graph::OnGraphs( const Json::Value & a_Result )
{
	Log::Debug( "Graph", "OnGraphs: %s", a_Result.toStyledString().c_str() );
	if (! a_Result.isNull() )
	{
		const Json::Value & graphs = a_Result["graphs"];
		if ( graphs.size() > 0 )
		{
			for(size_t i=0;i<graphs.size();++i)
			{
				std::string graphId( graphs[i].asString() );
				m_SchemaMap[ graphId ].m_GraphId = graphId;
			}
		}

		m_bReady = true;
	}
	else
	{
		Log::Error( "Graph", "Failed to get the graphs." );
		m_bError = true;
	}
}

void Graph::PushRequest( Schema * a_pSchema, Request * a_pRequest )
{
	a_pRequest->m_pGraph = this;
	a_pRequest->m_pSchema = a_pSchema;
	m_RequestMap[ a_pSchema->m_GraphId ].push_back( a_pRequest );

	if ( a_pSchema->m_nVersion != a_pSchema->m_nSavedVersion )
	{
		if (! a_pSchema->m_bSaving )
			new SaveSchema( this, a_pSchema );
	}
	else if ( m_nActiveRequests < m_nMaxConcurrent )
		SendNextRequest( a_pSchema );
}

void Graph::SendNextRequest( Schema * a_pSchema )
{
	// send the next request if we found one..
	RequestList & req = m_RequestMap[ a_pSchema->m_GraphId ];
	if ( req.begin() != req.end() )
	{
		Request * pNext = req.front();
		req.pop_front();

		pNext->Send();
	}
}

//----------------------------------------------------------

void Graph::Request::Send()
{
	assert( m_pSchema != NULL );
	assert( m_pGraph != NULL );

	m_pGraph->m_nActiveRequests += 1;

	Headers headers;
	std::string body;

	if ( ! m_Body.isNull() )
	{
		headers["Content-Type"] = "application/json";
		body = m_Body.toStyledString();
	}

	new RequestJson( m_pGraph, m_Endpoint, m_RequestType, headers, body, 
		DELEGATE( Request, OnResponse, const Json::Value &, this ) );
}

void Graph::Request::OnResponse( const Json::Value & a_Response )
{
	m_Callback( a_Response );

	m_pGraph->m_nActiveRequests -= 1;
	if ( m_pGraph->m_nActiveRequests < m_pGraph->m_nMaxConcurrent )
		m_pGraph->SendNextRequest( m_pSchema );

	delete this;
}

//----------------------------------------------------------

Graph::SaveSchema::SaveSchema( Graph * a_pGremlin, Schema * a_pSchema ) :
	m_pGraph( a_pGremlin ), m_pSchema( a_pSchema ), m_nVersion( 0 )
{
	assert( m_pSchema->m_bSaving == false );
	m_pSchema->m_bSaving = true;

	// now load the current schema, and diff with the one we plan to save, 
	// we need to remove duplicated property keys
	m_pGraph->GetSchema( m_pSchema->m_GraphId, 
		DELEGATE( SaveSchema, OnDiffSchema, const Json::Value &, this ) );
}

//! Helper function for removing duplicated elements from the schema before we upload.
static int DiffLists( const Json::Value & active, Json::Value & list )
{
	if (! list.isArray() )
		list = Json::Value( Json::arrayValue );

	for(size_t i=0;i<active.size();++i)
	{
		for(size_t k=0;k<list.size();)
		{
			if ( list[k]["name"] == active[i]["name"] )
			{
				Json::Value removed;
				list.removeIndex( k, &removed );
				continue;
			}
			k += 1;
		}
	}

	return list.size();
}

void Graph::SaveSchema::OnDiffSchema( const Json::Value & a_Result )
{
	if (! a_Result.isNull() )
	{
		m_nVersion = m_pSchema->m_nVersion;

		m_Schema = Json::Value(Json::objectValue);
		m_pSchema->Serialize( m_Schema );

		const Json::Value & activeSchema = a_Result["result"]["data"][0];
		//Log::Debug( "Graph", "Active Schema: %s", activeSchema.toStyledString().c_str() );
		//Log::Debug( "Graph", "Save Schema: %s", m_Schema.toStyledString().c_str() );

		int d = DiffLists( activeSchema["edgeLabels"], m_Schema["edgeLabels"] );
		d += DiffLists( activeSchema["vertexLabels"], m_Schema["vertexLabels"] );
		d += DiffLists( activeSchema["propertyKeys"], m_Schema["propertyKeys"] );
		d += DiffLists( activeSchema["vertexIndexes"], m_Schema["vertexIndexes"] );
		d += DiffLists( activeSchema["edgeIndexes"], m_Schema["edgeIndexes"] );

		if ( d > 0 )
		{
			m_pGraph->PostSchema( m_pSchema->m_GraphId, m_Schema,
				DELEGATE( SaveSchema, OnUpdateSchema, const Json::Value &, this ) );
		}
		else
		{
			Log::Status( "Graph", "No required changes to schema, skipping upload." );
			// no changes to make to the schema!
			m_pSchema->m_nSavedVersion = m_nVersion;
			m_pSchema->m_bSaving = false;
			// notify the service the schema has been saved..
			m_pGraph->OnSchemaSaved( m_pSchema );
		}
	}
	else
	{
		Log::Error( "Graph", "Failed to get schema for %s", m_pSchema->m_GraphId.c_str() );
		m_pSchema->m_bSaving = false;

		delete this;
	}
}

void Graph::SaveSchema::OnUpdateSchema( const Json::Value & a_Result )
{
	if ( a_Result.isNull() )
	{
		Log::Error( "Graph", "Failed to save schema for %s", m_pSchema->m_GraphId.c_str() );

		// failed, retry to save the schema then..
		m_pGraph->GetSchema( m_pSchema->m_GraphId, 
			DELEGATE( SaveSchema, OnDiffSchema, const Json::Value &, this ) );
	}
	else
	{
		Log::Status( "Graph", "Schema for %s updated to version %d", m_pSchema->m_GraphId.c_str(), m_nVersion );
		// update the version number found in the schema..
		m_pSchema->m_nSavedVersion = m_nVersion;
		m_pSchema->m_bSaving = false;
		// notify the service the schema has been saved..
		m_pGraph->OnSchemaSaved( m_pSchema );

		delete this;
	}
}

void Graph::OnSchemaSaved( Schema * a_pSchema )
{
	if ( a_pSchema->m_nSavedVersion == a_pSchema->m_nVersion )
	{
		if ( m_nActiveRequests < m_nMaxConcurrent )
			SendNextRequest( a_pSchema );
	}
	else
	{
		// schema still out of date, push another save..
		new SaveSchema( this, a_pSchema );
	}
}

void Graph::OnLogStatus()
{
	if ( m_nActiveRequests > 0 )
	{
		int nRequests = 0;
		for(RequestMap::iterator iReq = m_RequestMap.begin(); iReq != m_RequestMap.end(); ++iReq )
			nRequests += iReq->second.size();

		Log::Status( "Graph", "Active requests %d/%d, queued requests %d", 
			m_nActiveRequests, m_nMaxConcurrent, nRequests );
	}
}
