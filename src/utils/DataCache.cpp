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


#include <fstream>
#include <iostream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "DataCache.h"
#include "StringUtil.h"
#include "Time.h"
#include "Path.h"

namespace fs = boost::filesystem;

DataCache::DataCache() : m_bInitialized(false), m_MaxCacheSize( 0 ), m_MaxCacheAge( 0 ), m_CurrentCacheSize( 0 )
{}

bool DataCache::Initialize(const std::string & a_CachePath, 
	unsigned int maxCacheSize /* = 1024 * 1024 * 50*/,
	double maxCacheAge /*= 24 * 7*/,
	const std::string & a_Extention /*= ".bytes"*/ )
{
	m_bInitialized = true;
	m_Extension = a_Extention;
	m_CachePath = a_CachePath;
	m_MaxCacheSize =  maxCacheSize;
	m_MaxCacheAge = maxCacheAge;

	if (! fs::is_directory( fs::path( a_CachePath ) ) )
	{
		try {
			fs::create_directories( fs::path( a_CachePath ) );
		}
		catch( const std::exception & ex )
		{
			Log::Error( "DataCache", "Caught Exception: %s", ex.what() );
			return false;
		}
	}

	m_Cache.clear();
	m_CurrentCacheSize = 0;

	for( fs::directory_iterator p( m_CachePath ); p != fs::directory_iterator(); ++p )
	{
		if ( fs::is_regular_file( p->status() ) )
		{
			try {
#ifdef _WIN32
				std::string path = StringUtil::Format("%S", p->path().c_str());
#else
				std::string path = p->path().c_str();
#endif
				std::string id = Path(path).GetFile();

				CacheItem &item = m_Cache[id];
				item.m_Path = path;
				item.m_Id = id;
				item.m_Time = Time(fs::last_write_time(p->path())).GetEpochTime();
				item.m_Size = (unsigned int) fs::file_size(p->path());
				m_CurrentCacheSize += item.m_Size;
			}
			catch( const std::exception & e )
			{
				Log::Error( "DataCache", "Caught Exception: %s", e.what() );
			}
		}
	}

	// flush old items from cache..
	FlushAged();
	// flush data until we are under our max size
	while( m_CurrentCacheSize > m_MaxCacheSize )
		FlushOldest();

	return true;
}

void DataCache::Uninitialize()
{
	m_Cache.clear();
	m_CurrentCacheSize = 0;
	m_bInitialized = false;
}

//! Find data in this cache by ID, returns a NULL if object is not found in this cache.
DataCache::CacheItem * DataCache::Find(const std::string & a_ID, bool a_bLoadIntoMemory /*= true*/ )
{
	std::string id = a_ID;
	StringUtil::Replace( id, "/", "_" );

	CacheItemMap::iterator iFind = m_Cache.find( id );
	if ( iFind != m_Cache.end() )
	{
		CacheItem * pItem  = &iFind->second;
		if ( a_bLoadIntoMemory && !pItem->m_bLoaded )
		{
			// load the file from disk into memory now..
			try {
				std::ifstream input(pItem->m_Path.c_str(), std::ios::in | std::ios::binary);
				pItem->m_Data.reserve(pItem->m_Size);
				pItem->m_Data.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
				pItem->m_bLoaded = true;

				if (pItem->m_Data.size() != pItem->m_Size)
				{
					Log::Warning("DataCache", "Expected size of %u != %u", pItem->m_Size, pItem->m_Data.size());

					// adjust our cache size and update the item size..
					if (pItem->m_Data.size() > pItem->m_Size)
						m_CurrentCacheSize += pItem->m_Data.size() - pItem->m_Size;
					else
						m_CurrentCacheSize -= pItem->m_Size - pItem->m_Data.size();
					pItem->m_Size = pItem->m_Data.size();
				}
			}
			catch (const std::exception & ex)
			{
				Log::Error("DataCache", "Caught exception: %s", ex.what());
				return NULL;
			}
		}

		return pItem;
	}

	return NULL;
}

bool DataCache::Save(const std::string & a_ID, const std::string & a_Data, bool a_bKeepInMemory/* = true*/ )
{
	std::string id = a_ID;
	StringUtil::Replace(id, "/", "_");

	// flush the old object..
	if ( m_Cache.find( id ) != m_Cache.end() )
	{
		Log::Debug( "DataCache", "Flushing old object with same key %s.", id.c_str() );
		if (! Flush( id ) )
		{
			Log::Error( "DataCache", "Failed to save new object %s", a_ID.c_str() );
			return false;
		}
	}

	CacheItem & item = m_Cache[ id ];
	item.m_Path = m_CachePath + id + m_Extension;
	item.m_Id = id;
	item.m_Time = Time().GetEpochTime();
	item.m_Size = a_Data.size();

	if ( a_bKeepInMemory )
	{
		item.m_bLoaded = true;
		item.m_Data = a_Data;
	}

	try {
		std::ofstream output(item.m_Path.c_str(), std::ios::out | std::ios::binary);
		output << a_Data;
		output.close();
	}
	catch (const std::exception & ex)
	{
		Log::Warning("DataCache", "Caught exception: %s", ex.what());
	}

	m_CurrentCacheSize += item.m_Size;
	while( m_CurrentCacheSize > m_MaxCacheSize )
		FlushOldest();

	return true;
}

bool DataCache::Flush(const std::string & a_ID)
{
	std::string id = a_ID;
	StringUtil::Replace(id, "/", "_");

	// flush the old object..
	CacheItemMap::iterator iItem = m_Cache.find( id );
	if ( iItem != m_Cache.end() )
	{
		CacheItem & item = iItem->second;
		try {
			fs::remove( fs::path( item.m_Path ) );
		}
		catch( const std::exception & ex )
		{
			Log::Error( "DataCache", "Caught Exception: %s", ex.what() );
			return false;
		}
		m_CurrentCacheSize -= item.m_Size;
		m_Cache.erase( iItem );
		return true;
	}

	return false;
}

bool DataCache::FlushAged()
{
	bool bFlushed = false;
	if ( m_MaxCacheAge > 0 )
	{
		Time now;

		std::list<std::string> flush;
		for( CacheItemMap::iterator iItem = m_Cache.begin(); iItem != m_Cache.end(); ++iItem )
		{
			double age = (now.GetEpochTime() - iItem->second.m_Time) / 3600.0;
			if ( age > m_MaxCacheAge )
				flush.push_back( iItem->second.m_Id );
		}

		for( std::list<std::string>::iterator iFlush = flush.begin(); iFlush != flush.end(); ++iFlush )
			bFlushed |= Flush( *iFlush );
	}

	return bFlushed;
}

bool DataCache::FlushOldest()
{
	CacheItem * pOldest = NULL;
	for (CacheItemMap::iterator iItem = m_Cache.begin(); iItem != m_Cache.end(); ++iItem)
	{
		if ( pOldest == NULL || iItem->second.m_Time < pOldest->m_Time )
			pOldest = &iItem->second;
	}

	if ( pOldest != NULL )
		return Flush( pOldest->m_Id );

	return false;
}

bool DataCache::FlushAll()
{
	for (CacheItemMap::iterator iItem = m_Cache.begin(); iItem != m_Cache.end(); ++iItem)
	{
		try {
			fs::remove( fs::path( iItem->second.m_Path ) );
		}
		catch( const std::exception & ex )
		{
			Log::Error( "DataCache", "Caught Exception: %s", ex.what() );
		}
	}

	m_CurrentCacheSize = 0;
	m_Cache.clear();
	return true;
}

