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


#ifndef WDC_JPEG_HELPER_H
#define WDC_JPEG_HELPER_H

#include <string>
#include <vector>

#include "UtilsLib.h"

//! This is a container for skill/gesture arguments.
class UTILS_API JpegHelpers
{
public:
	//! encode the given raw RGB data into a JPEG encoded image.
	static bool EncodeImage( const void * a_RGB, int a_Width, int a_Height, int a_Depth,
		std::string & a_EncodedJpeg );
	//! Decode the given JPEG data into raw image
	static bool DecodeImage( const void * a_JPEG, int a_Bytes, int & a_Width, int & a_Height, int & a_Depth,
		std::string & a_DecodedJpeg );
	static bool ExtractImage( const std::string & a_ImageJpeg, int a_X, int a_Y, int a_Width, int a_Height, 
		std::string & a_ExtractedJpeg, std::vector<float> * a_pCenter = NULL );
};

#endif

