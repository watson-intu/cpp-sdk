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

#include "Time.h"
#include "StringUtil.h"
#include "Log.h"

#pragma warning(disable:4996)
#include <time.h>
#include <sys/stat.h>

std::string Time::GetFormattedTime( const char * a_pFormat )
{
	char buffer[ 1024 ];
	strftime( buffer, sizeof(buffer), a_pFormat, localtime( &m_tb.time ) );

	return buffer;
}


time_t Time::ParseTime(const std::string & a_Time)
{
	std::vector<std::string> formattedTimes;
	StringUtil::Split(a_Time, ":", formattedTimes);
	if ( formattedTimes.size() != 3 )
		return 0;

	time_t raw_time = time(0);
	struct tm * timePtr = localtime( &raw_time );
	struct tm t = {0};
	t.tm_year = timePtr->tm_year;
	t.tm_mon = timePtr->tm_mon;
	t.tm_mday = timePtr->tm_mday;
	t.tm_hour = atoi(formattedTimes.at(0).c_str()) - 1;
	t.tm_min = atoi(formattedTimes.at(1).c_str());
	t.tm_sec = atoi(formattedTimes.at(2).c_str());

	return mktime(&t);
}

time_t Time::GetFileModifyTime( const std::string & a_File )
{
	struct stat s;
	int result = stat( a_File.c_str(), &s );
	if ( result != 0 )
		return 0;

	return mktime( gmtime( &s.st_mtime ) );
}

