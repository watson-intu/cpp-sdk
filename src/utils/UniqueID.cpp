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

#include <assert.h>

const std::string & UniqueID::Generate()
{
	static GuidGenerator generator;
	generator.newGuid().GetString(m_GUID);
	m_eType = STRING;
	return m_GUID;
}

const UniqueID & UniqueID::ToBinary()
{
	if ( m_eType == ENCODED )
		Decode();
	if ( m_eType == STRING )
	{
		// covert 36-character GUID "a6c045e7-2751-407a-88bc-2067f05424a3" into 16-byte binary GUID
		StringUtil::Replace( m_GUID, "-", "" );

		std::string encoded;
		for(size_t i=0;i<m_GUID.size(); i += 2)
		{
			unsigned int c = HexToByte(m_GUID[i]) << 4;
			c |= HexToByte( m_GUID[i+1] );
			encoded += (char)c;
		}

		m_GUID = encoded;
		m_eType = BINARY;
	}

	return *this;
}

const UniqueID & UniqueID::ToString()
{
	if ( m_eType == ENCODED )
		Decode();
	if ( m_eType == BINARY )
	{
		std::string decoded;
		for(size_t i=0;i<m_GUID.size();++i)
		{
			if ( i == 4 || i == 6 || i == 8 || i == 10 )
				decoded += '-';
			decoded += StringUtil::Format( "%02x", (unsigned int)(m_GUID[i] & 0xff) );
		}

		m_GUID = decoded;
		m_eType = STRING;
	}

	return *this;
}

const UniqueID & UniqueID::Encode()
{
	if ( m_eType != BINARY )
		ToBinary();
	if ( m_eType == BINARY )
	{
		std::string encoded;
		unsigned int fragment = 0;
		unsigned int bits = 0;
		for(size_t i=0;i<m_GUID.size();++i)
		{
			unsigned int c = ((unsigned int)m_GUID[i]) & 0xff;
			fragment = (c << bits) | fragment;
			bits += 8;

			while( bits >= 6 )
			{
				encoded += Encode64( fragment & 0x3f );
				fragment = fragment >> 6;
				bits -= 6;
			}
		}

		if ( bits > 0 )
			encoded += Encode64( fragment & 0x3f );

		m_GUID = encoded;
		m_eType = ENCODED;
	}

	return *this;
}

const UniqueID & UniqueID::Decode()
{
	if ( m_eType == ENCODED )
	{
		std::string decoded;

		unsigned int fragment = 0;
		unsigned int bits = 0;
		for(size_t i=0;i<m_GUID.size();++i)
		{
			unsigned int c = Decode64( m_GUID[i] );
			assert( c < 64 );

			fragment = (c << bits) | fragment;
			bits += 6;

			while( bits >= 8 )
			{
				decoded += (char)(fragment & 0xff);
				fragment = fragment >> 8;
				bits -= 8;
			}
		}

		m_GUID = decoded;
		m_eType = BINARY;
	}

	return *this;
}

char UniqueID::Encode64( unsigned int v )
{
	static const char * encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (v > 63) 
		return '=';
	return encoding[v];
}

int UniqueID::Decode64( char c )
{
	static const char decoding[] = {62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1,
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
		22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
		37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
	static const char decoding_size = sizeof(decoding);
	c -= 43;
	if (c < 0 || c > decoding_size) 
		return -1;
	return decoding[(int) c];
}

unsigned int UniqueID::HexToByte( char c )
{
	if ( c >= 'a' && c <= 'f' )
		return (c - 'a') + 10;
	else if ( c >= 'A' && c <= 'F' )
		return (c - 'A') + 10;
	else if ( c >= '0' && c <= '9' )
		return (c - '0');

	return 0;
}


