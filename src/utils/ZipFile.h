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


#ifndef WDC_ZIP_FILE_H
#define WDC_ZIP_FILE_H

#include "UtilsLib.h"

#include <string>
#include <map>

class UTILS_API ZipFile
{
public:
	//! Types
	typedef std::map< std::string, std::string >		ZipMap;

	//! Create a zip archive from the provided map of file data. The map
	//! should be the name of the file and it's data as the key.
	static bool Deflate( const ZipMap & a_FileData, std::string & a_ArchiveData );
	//! Uncompress all files in a zip archive.
	static bool Inflate( const std::string & a_ArchiveData, ZipMap & a_FileData );
};

#endif

