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

#include "StringUtil.h"
#include "base64/encode.h"
#include "base64/decode.h"

#include <stdarg.h>
#include <stdio.h>
#include <sstream>
#include <algorithm>

#pragma warning(disable:4996)
#include <time.h>

#ifndef WIN32
#include <stdlib.h>
#include <strings.h>
#define _stricmp strcasecmp
#endif

int StringUtil::Compare( const std::string & a_Subject, const std::string & a_Target, bool a_bInsenstive /*= false */)
{
	if (! a_bInsenstive )
		return a_Subject.compare( a_Target );

	return _stricmp( a_Subject.c_str(), a_Target.c_str() );
}

bool StringUtil::EndsWith( const std::string & a_Subject, const std::string & a_Ending, bool a_bInsenstive /*= false */)
{
	if ( a_Subject.size() < a_Ending.size() )
		return false;		// can't because the target is too short already..

	return Compare( a_Subject.substr( a_Subject.size() - a_Ending.size() ), a_Ending, a_bInsenstive ) == 0;
}

bool StringUtil::StartsWith( const std::string & a_Subject, const std::string & a_Start, bool a_bInsenstive /*= false*/ )
{
	if ( a_Subject.size() < a_Start.size() )
		return false;

	return Compare( a_Subject.substr( 0, a_Start.size() ), a_Start, a_bInsenstive ) == 0;
}

size_t StringUtil::Find(const std::string & a_Subject, const std::string & a_Search,
	size_t a_Offset /*= 0*/, bool a_bInsenstive /*= false*/)
{
	if ( a_bInsenstive )
	{
		std::string::const_iterator iFound = std::search( a_Subject.begin() + a_Offset, a_Subject.end(), 
			a_Search.begin(), a_Search.end(), nocase_equal<char>() );
		if ( iFound != a_Subject.end() )
			return iFound - a_Subject.begin();

		return std::string::npos;
	}

	return a_Subject.find( a_Search, a_Offset );
}


int StringUtil::Replace(std::string & subject, const std::string& search, const std::string& replace, 
	bool a_bInsenstive /*= false*/ ) 
{
	int count = 0;

	size_t pos = 0;
	while ((pos = Find( subject, search, pos, a_bInsenstive)) != std::string::npos) 
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
		count += 1;
	}

	return count;
}

std::string StringUtil::Format( const char * a_pFormat, ... )
{
	va_list args;
	va_start(args, a_pFormat);

	char buffer[1024 * 32];

	int rv = vsnprintf(buffer, sizeof(buffer), a_pFormat, args);
	if ( rv >= sizeof(buffer) )
	{
		// buffer too small, allocate a buffer from the heap and try again..
		char * heapBuffer = new char[ rv + 1 ];
		vsnprintf(heapBuffer, rv, a_pFormat, args);
		va_end(args);

		std::string result( heapBuffer );
		delete [] heapBuffer;

		return result;
	}
	va_end(args);

	return buffer;
}

bool StringUtil::IsEscaped( const std::string & a_Input )
{
	for(size_t i=0;i<a_Input.size();++i)
	{
		char c = a_Input[i];
		if (!isalnum(c) && c != '-' && c != '_' && c != '.' && c != '~' && c != '+' )
			return false;
	}

	return true;
}

std::string StringUtil::UrlEscape( const std::string & a_Input )
{
	std::string output;
	for(size_t i=0;i<a_Input.size();++i)
	{
		unsigned char c = a_Input[i];
		if ( c == ' ' )
			output += '+';
		else if ( isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' )
			output += c;
		else
			output += StringUtil::Format( "%%%2X", c );
	}

	return output;
}

void StringUtil::AppendParameter( const std::string & a_Param, std::string & a_Output )
{
	if ( a_Output.size() > 0 )
		a_Output += "&";
	a_Output += a_Param;
}

void StringUtil::Split( const std::string & a_String, const std::string & a_Seperators, 
	std::vector< std::string > & a_Split )
{
	size_t start = 0;
	while( start != std::string::npos )
	{
		size_t end = a_String.find_first_of( a_Seperators, start );
		if ( end == std::string::npos )
			end = a_String.size();

		std::string item = a_String.substr( start, end - start );
		if ( item.size() > 0 )
			a_Split.push_back( item );

		if (end >= a_String.size())
			break;
		start = a_String.find_first_not_of(a_Seperators, end);
	}
}

void StringUtil::ConvertToTime( const std::string & a_String, std::string & a_Time)
{
	int hour = 0;
	int minute = 0;

	if(a_String.empty())
	{
		a_Time ="00:00:00";
		return;
	}

	std::string nSingleDigit[13]={"zero","one","two","three","four","five","six","seven","eight","nine","ten","eleven","twelve"};
	std::string nSpecialDigit[10]={"ten","eleven","twelve","thirteen","fourteen","fifteen","sixteen","seventeen","eighteen","nineteen"};
	std::string nDoubleDigit[4]={"twenty", "thirty", "forty", "fifty"};

	std::vector< std::string > times;
	if(a_String.find_first_of(" "))
		Split( a_String, " ", times );
	else
		times.push_back(a_String);


	for(int i = 0; i < sizeof(nSingleDigit)/sizeof(nSingleDigit[0]); i++)
	{
		if(!nSingleDigit[i].compare(times.at(0)))
		{
			time_t t = time(0);
			struct tm * now = localtime( &t );

			// Standard Robot time is 5 hours faster
			int robotTime = now->tm_hour;

			// Determine if it's an AM or PM reminder
			if((robotTime < 12 && robotTime < i) || (robotTime > 12 && robotTime > i + 12))
				hour = i;
			else if (robotTime == i + 12 || robotTime == i)
				hour = robotTime;
			else
				hour = i + 12;
			break;
		}
	}

	if(times.size() == 3)
	{
		for(int i = 0; i < sizeof(nDoubleDigit)/sizeof(nDoubleDigit[0]); i++)
		{
			if(!nDoubleDigit[i].compare(times.at(1)))
			{
				minute = 20 + (i * 10);
				break;
			}
		}
		for(int i = 0; i < sizeof(nSingleDigit)/sizeof(nSingleDigit[0]); i++)
		{
			if(!nSingleDigit[i].compare(times.at(2)))
			{
				minute += i;
				break;
			}
		}
	}
	else if(times.size() == 2)
	{
		for(int i = 0; i < sizeof(nDoubleDigit)/sizeof(nDoubleDigit[0]); i++)
		{
			if(!nDoubleDigit[i].compare(times.at(1)))
			{
				minute = 20 + (i * 10);
				break;
			}
		}
		if(minute == 0)
		{
			for(int i = 0; i < sizeof(nSpecialDigit)/sizeof(nSpecialDigit[0]); i++)
			{
				if(!nSpecialDigit[i].compare(times.at(1)))
				{
					minute = i + 10;
				}
			}
		}

	}
	std::string hConvert = Format("%d", hour);
	std::string mConvert = Format("%d", minute);
	a_Time = hConvert + ":" + mConvert + ":00";
}

std::string StringUtil::RightTrim( const std::string & a_String, const std::string & a_Trim /*= " "*/ )
{
	size_t i = a_String.find_last_not_of( a_Trim );
	if ( i != std::string::npos )
		return a_String.substr( 0, i + 1 );
	return "";
}

std::string StringUtil::LeftTrim( const std::string & a_String, const std::string & a_Trim /*= " "*/ )
{
	size_t i = a_String.find_first_not_of( a_Trim );
	if ( i != std::string::npos )
		return a_String.substr( i );
	return "";
}

std::string StringUtil::Trim( const std::string & a_String, const std::string & a_Trim /*= " "*/ )
{
	return RightTrim( LeftTrim( a_String, a_Trim ), a_Trim );
}

std::string StringUtil::EncodeBase64(const std::string & a_Input)
{
	std::ostringstream output;
	std::istringstream input(a_Input);
	base64::encoder().encode(input, output);
	return Trim( output.str(), " \r\n" );
}

std::string StringUtil::DecodeBase64(const std::string & a_Input)
{
	std::ostringstream output;
	std::istringstream input( a_Input );
	base64::decoder().decode( input, output );
	return Trim( output.str(), " \r\n" );
}

std::string StringUtil::GetFilename( const std::string &a_Path)
{
    size_t lastSlash = a_Path.find_last_of("\\/");
    if ( lastSlash != std::string::npos)
        return a_Path.substr(lastSlash + 1);
    return a_Path;
}

bool StringUtil::WildMatch(const std::string & a_Pattern, const std::string & a_Search)
{
	return WildMatch(a_Pattern.c_str(), a_Search.c_str());
}

bool StringUtil::WildMatch( const char * pat, const char * str )
{
	while (*str)
	{
		switch (*pat)
		{
		case '?':
			//Do not match ? with .
			if (*str == '.')
				return false;
			break;
		case '*':
			do {
				++pat;
			} while (*pat == '*'); /* enddo */

			if (!*pat)
				return true;

			while (*str)
			{
				if (WildMatch(pat, str++))
					return true;
			}
			return false;
		default:
			if (*str != *pat)
				return false;
			break;
		} /* endswitch */
		++pat, ++str;
	} /* endwhile */

	while (*pat == '*')
		++pat;

	return !(*pat);
}
