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

#include "Logic.h"
#include "Log.h"
#include "StringUtil.h"

//! DB style matching, % is a wildcard character..
static bool LikeMatch( const char * pat, const char * str )
{
	while (*str)
	{
		switch (*pat)
		{
		case '%':
			do {
				++pat;
			} while (*pat == '%'); /* enddo */

			if (!*pat)
				return true;

			while (*str)
			{
				if (LikeMatch(pat, str++))
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

	while (*pat == '%')
		++pat;

	return !(*pat);
}


const char * Logic::EqualityOpText(EqualityOp a_Op)
{
	static const char * TEXT[] = 
	{
		"EQ",		// ==
		"NE",		// !=
		"GR",		// >
		"GE",		// >=
		"LS",		// <
		"LE",		// <=
		"LIKE"
	};
	int index = (int)a_Op;
	if (index < 0 || index >= sizeof(TEXT) / sizeof(TEXT[0]))
		return "?";
	return TEXT[index];
}

Logic::EqualityOp Logic::GetEqualityOp(const std::string & a_Op)
{
	for (int i = 0; i < LAST_EO; ++i)
		if (a_Op.compare(EqualityOpText((EqualityOp)i)) == 0)
			return (EqualityOp)i;

	// default to EQ
	return EQ;
}

bool Logic::TestEqualityOp(EqualityOp a_Op, const Json::Value & a_LHS, const Json::Value & a_RHS )
{
	// if left and right hand sides are objects/arrays, then check that the RHS has all the fields
	// found in the left hand-side and the equality operators are true.
	if ( (a_LHS.isObject() && a_RHS.isObject()) ||
		(a_LHS.isArray() && a_RHS.isArray()) )
	{
		for( Json::ValueConstIterator iValue = a_LHS.begin(); iValue != a_LHS.end(); ++iValue )
		{
			std::string key = iValue.key().asString();
			if (! a_RHS.isMember( key ) )
				return false;
			if (! TestEqualityOp( a_Op, *iValue, a_RHS[key] ) )
				return false;
		}

		return true;
	}

	switch (a_Op)
	{
	case EQ:
		return a_LHS.compare(a_RHS) == 0;
	case NE:
		return a_LHS.compare(a_RHS) != 0;
	case LS:
		return a_LHS.compare(a_RHS) < 0;
	case LE:
		return a_LHS.compare(a_RHS) <= 0;
	case GR:
		return a_LHS.compare(a_RHS) > 0;
	case GE:
		return a_LHS.compare(a_RHS) >= 0;
	case LIKE:
		return LikeMatch( a_LHS.asCString(), a_RHS.asCString() );
	default:
		break;
	}

	Log::Error("Logic", "Unsupported EqualityOp %u", a_Op);
	return false;
}

const char * Logic::LogicalOpText(LogicalOp a_Op)
{
	static const char * TEXT[] = 
	{
		"AND",		// all conditions must be true
		"OR",		// any condition may be true
		"NOT"
	};

	int index = (int)a_Op;
	if (index < 0 || index >= sizeof(TEXT) / sizeof(TEXT[0]))
		return "?";
	return TEXT[index];
}

Logic::LogicalOp Logic::GetLogicalOp(const std::string & a_Op)
{
	for (int i = 0; i < LAST_EO; ++i)
		if (a_Op.compare(LogicalOpText((LogicalOp)i)) == 0)
			return (LogicalOp)i;
	// default to AND
	return AND;
}

bool Logic::TestLogicalOp(LogicalOp a_Op, const std::vector<bool> & a_Values)
{
	switch (a_Op)
	{
	case AND:
	{
		bool bResult = true;
		for (size_t i = 0; i < a_Values.size() && bResult; ++i)
			bResult &= a_Values[i];
		return bResult;
	}
	break;
	case OR:
	{
		bool bResult = false;
		for (size_t i = 0; i < a_Values.size() && !bResult; ++i)
			bResult |= a_Values[i];
		return bResult;
	}
	break;
	case NOT:
	{
		bool bResult = true;
		for (size_t i = 0; i < a_Values.size() && bResult; ++i)
			if (a_Values[i])
				bResult = false;
		return bResult;
	}
	break;
	default:
		break;
	}

	Log::Error("Logic", "Invalid LogicalOp %u", a_Op );
	return false;
}

