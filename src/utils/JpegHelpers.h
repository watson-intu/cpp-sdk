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

#ifndef WDC_JPEG_HELPER_H
#define WDC_JPEG_HELPER_H

#include <string>
#include <vector>

#include "WDCLib.h"

//! This is a container for skill/gesture arguments.
class WDC_API JpegHelpers
{
public:
	static bool ExtractImage( const std::string & a_ImageJpeg, int a_X, int a_Y, int a_Width, int a_Height, 
		std::string & a_ExtractedJpeg, std::vector<float> * a_pCenter = NULL );
};

#endif

