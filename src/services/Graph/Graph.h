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

#ifndef WDC_GRAPH_SERVICE_H
#define WDC_GRAPH_SERVICE_H

#include "DataModels.h"
#include "utils/IConditional.h"
#include "utils/TimerPool.h"
#include "utils/IService.h"
#include "UtilsLib.h"

//! low-level service interface for communicating with IBM graph (Titan, Tinker-Pop and Gremlin stack)

//! NOTE: This class will automatically update the schema of the graph based on the labels and properties
//! of the vertexes and edges created through this interface. indexes are automatically created for all 
//! properties.

class UTILS_API Graph : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef Json::Value							PropertyMap;
	typedef GraphDataModels::VertexId			VertexId;
	typedef GraphDataModels::EdgeId				EdgeId;
	typedef GraphDataModels::Schema				Schema;

	//! Construction
	Graph();

	//! ISerializable
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();
	virtual bool Stop();

	//! Accessors
	bool IsReady() const { return m_bReady; } 
	bool IsError() const { return m_bError; }

	//! Grab the memory schema object fro the given graph, the user
	//! may make calls into this schema to update it if needed.
	//! This will return NULL if the graph doesn't exist.
	Schema * GetSchema( const std::string & a_GraphId );

	//! Request a session token
	void GetSessionToken(
		Delegate<const Json::Value &> a_Callback );

	//! Get all available graphs
	void GetGraphs( Delegate<const Json::Value &> a_Callback );
	//! Create a new graph
	void CreateGraph( const std::string & a_Graph,
		Delegate<const Json::Value &> a_Callback );
	//! Delete the named graph.
	void DeleteGraph( const std::string & a_Graph,
		Delegate<const Json::Value &> a_Callback );

	//! Grab the current schema
	void GetSchema( const std::string & a_Graph,
		Delegate<const Json::Value &> a_Callback );
	//! Update the schema for the given graph
	void PostSchema( const std::string & a_Graph,
		const Json::Value & a_Schema,
		Delegate<const Json::Value &> a_Callback );

	//--------------------------------------------------------------
	// The functions below work using a internal queue that will hold 
	// onto the requests if a pending schema change is needed before the 
	// request can succeed.

	//! Add a new vertex with the given id, label and properties.
	bool Createvertex( const std::string & a_Graph,
		const std::string & a_Label,
		const PropertyMap & a_Properties,
		Delegate<const Json::Value &> a_Callback );
	//! Get a vertex by it's ID
	bool GetVertex( const std::string & a_Graph,
		const VertexId & a_Id, 
		Delegate<const Json::Value &> a_Callback );
	//! Update vertex properties by it's ID
	bool UpdateVertex( const std::string & a_Graph,
		const VertexId & a_Id, 
		const PropertyMap & a_Properties,
		Delegate<const Json::Value &> a_Callback );
	//! Find vertices with the given properties
	bool FindVertex( const std::string & a_Graph,
		const PropertyMap & a_Properties,
		Delegate<const Json::Value &> a_Callback );
	//! Delete a vertex 
	bool DeleteVertex( const std::string & a_Graph,
		const VertexId & a_Id, 
		Delegate<const Json::Value &> a_Callback );

	//! Create a new edge with the given id, label and properties 
	//! starting at the out vertex and to the in vertex.
	bool CreateEdge( const std::string & a_Graph,
		const VertexId & a_OutId,
		const VertexId & a_InId,
		const std::string & a_Label,
		const PropertyMap & a_Properties,
		Delegate<const Json::Value & > a_Callback );
	//! Get an edge by it's ID
	bool GetEdge( const std::string & a_Graph,
		const EdgeId & a_Id,
		Delegate<const Json::Value &> a_Callback );
	//! Update an edge by it's ID
	bool UpdateEdge( const std::string & a_Graph,
		const EdgeId & a_Id,
		const PropertyMap & a_Properties,
		Delegate<const Json::Value &> a_Callback );
	//! Find an edge by it's properties
	bool FindEdge( const std::string & a_Graph,
		const PropertyMap & a_Properties,
		Delegate<const Json::Value &> a_Callback );
	//! Delete an edge by it's ID
	bool DeleteEdge( const std::string & a_Graph,
		const EdgeId & a_Id,
		Delegate<const Json::Value &> a_Callback );

	//! Run the given gremlin query on the graph with the provided bindings, the callback
	//! will be invoked with the results.
	bool Query( const std::string & a_Graph,
		const std::string & a_Query,
		const PropertyMap & a_Bindings,
		Delegate<const Json::Value &> a_Callback );

protected:
	//! Types
	typedef std::map<std::string,Schema>		SchemaMap;

	struct SaveSchema
	{
		Graph *			m_pGraph;				// a pointer back to our service so we can invoke on completion
		Schema *		m_pSchema;				// memory schema object
		Json::Value		m_Schema;				// Json version of the schema
		int				m_nVersion;				// version we are actively saving

		SaveSchema( Graph * a_pGemlin, Schema * a_pSchema );
		void OnDiffSchema( const Json::Value & a_Result );
		void OnUpdateSchema( const Json::Value & a_Result );
	};

	//! Data structure for holding a request to the graph, we need this internal structure
	//! since we may need to block until schema changes are applied or for batching purposes.
	struct Request
	{
		Request() : m_pGraph( NULL ), m_pSchema( NULL )
		{}

		std::string		m_RequestType;
		std::string		m_Endpoint;
		Json::Value		m_Body;
		Delegate<const Json::Value &>
			m_Callback;

		Graph *			m_pGraph;
		Schema *		m_pSchema;

		void Send();
		void OnResponse( const Json::Value & a_Response );
	};
	typedef std::list<Request *>				RequestList;
	typedef std::map<std::string,RequestList>	RequestMap;

	//! Data
	bool			m_bReady;
	bool			m_bError;
	volatile int	m_nPendingOps;
	int				m_nMaxConcurrent;
	volatile int	m_nActiveRequests;			// how many concurrent connections do we have active
	SchemaMap		m_SchemaMap;
	RequestMap		m_RequestMap;
	TimerPool::ITimer::SP
					m_spLogStatus;

	//! Callbacks
	void OnSessionToken( const Json::Value & a_Result );
	void OnGraphs( const Json::Value & a_Result );

	void PushRequest( Schema * a_pSchema, Request * a_pRequest );
	void SendNextRequest( Schema * a_pSchema );
	void OnSchemaSaved( Schema * a_pSchema );	// called by the SaveSchema() object when it's done
	void OnLogStatus();
};

#endif
