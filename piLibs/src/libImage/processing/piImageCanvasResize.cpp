#include <malloc.h>
#include <string.h>

#include "../piImage.h"

namespace piLibs {

bool piImage_CanvasResize( piImage *image, unsigned int nxres, unsigned int nyres, void *color )
{
	piImage tmp;
    if (!tmp.Make( image->GetType(), nxres, nyres, 1, image->GetFormat()))
		return false;

	if( image->GetFormat()==piImage::FORMAT_I_RGBA )
	{
		const unsigned int col = ((unsigned int*)color)[0];
		const unsigned int num = nxres*nyres;
		unsigned int *ptr = (unsigned int*)tmp.GetData();
		for( unsigned int i=0; i<num; i++ ) ptr[i] = col;

		const unsigned int xres = image->GetXRes();
		const unsigned int yres = image->GetYRes();

		unsigned int dxoff, dyoff, sxoff, syoff, xnum, ynum;

		if( nxres<xres ) { dxoff = 0; xnum = nxres; sxoff = (xres-nxres)/2; } 
		            else { dxoff = (nxres-xres)/2; xnum = xres; sxoff = 0; }
		if( nyres<yres ) { dyoff = 0; ynum = nyres; syoff = (yres-nyres)/2; } 
		            else { dyoff = (nyres-yres)/2; ynum = yres; syoff = 0; }

		for( unsigned int j=0; j<ynum; j++ )
		{
            unsigned int *src = (unsigned int*)image->GetData() + xres*(syoff + j) + sxoff;
            unsigned int *dst = (unsigned int*)tmp.GetData() + nxres*(dyoff + j) + dxoff;
			for( unsigned int i=0; i<xnum; i++ )
				dst[i] = src[i];
		}
	}

    image->Replace( &tmp );

	return true;
}
		
}