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

#include <string.h>

#include "UnitTest.h"
#include "utils/Log.h"
#include "utils/Library.h"

int main( int argc, char ** argv )
{
	Log::RegisterReactor( new ConsoleReactor( LL_DEBUG_LOW ) );
	Log::RegisterReactor( new FileReactor( "UnitTest.log", LL_DEBUG_LOW ) );

	// invoke GetInstance() to force the linker to link the DLL
	Library self( "self" );

	// load only one platform type.. 
	Library platform_nao( "platform_nao" );
	Library platform_win( "platform_win" );
	Library platform_mac( "platform_mac" );
	Library faces("face_plugin");

	// TODO: let the user specify the platform with the command line

	if( argc == 0 )
		return UnitTest::RunTests(NULL);
	else
		return UnitTest::RunTests(argv[1]);
}
