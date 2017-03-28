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

#include "ZipFile.h"
#include "minizip/zip.h"
#include "minizip/unzip.h"
#include "minizip/ioapi_mem.h"

bool ZipFile::Deflate( const ZipMap & a_FileData, std::string & a_ArchiveData )
{
	ourmemory_t zm = ourmemory_t();
	zm.grow = 1;

	zlib_filefunc_def ff = zlib_filefunc_def();
	fill_memory_filefunc( &ff, &zm );

	zipFile zf = zipOpen3( "__memoryzip__", 0, 0, 0, &ff);
	if ( zf == NULL )
		return false;

	for( ZipMap::const_iterator iFile = a_FileData.begin(); 
		iFile != a_FileData.end(); ++iFile )
	{
		zip_fileinfo zi = { 0 };

		zipOpenNewFileInZip64( zf,
			iFile->first.c_str(),
			&zi,
			NULL,
			0,
			NULL,
			0,
			NULL, // comment
			Z_DEFLATED,
			1,
			0 );
		zipWriteInFileInZip( zf, iFile->second.data(), iFile->second.size() );
		zipCloseFileInZip( zf );
	}

	zipClose( zf, NULL );
	if ( zm.base == NULL )
		return false;

	a_ArchiveData = std::string( zm.base, zm.limit );
	free( zm.base );			// free the memory

	return true;
}

bool ZipFile::Inflate( const std::string & a_ArchiveData, ZipMap & a_FileData )
{
	return false;
}
