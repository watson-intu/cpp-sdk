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


#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION

#include "JpegHelpers.h"
#include "stb/stb_image.h"
#include "jo/jo_jpeg.h"

bool JpegHelpers::EncodeImage( const void * a_RGB, int a_Width, int a_Height, int a_Depth,
	std::string & a_EncodedJpeg )
{
	std::stringstream ss;
	if ( jo_write_jpg( ss, a_RGB, a_Width, a_Height, a_Depth, 90 ) )
	{
		a_EncodedJpeg = ss.str();
		return true;
	}

	return false;
}

bool JpegHelpers::DecodeImage( const void * a_JPEG, int a_Bytes, int & a_Width, int & a_Height, int & a_Depth,
	std::string & a_DecodedJpeg )
{
	stbi_uc * pDecoded = stbi_load_from_memory( (stbi_uc *)a_JPEG, a_Bytes, &a_Width, &a_Height, &a_Depth, 0 );
	if ( pDecoded == NULL )
		return false;

	a_DecodedJpeg = std::string( (const char *)pDecoded, (a_Width * a_Depth) * a_Height );
	stbi_image_free( pDecoded );

	return true;
}

bool JpegHelpers::ExtractImage( const std::string & a_ImageJpeg, int a_X, int a_Y, int a_Width, int a_Height, 
	std::string & a_ExtractedJpeg, std::vector<float> * a_pCenter /*= NULL*/ )
{
	int img_width, img_height, img_depth;
	stbi_uc * pDecoded = stbi_load_from_memory( (stbi_uc *)a_ImageJpeg.data(), a_ImageJpeg.size(), &img_width, &img_height, &img_depth, 0 );
	if ( pDecoded == NULL )
		return false;

	if ( a_pCenter != NULL )
	{
		a_pCenter->clear();
		a_pCenter->push_back( ((a_X + (a_Width * 0.5f)) / (float)img_width) - 0.5f );
		a_pCenter->push_back( ((a_Y + (a_Height * 0.5f)) / (float)img_height) - 0.5f );
	}

	if ( a_X < 0 )
		a_X = 0;
	if ( (a_X + a_Width) >= img_width )
		a_Width = (img_width - a_X) - 1;
	if ( a_Y < 0 )
		a_Y = 0;
	if ( (a_Y + a_Height) >= img_height )
		a_Height = (img_height - a_Y) - 1;

	unsigned char * pUpperLeft = pDecoded + (a_X * img_depth) + (a_Y * img_width * img_depth);
	unsigned char * pCroppedImage = new unsigned char[ a_Width * a_Height * img_depth ];

	for(int y=0;y<a_Height;++y)
		memcpy( pCroppedImage + (y * a_Width * img_depth), pUpperLeft + (y * img_width * img_depth), a_Width * img_depth );
	stbi_image_free( pDecoded );

	bool bSuccess = false;

	std::stringstream ss;
	if ( jo_write_jpg( ss, pCroppedImage, a_Width, a_Height, img_depth, 90 ) )
	{
		a_ExtractedJpeg = ss.str();
		bSuccess = true;
	}

	delete [] pCroppedImage;
	return bSuccess;
}

