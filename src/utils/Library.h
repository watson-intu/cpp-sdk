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


#ifndef WDC_LIBRARY_H
#define WDC_LIBRARY_H

#include <string>
#include <set>

#include "UtilsLib.h"

class ICreator;

class UTILS_API Library
{
public:
	//! Types
	typedef std::set<ICreator *>	CreatorSet;

	//! Construction
	Library( const std::string & a_Lib );
	~Library();

	bool IsLoaded() const
	{
		return m_pLibrary != NULL;
	}
	const std::string & GetLibraryName() const
	{
		return m_Lib;
	}
	const CreatorSet & GetCreators() const
	{
		return m_Creators;
	}

	void Load( const std::string & a_Lib );
	bool Unload();

	void AddCreator( ICreator * a_pCreator );
	void RemoveCreator( ICreator * a_pCreator );

	//! This returns a pointer to the currently loading library if any, this may be NULL.
	static Library * LoadingLibrary()
	{
		return sm_pLoadingLibrary;
	}


private:

	//! Data
	std::string m_Lib;
	void * m_pLibrary;
	CreatorSet m_Creators;
	bool m_bDestroyed;

	static Library * sm_pLoadingLibrary;

	//! no copy constructor
	Library( const Library & a_Copy )
	{}
};

#endif

