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

//! Author rsinha

#include "Form.h"
#include "Log.h"
#include "UniqueID.h"
#include "StringUtil.h"
#include "Path.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "boost/filesystem/path.hpp"

//! Static initializers
std::string Form::m_lineFeed = "\r\n";

Form::Form()
{
	// generate a boundary..
    std::stringstream b;
    b << "--------------------------";
    b << UniqueID().Get();

    m_boundary = b.str();

	m_ContentType = "multipart/form-data; boundary=" + m_boundary;
}

void Form::AddFormField(const std::string & name, const std::string & value,
	const std::string & a_CharSet /*= "UTF-8"*/ )
{
    std::stringstream part;
    part << "--" << m_boundary << m_lineFeed;
    part << "Content-Disposition: form-data; name=\"" << name << "\"" << m_lineFeed;
    part << m_lineFeed << value << m_lineFeed;
	m_Parts.push_back( part.str() );
}

bool Form::AddFilePartFromPath(const std::string & a_FieldName,const  std::string & a_FullFilePath,
	const std::string & a_ContentType /*= "application/octet-stream"*/ ) 
{
	// get just the filename part..
	std::string filename( Path( a_FullFilePath ).GetFileName() );

	// read in all the file data..
    std::ifstream input(a_FullFilePath.c_str(), std::ios::in | std::ios::binary);
    if (!input.is_open()) 
	{
		Log::Error( "Form", "Failed to open input file." );
		return false;
    }
	std::string fileData;
	fileData.assign( std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() );

	return AddFilePart( a_FieldName, filename, fileData, a_ContentType );
}

bool Form::AddFilePart(const std::string & a_FieldName, 
	const std::string & a_FileName, 
	const std::string & a_FileData,
	const std::string & a_ContentType /*= "application/octet-stream"*/ )
{
	std::stringstream part;
	part << "--" << m_boundary << m_lineFeed;
	part << "Content-Disposition: form-data; name=\"" << a_FieldName;
	part << "\"; filename=\"" << a_FileName;
	part << "\"" << m_lineFeed << "Content-Type: " << a_ContentType << m_lineFeed;
	part << "Content-Transfer-Encoding: binary" << m_lineFeed;
	part << m_lineFeed;
	part << a_FileData;
	part << m_lineFeed;

	m_Parts.push_back( part.str() );
	return true;
}

void Form::Finish() 
{
    std::stringstream body;
	for(size_t i=0;i<m_Parts.size();++i)
		body << m_Parts[i];
	// write out footer..
	body << m_lineFeed << "--" << m_boundary << "--" << m_lineFeed;

	m_Body = body.str();
}

