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


#include "Library.h"
#include "Log.h"
#include "Factory.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#endif

Library * Library::sm_pLoadingLibrary = NULL;

Library::Library( const std::string & a_Lib ) : m_pLibrary( NULL )
{
	Load( a_Lib );
}

Library::~Library()
{
	if (! Unload() )
		Log::Error( "Library", "Failed to unload %s", m_Lib.c_str() );
}

void Library::Load( const std::string & a_Lib )
{
	static boost::mutex lock;
	boost::mutex::scoped_lock l(lock);

	sm_pLoadingLibrary = this;
	m_Lib = a_Lib;

#if defined(_WIN32)
	const std::string POSTFIX(".dll");
	m_pLibrary = LoadLibrary( (m_Lib + POSTFIX).c_str() );
#elif defined(__APPLE__)
    const std::string POSTFIX(".dylib");
	const std::string PREFIX("lib");
	std::string  libFile = PREFIX + m_Lib + POSTFIX;
	m_pLibrary = dlopen( libFile.c_str(), RTLD_LAZY );
	if ( m_pLibrary == NULL )
		Log::Error( "Library", "dlerror: %s", dlerror() );
#else
	const std::string PREFIX("lib");
	const std::string POSTFIX(".so");
	std::string  libFile = PREFIX + m_Lib + POSTFIX;
	m_pLibrary = dlopen( libFile.c_str(), RTLD_LAZY );
	if ( m_pLibrary == NULL )
		Log::Error( "Library", "dlerror: %s", dlerror() );
#endif

	if ( m_pLibrary == NULL )
		Log::Warning( "Library", "Failed to load dynamic library %s.", m_Lib.c_str() );
	else
		Log::Status( "Library", "Dynamic library %s loaded.", m_Lib.c_str() );
	sm_pLoadingLibrary = NULL;
}

bool Library::Unload()
{
	if ( m_pLibrary != NULL )
	{
		// make sure there are no objects created from this library before we try to unload the code.
		size_t nObjectCount = 0;
		for( CreatorSet::const_iterator iCreator = m_Creators.begin(); 
			iCreator != m_Creators.end(); ++iCreator )
		{
			const ICreator::ObjectSet & objects = (*iCreator)->GetObjects();
			for( ICreator::ObjectSet::const_iterator iObject = objects.begin(); iObject != objects.end(); ++iObject )
			{
				const IWidget * pWidget = *iObject;
				Log::Warning( "Library", "Found instance %p (%s)", pWidget, pWidget->GetRTTI().GetName().c_str() );
				nObjectCount += 1;
			}
		}

		if ( nObjectCount > 0 )
		{
			Log::Error( "Library", "Unable to unload %s, found %u object instances.", m_Lib.c_str(), nObjectCount );
			return false;
		}

#if defined(_WIN32)
		FreeLibrary( (HINSTANCE)m_pLibrary );
#else
		dlclose( m_pLibrary );
#endif
		m_pLibrary = NULL;
	}

	Log::Status( "Library", "Dynamic library %s unloaded.", m_Lib.c_str() );
	return true;
}

void Library::AddCreator( ICreator * a_pCreator )
{
	m_Creators.insert( a_pCreator );
}

void Library::RemoveCreator( ICreator * a_pCreator )
{
	m_Creators.erase( a_pCreator );
}
