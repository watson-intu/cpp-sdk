/* ***************************************************************** */
/*                                                                   */
/* IBM Confidential                                                  */
/* OCO Source Materials                                              */
/*                                                                   */
/* (C) Copyright IBM Corp. 2001, 2014                                */
/*                                                                   */
/* The source code for this program is not published or otherwise    */
/* divested of its trade secrets, irrespective of what has been      */
/* deposited with the U.S. Copyright Office.                         */
/*                                                                   */
/* ***************************************************************** */

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION

#include "JpegHelpers.h"
#include "stb/stb_image.h"
#include "jo/jo_jpeg.h"

static void stbi_write( void *context, void *data, int size)
{
	std::stringstream * ss = (std::stringstream *)context;
	ss->write( (const char *)data, size );
}

bool JpegHelpers::ExtractImage( const std::string & a_ImageJpeg, int a_X, int a_Y, int a_Width, int a_Height, 
	std::string & a_ExtractedJpeg )
{
	int img_width, img_height, img_depth;
	stbi_uc * pDecoded = stbi_load_from_memory( (stbi_uc *)a_ImageJpeg.data(), a_ImageJpeg.size(), &img_width, &img_height, &img_depth, 0 );
	if ( pDecoded == NULL )
		return false;
	if ( a_X < 0 || (a_X + a_Width) >= img_width || a_Y < 0 || (a_Y + a_Height) >= img_height )
	{
		stbi_image_free( pDecoded );
		return false;		// rect is out of bounds
	}

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

