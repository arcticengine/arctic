#include <malloc.h>
#include "../piImage.h"

namespace piLibs {

static void resize_irgba(piImage *dst, const piImage *src )
{
    const int sxres = src->GetXRes();
    const int syres = src->GetYRes();
    const int dxres = dst->GetXRes();
    const int dyres = dst->GetYRes();


    // case 1 (comrpession)
    if( (dxres<sxres) && (dyres<syres) )
    {
        const unsigned char *sptr = (unsigned char*)src->GetData();
        unsigned char *dptr = (unsigned char*)dst->GetData();

        for( int j=0; j<dyres; j++ )
        for( int i=0; i<dxres; i++ )
        {
            const int xa = (sxres-1)*(i+0)/dxres;
            const int ya = (syres-1)*(j+0)/dyres;
            const int xb = (sxres-1)*(i+1)/dxres;
            const int yb = (syres-1)*(j+1)/dyres;

            int tot[4] = { 0, 0, 0, 0 };
            for( int y=ya; y<=yb; y++ )
            for( int x=xa; x<=xb; x++ )
            {
                const int off = 4*(sxres*y+x);
                tot[0] += sptr[ off+0 ];
                tot[1] += sptr[ off+1 ];
                tot[2] += sptr[ off+2 ];
                tot[3] += sptr[ off+3 ];
            }

            #if 0
            const int numsam = (xb-xa+1)*(yb-ya+1);
            tot[0] /= numsam;
            tot[1] /= numsam;
            tot[2] /= numsam;
            tot[3] /= numsam;
            #else
            const int numsam = 65536 / ((xb-xa+1)*(yb-ya+1));
            tot[0] = (tot[0]*numsam)>>16;
            tot[1] = (tot[1]*numsam)>>16;
            tot[2] = (tot[2]*numsam)>>16;
            tot[3] = (tot[3]*numsam)>>16;
            #endif

            const int doff = 4*(dxres*j+i);
            dptr[ doff+0 ] = tot[0];
            dptr[ doff+1 ] = tot[1];
            dptr[ doff+2 ] = tot[2];
            dptr[ doff+3 ] = tot[3];
        }
    }
    // case 2 (expansion)
    else if( (dxres>sxres) && (dyres>syres) )
    {
    }
    // case 3
    else if( (dxres<sxres) && (dyres>syres) )
    {
    }
    // csae 4
    else if( (dxres>sxres) && (dyres<syres) )
    {
    }
}


static void resize_irgb(piImage *dst, const piImage *src )
{
    const int sxres = src->GetXRes();
    const int syres = src->GetYRes();
    const int dxres = dst->GetXRes();
    const int dyres = dst->GetYRes();


    // case 1 (comrpession)
    if( (dxres<sxres) && (dyres<syres) )
    {
        const unsigned char *sptr = (unsigned char*)src->GetData();
        unsigned char *dptr = (unsigned char*)dst->GetData();

        for( int j=0; j<dyres; j++ )
        for( int i=0; i<dxres; i++ )
        {
            const int xa = (sxres-1)*(i+0)/dxres;
            const int ya = (syres-1)*(j+0)/dyres;
            const int xb = (sxres-1)*(i+1)/dxres;
            const int yb = (syres-1)*(j+1)/dyres;

            int tot[3] = { 0, 0, 0 };
            for( int y=ya; y<=yb; y++ )
            for( int x=xa; x<=xb; x++ )
            {
                const int off = 3*(sxres*y+x);
                tot[0] += sptr[ off+0 ];
                tot[1] += sptr[ off+1 ];
                tot[2] += sptr[ off+2 ];
            }

            #if 0
            const int numsam = (xb-xa+1)*(yb-ya+1);
            tot[0] /= numsam;
            tot[1] /= numsam;
            tot[2] /= numsam;
            #else
            const int numsam = 65536 / ((xb-xa+1)*(yb-ya+1));
            tot[0] = (tot[0]*numsam)>>16;
            tot[1] = (tot[1]*numsam)>>16;
            tot[2] = (tot[2]*numsam)>>16;
            #endif

            const int doff = 3*(dxres*j+i);
            dptr[ doff+0 ] = tot[0];
            dptr[ doff+1 ] = tot[1];
            dptr[ doff+2 ] = tot[2];
        }
    }
    // case 2 (expansion)
    else if( (dxres>sxres) && (dyres>syres) )
    {
    }
    // case 3
    else if( (dxres<sxres) && (dyres>syres) )
    {
    }
    // csae 4
    else if( (dxres>sxres) && (dyres<syres) )
    {
    }


}


int piImage_Resize( piImage *img, int xres, int yres )
{
	if( img->GetFormat() != piImage::FORMAT_I_RGBA && 
        img->GetFormat() != piImage::FORMAT_I_RGB) return 0;

	piImage tmp;
    
    if (!tmp.Make(piImage::TYPE_2D, xres, yres, 1, img->GetFormat()))
		return 0;

    if (img->GetFormat() == piImage::FORMAT_I_RGBA)
	{
        resize_irgba(&tmp,img);
    }
    else if (img->GetFormat() == piImage::FORMAT_I_RGB)
	{
        resize_irgb(&tmp,img);
    }
    else
    {
        return 0;
    }

    img->Replace( &tmp );

    return 1;
}

}