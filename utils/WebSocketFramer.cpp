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

#include "WebSocketFramer.h"
#include "Log.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#pragma pack( push, 1 )
struct WEB_SOCKET_HEADER
{
	unsigned short	opcode : 4;
	unsigned short	rsv3 : 1;
	unsigned short	rsv2 : 1;
	unsigned short	rsv1 : 1;
	unsigned short	fin : 1;
	unsigned short	payload_len : 7;
	unsigned short	mask : 1;
};
#pragma pack(pop)


IWebSocket::Frame * WebSocketFramer::ParseFrame(std::string & a_Input)
{
	if (a_Input.size() < 2)
		return NULL;

	size_t hsize = 0;
	WEB_SOCKET_HEADER * pHeader = reinterpret_cast<WEB_SOCKET_HEADER *>(&a_Input[hsize]);
#if ENABLE_DEBUGGING
	Log::Debug("WebClient", "WebSocket Header: fin = %u, opcode = %u, mask = %u, payload_len = %u, incoming = %u",
		pHeader->fin, pHeader->opcode, pHeader->mask, pHeader->payload_len, m_Incoming.size());
#endif
	hsize += 2;

	size_t payload_len = 0;
	if (pHeader->opcode == IWebSocket::TEXT_FRAME || pHeader->opcode == IWebSocket::BINARY_FRAME || pHeader->opcode == IWebSocket::CONTINUATION)
	{
		if (pHeader->payload_len == 126)
		{
			if (a_Input.size() < (hsize + 2))
				return NULL;
			for (int i = 0; i < 2; ++i)
			{
				uint8_t c = a_Input[hsize++];
				payload_len = (payload_len << 8) | c;
			}
		}
		else if (pHeader->payload_len == 127)
		{
			if (a_Input.size() < (hsize + 8))
				return NULL;
			// TODO: check for 32-bit overflow here.. 
			for (int i = 0; i < 8; ++i)
			{
				uint8_t c = a_Input[hsize++];
				payload_len = (payload_len << 8) | c;
			}
		}
		else
			payload_len = pHeader->payload_len;
	}

	uint8_t mask[4];
	if (pHeader->mask)
	{
		if (a_Input.size() < (hsize + 4))
			return NULL;
		memcpy(mask, &a_Input[hsize], sizeof(mask));
		hsize += 4;
	}

	if (a_Input.size() < (payload_len + hsize))
		return NULL;		// not enough data yet..

	IWebSocket::Frame * pFrame = new IWebSocket::Frame();
	pFrame->m_Op = (IWebSocket::OpCode)pHeader->opcode;

	if (payload_len > 0)
	{
		pFrame->m_Data += a_Input.substr(hsize, payload_len);
		if (pHeader->mask)
		{
			for (size_t k = 0; k < pFrame->m_Data.size(); ++k)
				pFrame->m_Data[k] ^= mask[k & 0x3];
		}
	}

	a_Input.erase(a_Input.begin(), a_Input.begin() + hsize + payload_len);
	return pFrame;
}

void WebSocketFramer::CreateFrame(std::string & a_Output, IWebSocket::OpCode a_Op, const std::string & a_Data, bool a_bUseMask /*= true*/)
{
	WEB_SOCKET_HEADER header;
	memset(&header, 0, sizeof(header));

	uint8_t mask[4] = { 0, 0, 0, 0 };
	if (a_bUseMask)
	{
		header.mask = true;
		for (int i = 0; i < 4; ++i)
			mask[i] = (unsigned char)rand() & 0x7f;
	}

	header.opcode = a_Op;
	header.fin = true;
	if (a_Data.size() >= 65536)
		header.payload_len = 127;
	else if (a_Data.size() >= 126)
		header.payload_len = 126;
	else
		header.payload_len = a_Data.size();

	a_Output += std::string((const char *)&header, sizeof(header));
	if (a_Data.size() >= 65536)
	{
		uint64_t size = a_Data.size();
		a_Output.push_back((size >> 56) & 0xff);
		a_Output.push_back((size >> 48) & 0xff);
		a_Output.push_back((size >> 40) & 0xff);
		a_Output.push_back((size >> 32) & 0xff);
		a_Output.push_back((size >> 24) & 0xff);
		a_Output.push_back((size >> 16) & 0xff);
		a_Output.push_back((size >> 8) & 0xff);
		a_Output.push_back((size >> 0) & 0xff);
	}
	else if (a_Data.size() >= 126)
	{
		uint16_t size = (uint16_t)a_Data.size();
		a_Output.push_back((size >> 8) & 0xff);
		a_Output.push_back(size & 0xff);
	}

	if (a_bUseMask)
		a_Output += std::string((const char *)mask, 4);

	size_t hsize = a_Output.size();
	a_Output += a_Data;

	if (a_bUseMask)
	{
		for (size_t i = 0; i < a_Data.size(); ++i)
			a_Output[hsize + i] ^= mask[i & 0x3];
	}
}

