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


#ifndef WDC_GRAPH_DATAMODELS_H
#define WDC_GRAPH_DATAMODELS_H

#include "utils/ISerializable.h"
#include "UtilsLib.h"

namespace GraphDataModels {

	//! Types
	typedef std::vector<std::string>	StringVector;
	typedef std::string					VertexId;
	typedef std::string					EdgeId;

	struct UTILS_API EdgeLabel : public ISerializable
	{
		std::string		m_Multiplicity;
		std::string		m_Name;

		virtual void Serialize(Json::Value & json)
		{
			json["multiplicity"] = m_Multiplicity;
			json["name"] = m_Name;
		}
		virtual void Deserialize(const Json::Value & json)
		{
			m_Multiplicity = json["multiplicity"].asString();
			m_Name = json["name"].asString();
		}
	};

	struct UTILS_API VertexLabel : public ISerializable
	{
		std::string		m_Name;

		virtual void Serialize(Json::Value & json)
		{
			json["name"] = m_Name;
		}
		virtual void Deserialize(const Json::Value & json)
		{
			m_Name = json["name"].asString();
		}
	};

	struct UTILS_API IndexOnly : public ISerializable
	{
		IndexOnly() 
		{}
		IndexOnly( const std::string & a_Name ) : m_Name( a_Name )
		{}

		std::string		m_Name;

		virtual void Serialize(Json::Value & json)
		{
			json["name"] = m_Name;
		}
		virtual void Deserialize(const Json::Value & json)
		{
			m_Name = json["name"].asString();
		}
	};

	struct UTILS_API Index : public ISerializable
	{
		Index() : m_Composite(false), m_Unique(false)
		{}

		bool						m_Composite;
		std::string					m_Name;
		bool						m_Unique;
		std::vector<std::string>	m_PropertyKeys;
		std::vector<IndexOnly>		m_IndexOnly;

		virtual void Serialize(Json::Value & json)
		{
			json["composite"] = m_Composite;
			json["name"] = m_Name;
			json["unique"] = m_Unique;
			SerializeVector("propertyKeys", m_PropertyKeys, json);
			SerializeVector("indexOnly", m_IndexOnly, json);
		}
		virtual void Deserialize(const Json::Value & json)
		{
			m_Composite = json["composite"].asBool();
			m_Name = json["name"].asString();
			m_Unique = json["unique"].asBool();
			DeserializeVector("propertyKeys", json, m_PropertyKeys);
			DeserializeVector("indexOnly", json, m_IndexOnly);
		}
	};

	struct UTILS_API PropertyKey : public ISerializable
	{
		std::string		m_Cardinality;
		std::string		m_DataType;
		std::string		m_Name;

		virtual void Serialize(Json::Value & json)
		{
			json["cardinality"] = m_Cardinality;
			json["dataType"] = m_DataType;
			json["name"] = m_Name;
		}
		virtual void Deserialize(const Json::Value & json)
		{
			m_Cardinality = json["cardinality"].asString();
			m_DataType = json["dataType"].asString();
			m_Name = json["name"].asString();
		}
	};

	typedef std::map<std::string, size_t>		IndexMap;

	struct UTILS_API Schema : public ISerializable
	{
		Schema() : m_nVersion(0), m_bSaving( false ), m_bDropped(false), m_nSavedVersion(-1)
		{}

		std::vector<Index>			m_EdgeIndexes;
		std::vector<EdgeLabel>		m_EdgeLabels;
		std::vector<PropertyKey>	m_PropertyKeys;
		std::vector<Index>			m_VertexIndexes;
		std::vector<VertexLabel>	m_VertexLabels;

		int							m_nVersion;			// version number for tracking number of changes made to this schema
		bool						m_bSaving;			// true if we are currently saving a schema change to the server
		bool						m_bDropped;			// true if this schema has been dropped (deleted)
		int							m_nSavedVersion;	// version saved to the server
		std::string					m_GraphId;			// the ID of our graph
		IndexMap					m_Index;			// index for quickly looking up if an edge, vertex label, property, etc already exists

		virtual void Serialize(Json::Value & json)
		{
			SerializeVector("edgeIndexes", m_EdgeIndexes, json);
			SerializeVector("edgeLabels", m_EdgeLabels, json);
			SerializeVector("propertyKeys", m_PropertyKeys, json);
			SerializeVector("vertexIndexes", m_VertexIndexes, json);
			SerializeVector("vertexLabels", m_VertexLabels, json);
		}
		virtual void Deserialize(const Json::Value & json)
		{
			m_EdgeIndexes.clear();
			DeserializeVector("edgeIndexes", json, m_EdgeIndexes);
			m_EdgeLabels.clear();
			DeserializeVector("edgeLabels", json, m_EdgeLabels);
			m_PropertyKeys.clear();
			DeserializeVector("propertyKeys", json, m_PropertyKeys);
			m_VertexIndexes.clear();
			DeserializeVector("vertexIndexes", json, m_VertexIndexes);
			m_VertexLabels.clear();
			DeserializeVector("vertexLabels", json, m_VertexLabels);
			BuildIndex();
		}

		void BuildIndex();
		static const char * GetCardinality(const Json::Value & a_Property);
		static const char * GetDataType(const Json::Value & a_Property);

		void ValidatePropertyKey(const std::string & a_Name, const Json::Value & a_Property);
		void ValidateVertexLabel(const std::string & a_Label);
		void ValidateVertexIndex(const std::string & a_Index, bool a_bComposite = false, bool a_bUnique = false);
		void ValidateEdgeLabel(const std::string & a_Label, const char * a_pMultiplicity = "MULTI");
		void ValidateEdgeIndex(const std::string & a_Index, bool a_bComposite = false, bool a_bUnique = false);
	};

} // end namespace

#endif
