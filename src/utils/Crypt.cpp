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

#include <openssl/evp.h>
#include <stdio.h>						// sprintf

#include "Crypt.h"
#include "Log.h"
#include "StringUtil.h"

#pragma warning( disable : 4996 )		// silence windows warning

static EVP_CIPHER_CTX g_EncodeContext;
static EVP_CIPHER_CTX g_DecodeContext;

static const unsigned int AES_BLOCK_SIZE = 16;

//! perform a symmetric encryption on the provided unencrypted data
std::string Crypt::Encode( const std::string & a_Data )
{
	Initialize();

	int c_len = a_Data.size() + AES_BLOCK_SIZE;
	int f_len = 0;

	std::string encrypted;
	encrypted.resize( c_len );

	EVP_EncryptInit_ex( &g_EncodeContext, NULL, NULL, NULL, NULL );
	EVP_EncryptUpdate( &g_EncodeContext, (unsigned char *)&encrypted[0], &c_len, (const unsigned char *)&a_Data[0], a_Data.size() );
	EVP_EncryptFinal_ex( &g_EncodeContext, (unsigned char *)&encrypted[c_len], &f_len );
	encrypted.resize( c_len + f_len );

	return "ENC:" + StringUtil::EncodeBase64( encrypted );
}

//! perform a symmetric decryption on the provided encrypted data
std::string Crypt::Decode( const std::string & a_Data )
{
	if ( a_Data.size() < 4 || a_Data.substr( 0, 4 ) != "ENC:" )
		return a_Data;		// data not encoded, just return the data passed in..

	std::string encoded( StringUtil::DecodeBase64( a_Data.substr( 4 ) ) );

	Initialize();

	int len = encoded.size();
	int p_len = len;
	int f_len = 0;

	std::string decrypted;
	decrypted.resize( p_len );

	EVP_DecryptInit_ex( &g_DecodeContext, NULL, NULL, NULL, NULL );
	EVP_DecryptUpdate( &g_DecodeContext, (unsigned char *)&decrypted[0], &p_len, (unsigned char *)&encoded[0], len );
	EVP_DecryptFinal_ex( &g_DecodeContext, (unsigned char *)&decrypted[p_len], &f_len );
	
	decrypted.resize( p_len + f_len );
	return decrypted;
}

bool Crypt::Initialize()
{
	static bool bInitialized = false;
	if (! bInitialized )
	{
		unsigned char key[32], iv[32];

		unsigned int salt[] = {12345, 54321};
		const unsigned char * key_data = (const unsigned char *)"sdJDOSDh98HsdsHDKJSHDhjsdgUSDSJK";
		size_t key_data_len = strlen( (const char *)key_data );

		int bytes =  EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), (unsigned char *)&salt, key_data, key_data_len, 5, key, iv);
		if ( bytes != 32 )
		{
			Log::Error( "Crypt", "Key size is %d bits, should be 256 bits.", bytes );
			return false;
		}

		EVP_CIPHER_CTX_init(&g_EncodeContext);
		EVP_EncryptInit_ex(&g_EncodeContext, EVP_aes_256_cbc(), NULL, key, iv);
		EVP_CIPHER_CTX_init(&g_DecodeContext);
		EVP_DecryptInit_ex(&g_DecodeContext, EVP_aes_256_cbc(), NULL, key, iv);

		bInitialized = true;
	}

	return true;
}


