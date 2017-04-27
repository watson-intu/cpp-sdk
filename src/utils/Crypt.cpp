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

#include <openssl/aes.h>
#include <stdio.h>						// sprintf

#include "Crypt.h"

#pragma warning( disable : 4996 )		// silence windows warning

static const unsigned char key[] = {
	0x87, 0x34, 0x92, 0x23, 0xf4, 0xc5, 0xb6, 0xa7,
	0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0x9f,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

//! perform a symmetric encryption on the provided unencrypted data
std::string Encrypt( const std::string & a_Data )
{
	std::string encrypted("AES:");

	AES_KEY enc_key;
	AES_set_encrypt_key( key, 128, &enc_key );

	size_t i=0;
	while( i < a_Data.size() )
	{
		std::string block( a_Data.substr( i, AES_BLOCK_SIZE ) );
		i += AES_BLOCK_SIZE;

		unsigned char encoded[ AES_BLOCK_SIZE ];
		AES_encrypt( (unsigned char *)&block[0], encoded, &enc_key );

		encrypted += std::string( (const char *)encoded, block.size() );
	}

	return encrypted;
}

//! perform a symmetric decryption on the provided encrypted data
std::string Decrypt( const std::string & a_Data )
{
	if ( a_Data.size() < 4 || a_Data.substr( 0, 4 ) != "AES:" )
		return a_Data;

	AES_KEY dec_key;
	AES_set_decrypt_key( key, 128, &dec_key );

	std::string decrypted;
	size_t i=4;
	while( i < a_Data.size() )
	{
		std::string block( a_Data.substr( i, AES_BLOCK_SIZE ) );
		i += AES_BLOCK_SIZE;

		unsigned char decoded[ AES_BLOCK_SIZE ];
		AES_decrypt( (unsigned char *)&block[0], decoded, &dec_key );

		decrypted += std::string( (const char *)decoded, block.size() );
	}

	return decrypted;
}

