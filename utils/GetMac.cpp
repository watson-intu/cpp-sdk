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

#include "GetMac.h"
#include "StringUtil.h"
#include "Log.h"

#ifdef _WIN32
#include <winsock2.h> 
#include <iphlpapi.h> 
#pragma comment(lib, "Iphlpapi" )

#else
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <net/if.h> 
#ifdef __APPLE__
#include <net/if_dl.h>
#else
#include <netpacket/packet.h> 
#endif
#include <ifaddrs.h> 
#endif

std::string GetMac::GetMyAddress()
{
	std::list<std::string> emptyPatternList;
	emptyPatternList.push_back("*");

	return GetMac::GetMyAddress(emptyPatternList);
}

std::string GetMac::GetMyAddress(const std::list<std::string> & a_Patterns)
{
	std::string mac("00:00:00:00:00:00");
#ifdef _WIN32
	std::vector<unsigned char> buf; 
	DWORD bufLen = 0; 
	GetAdaptersAddresses(0, 0, 0, 0, &bufLen); 
	if(bufLen) 
	{ 
		buf.resize(bufLen, 0); 
		IP_ADAPTER_ADDRESSES * ptr = 
			reinterpret_cast<IP_ADAPTER_ADDRESSES*>(&buf[0]); 
		DWORD err = GetAdaptersAddresses(0, 0, 0, ptr, &bufLen); 
		if(err == NO_ERROR) 
		{ 
			while(ptr) 
			{ 
				if( ptr->OperStatus == IfOperStatusUp
					&& ptr->PhysicalAddressLength == 6) 
				{ 
					mac = StringUtil::Format( "%02X:%02X:%02X:%02X:%02X:%02X",
						ptr->PhysicalAddress[0],
						ptr->PhysicalAddress[1],
						ptr->PhysicalAddress[2],
						ptr->PhysicalAddress[3],
						ptr->PhysicalAddress[4],
						ptr->PhysicalAddress[5] );
					break;
				} 
				ptr = ptr->Next; 
			} 
		} 
	} 
#else
	ifaddrs * ifap = 0; 
	if(getifaddrs(&ifap) == 0) 
	{	
		for (std::list<std::string>::const_iterator patternIter = a_Patterns.begin(); patternIter != a_Patterns.end();
			 ++patternIter)
		{
			ifaddrs * iter = ifap;
			while(iter) 
			{
				bool interfaceMatch = StringUtil::WildMatch((*patternIter), iter->ifa_name);
#if defined(__APPLE__) 
				sockaddr_dl * sdl =
					reinterpret_cast<sockaddr_dl*>(iter->ifa_addr);
				if ( interfaceMatch
					&& (iter->ifa_flags & IFF_UP) != 0
					&& (iter->ifa_flags & IFF_LOOPBACK) == 0
					&& sdl->sdl_family == AF_LINK)
				{
					unsigned char * addr = (unsigned char *)LLADDR( sdl );
									mac = StringUtil::Format( "%02X:%02X:%02X:%02X:%02X:%02X",
											addr[0],
						addr[1],
						addr[2],
						addr[3],
						addr[4],
						addr[5] );
					return mac;
				}
#else
				sockaddr_ll *sal =
						reinterpret_cast<sockaddr_ll *>(iter->ifa_addr);

				if ((iter->ifa_flags & IFF_UP) != 0
					&& (iter->ifa_flags & IFF_LOOPBACK) == 0
					&& (sal->sll_family == AF_PACKET)
					&& interfaceMatch) {
					mac = StringUtil::Format("%02X:%02X:%02X:%02X:%02X:%02X",
											 sal->sll_addr[0],
											 sal->sll_addr[1],
											 sal->sll_addr[2],
											 sal->sll_addr[3],
											 sal->sll_addr[4],
											 sal->sll_addr[5]);
					return mac;
				}
	#endif
				iter = iter->ifa_next;
			}
		}

		freeifaddrs(ifap);
	} 
#endif
	return mac;
}

