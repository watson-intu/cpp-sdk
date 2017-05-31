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


#include <openssl/md5.h>
#include <stdio.h>						// sprintf

#include "MD5.h"

#pragma warning( disable : 4996 )		// silence windows warning

template<class type>
void MD5(const type& input, type& hash)
{
	MD5_CTX context;
	MD5_Init(&context);
	MD5_Update(&context, &input[0], input.size());

	unsigned char digest[16];
	MD5_Final(digest, &context);

	hash.resize(32);
	for (size_t i = 0; i < 16; ++i)
		sprintf(&hash[i * 2], "%02x", (unsigned int)digest[i]);
}

template<class type>
type MD5(const type& input)
{
	type hash;
	MD5(input, hash);
	return hash;
}

std::string MakeMD5( const std::string & a_Data )
{
	return MD5( a_Data );
}

