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

#include "DataModels.h"

using namespace GraphDataModels;

void Schema::BuildIndex()
{
	m_Index.clear();
	for (size_t i = 0; i < m_EdgeIndexes.size(); ++i)
		m_Index["edgeIndex." + m_EdgeIndexes[i].m_Name] = i;
	for (size_t i = 0; i < m_EdgeLabels.size(); ++i)
		m_Index["edgeLabels." + m_EdgeLabels[i].m_Name] = i;
	for (size_t i = 0; i < m_PropertyKeys.size(); ++i)
		m_Index["propertyKey." + m_PropertyKeys[i].m_Name] = i;
	for (size_t i = 0; i < m_VertexIndexes.size(); ++i)
		m_Index["vertexIndex." + m_VertexIndexes[i].m_Name] = i;
	for (size_t i = 0; i < m_VertexLabels.size(); ++i)
		m_Index["vertexLabel." + m_VertexLabels[i].m_Name] = i;
}

const char * Schema::GetCardinality(const Json::Value & a_Property)
{
	if (a_Property.isArray())
		return "LIST";
	return "SINGLE";
}

const char * Schema::GetDataType(const Json::Value & a_Property)
{
	if (a_Property.isDouble())
		return "Float";
	else if (a_Property.isInt())
		return "Integer";
	else if (a_Property.isBool())
		return "Boolean";
	return "String";
}

void Schema::ValidatePropertyKey(const std::string & a_Name, const Json::Value & a_Property)
{
	IndexMap::iterator iIndex = m_Index.find("propertyKey." + a_Name);
	if (iIndex == m_Index.end())
	{
		// add new property key..
		m_Index["propertyKey." + a_Name] = m_PropertyKeys.size();
		m_nVersion += 1;

		PropertyKey prop;
		prop.m_Name = a_Name;
		prop.m_Cardinality = GetCardinality(a_Property);
		prop.m_DataType = GetDataType(a_Property);

		m_PropertyKeys.push_back(prop);
		Log::Status("Gremlin", "Added new property key: %s", prop.ToJson().toStyledString().c_str());
	}
	else	// verify the data type and cardinality for this property key is unchanged then..
	{
		PropertyKey & pk = m_PropertyKeys[iIndex->second];
		if (pk.m_DataType != GetDataType(a_Property))
		{
			Log::Error("Gremlin", "PropertyKey data type is different %s != %s",
				pk.m_DataType.c_str(), GetDataType(a_Property));
		}
		if (pk.m_Cardinality != GetCardinality(a_Property))
		{
			Log::Error("Gremlin", "PropertyKey cardinality is different %s != %s",
				pk.m_Cardinality.c_str(), GetCardinality(a_Property));
		}
	}
}

void Schema::ValidateVertexLabel(const std::string & a_Label)
{
	IndexMap::iterator iIndex = m_Index.find("vertexLabel." + a_Label);
	if (iIndex == m_Index.end())
	{
		m_Index["vertexLabel." + a_Label] = m_VertexLabels.size();
		m_nVersion += 1;

		VertexLabel label;
		label.m_Name = a_Label;

		m_VertexLabels.push_back(label);
		Log::Status("Gremlin", "Added new vertex label: %s", label.ToJson().toStyledString().c_str());
	}
}

void Schema::ValidateVertexIndex(const std::string & a_Index, bool a_bComposite /*= false*/, bool a_bUnique /*= false*/)
{
	IndexMap::iterator iIndex = m_Index.find("vertexIndex." + a_Index);
	if (iIndex == m_Index.end())
	{
		m_Index["vertexIndex." + a_Index] = m_VertexIndexes.size();
		m_nVersion += 1;

		Index index;
		index.m_Composite = a_bComposite;
		index.m_Name = "vi_" + a_Index;
		index.m_PropertyKeys.push_back(a_Index);
		index.m_Unique = a_bUnique;

		m_VertexIndexes.push_back(index);
		Log::Status("Gremlin", "Added new vertex index: %s", index.ToJson().toStyledString().c_str());
	}
}

void Schema::ValidateEdgeLabel(const std::string & a_Label, const char * a_pMultiplicity /*= "MULTI"*/)
{
	IndexMap::iterator iIndex = m_Index.find("edgeLabel." + a_Label);
	if (iIndex == m_Index.end())
	{
		m_Index["edgeLabel." + a_Label] = m_EdgeLabels.size();
		m_nVersion += 1;

		EdgeLabel label;
		label.m_Multiplicity = a_pMultiplicity;
		label.m_Name = a_Label;

		m_EdgeLabels.push_back(label);
		Log::Status("Gremlin", "Added new edge label: %s", label.ToJson().toStyledString().c_str());
	}
}

void Schema::ValidateEdgeIndex(const std::string & a_Index, bool a_bComposite /*= false*/, bool a_bUnique /*= false*/)
{
	IndexMap::iterator iIndex = m_Index.find("edgeIndex." + a_Index);
	if (iIndex == m_Index.end())
	{
		m_Index["edgeIndex." + a_Index] = m_EdgeIndexes.size();
		m_nVersion += 1;

		Index index;
		index.m_Composite = a_bComposite;
		index.m_Name = "ei_" + a_Index;
		index.m_PropertyKeys.push_back(a_Index);
		index.m_Unique = a_bUnique;

		m_EdgeIndexes.push_back(index);
		Log::Status("Gremlin", "Added new edge index: %s", index.ToJson().toStyledString().c_str());
	}
}

