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

#include "UniqueID.h"
#include "StringUtil.h"
#include "crossguid/crossguid.h"

const std::string & UniqueID::Generate()
{
	static GuidGenerator generator;
	generator.newGuid().GetString(m_GUID);
	return m_GUID;
}

static unsigned int HexToByte( char c )
{
	if ( c >= 'a' && c <= 'f' )
		return (c - 'a') + 10;
	else if ( c >= 'A' && c <= 'F' )
		return (c - 'A') + 10;
	else if ( c >= '0' && c <= '9' )
		return (c - '0');

	return 0;
}

const std::string & UniqueID::ToBinary()
{
	// "a6c045e7-2751-407a-88bc-2067f05424a3"
	if ( m_GUID.size() == 36 )
	{
		StringUtil::Replace( m_GUID, "-", "" );

		std::string encoded;
		for(size_t i=0;i<m_GUID.size(); i += 2)
		{
			unsigned int c = HexToByte(m_GUID[i]) << 4;
			c |= HexToByte( m_GUID[i+1] );
			encoded += (char)c;
		}

		m_GUID = encoded; 
	}

	return m_GUID;
}

const std::string & UniqueID::Encode64()
{
	ToBinary();
	if ( m_GUID.size() == 16 )
		m_GUID = StringUtil::EncodeBase64( m_GUID );
	return m_GUID;
}

const std::string & UniqueID::FromBinary()
{
	if ( m_GUID.size() == 16 )
	{
		std::string decoded;
		for(size_t i=0;i<m_GUID.size();++i)
		{
			if ( i == 4 || i == 6 || i == 8 || i == 10 )
				decoded += '-';
			decoded += StringUtil::Format( "%02x", (unsigned int)(m_GUID[i] & 0xff) );
		}

		m_GUID = decoded;
	}

	return m_GUID;
}

const std::string & UniqueID::Decode64()
{
	if ( m_GUID.size() == 24 )
	{
		m_GUID = StringUtil::DecodeBase64( m_GUID );
		FromBinary();
	}

	return m_GUID;
}

