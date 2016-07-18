#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "../../libSystem/piFile.h"
#include "../piImage.h"
#include "../processing/piImageFlip.h"

namespace piLibs {

#pragma pack(1)
typedef struct
{
    unsigned char   IDFieldLength;
    unsigned char   ColorMapType;
    unsigned char   ImageType;
    unsigned short  ColorMapOrigin;
    unsigned short  ColorMapLength;
    unsigned char   ColorMapEntrySize;
    unsigned short  ImageXOrigin;
    unsigned short  ImageYOrigin;
    
    unsigned short  xres;
    unsigned short  yres;
    unsigned char   bpp;
    unsigned char   ImageDescriptor;
}TGAHEADER;
#pragma pack()


static int U_Load( piImage *bmp, FILE * fp )
{
    const int bpp = bmp->GetBpp();
	const int l = bmp->GetXRes()*bmp->GetYRes()*bpp;

    if( !fread( bmp->GetData(), 1, l, fp) )
        return( 0 );

    return( 1 );
}

static int C_Load( piImage * bmp, FILE * fp )
{
    unsigned char   aux[4];
    unsigned char   chunkheader;
    int             i, counter;
    
    const int bpp = bmp->GetBpp();

    int pixelcount = bmp->GetXRes()*bmp->GetYRes();
    int currentpixel = 0;
    unsigned char *buffer = (unsigned char*)bmp->GetData();


    while( currentpixel<pixelcount )
        {
        if( !fread( &chunkheader, 1, 1, fp) )
            return( 0 );

        counter = 1 + (chunkheader & 0x7F);

        if( chunkheader<128 )
            {             
            for( i=0; i<counter; i++ )
                {
                if( !fread(buffer, 1, bpp, fp) )
                    return( 0 );
                buffer += bpp;
                }
            }
        else
            {
            if( !fread( aux, 1, bpp, fp) )
                return( 0 );                          

            for( i=0; i<counter; i++ )
                {                                     
                buffer[0] = aux[0];
                if( bpp>1 ) buffer[1] = aux[1];
                if( bpp>2 ) buffer[2] = aux[2];
                if( bpp>3 ) buffer[3] = aux[3];
                buffer += bpp;
                }
            }

        currentpixel+=counter;
        }

    return( 1 );
}







//////////////////////////////////


int TGA_Load( piImage *bmp, const wchar_t *name )
{
    FILE            *fp;
    long            res = 0;
    int             i;
    piImage::Format   format;
    unsigned char   byte;
	TGAHEADER       tga;

    fp = _wfopen( name, L"rb" );
    if( !fp )
        return( 0 );

    if( !fread( &tga, 18, 1, fp) )
        {
        fclose( fp );
        return( 0 );
        }

    #ifdef BIGENDIAN
    tga.xres = ((tga.xres&255)<<8) | (tga.xres>>8);
    tga.yres = ((tga.yres&255)<<8) | (tga.yres>>8);
    #endif

    if( tga.xres<2 || tga.yres<2 )
		{
		fclose( fp );
        return( 0 );
		}

    if( (tga.bpp!=24) && (tga.bpp!=32) && (tga.bpp!=16) && (tga.bpp!=8))
		{
		fclose( fp );
        return( 0 );
		}

    if( tga.IDFieldLength )
        for( i=0; i<tga.IDFieldLength; i++ )
            fread( &byte, 1, 1, fp);


    switch( tga.ImageType )
        {
		// uncommpressed rgb
        case 2:

			if( tga.bpp==24 )
				format = piImage::FORMAT_I_RGB;
			else if( tga.bpp==32 )
				format = piImage::FORMAT_I_RGBA;
			else
				break;
			
            if (!bmp->Make(piImage::TYPE_2D, tga.xres, tga.yres, 1, format))
				break;
            if( !U_Load(bmp,fp) )
				break;
			if( !piImage_Flip(bmp) )
				break;
            res = 1;
            break;

		// compressed rgb
        case 10:                    
            if( tga.bpp==24 )
				format = piImage::FORMAT_I_RGB;
			else if( tga.bpp==32 )
				format = piImage::FORMAT_I_RGBA;
			else
				break;

            if (!bmp->Make(piImage::TYPE_2D, tga.xres, tga.yres, 1, format))
				break;
            if( !C_Load(bmp,fp) )
				break;
			if( !piImage_Flip(bmp) )
				break;
            res = 1;
            break;

		// uncompressed grey
        case 3:                                 
            if (!bmp->Make(piImage::TYPE_2D, tga.xres, tga.yres, 1, piImage::FORMAT_I_GREY))
				break;
            if( !U_Load(bmp,fp) )
				break;
			if( !piImage_Flip(bmp) )
				break;
            res = 1;
            break;

		// compressed grey
        case 11:                                 
            if (!bmp->Make(piImage::TYPE_2D, tga.xres, tga.yres, 1, piImage::FORMAT_I_GREY))
				break;
            if( !C_Load(bmp,fp) )
				break;
			if( !piImage_Flip(bmp) )
				break;
            res = 1;
            break;

        case 1:                                 // uncompressed pallete
            break;

        case 9:                                 // compressed pallete
            break;
        }

	if( !res )
        bmp->Free();

    fclose( fp );

    return( res );
}

//-----------------------------------------------------------------------------

static unsigned char head32[18] = {
     0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x40, 0x01,  //x
     0xf0, 0x00,  //y
     0x20, 0x00
};

static unsigned char head24[18] = {
     0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x40, 0x01,
     0xf0, 0x00,
     0x18, 0x00
};

bool TGA_Salva24( const wchar_t *nombre, long *buffer, int xres, int yres )
{
    unsigned char   *ptr = (unsigned char*)buffer;

    if( xres<=0 || yres<=0 )
        return( 0 );

    head24[12] = (xres&255);
    head24[13] = (xres>>8)&255;
    head24[14] = (yres&255);
    head24[15] = (yres>>8)&255;

    FILE *fo = _wfopen( nombre, L"wb" );
    if( !fo )
        return false;

    fwrite( head24, 1, 18, fo );

    for( int i=0; i<yres; i++ )
    {
        fseek( fo, 18 + 3*xres*(yres-i-1), SEEK_SET );
        fwrite( ptr, xres, 3, fo );
        ptr += xres*3;
    }

    fclose( fo );

    return true;
}

bool TGA_Salva8( const wchar_t *nombre, unsigned char *buffer, int xres, int yres )
{
    if( xres<=0 || yres<=0 )
        return( 0 );

    unsigned char head8[18] = {
        0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0x01,
        0xf0, 0x00,
        0x08, 0x08
    };


    head8[12] = (xres&255);
    head8[13] = (xres>>8)&255;
    head8[14] = (yres&255);
    head8[15] = (yres>>8)&255;

    piFile fo;
    if( !fo.Open(nombre, L"wb") )
        return( 0 );

    fo.Write( head8, 18 );

    for( int i=0; i<yres; i++ )
    {
        fo.Seek( 18 + xres*(yres-i-1), piFile::SET );
        fo.Write( buffer, xres );
        buffer += xres;
    }

    fo.Close();

    return( 1 );
}
bool TGA_Salva32( const wchar_t *nombre, int *buffer, int xres, int yres )
{
    if( xres<=0 || yres<=0 )
        return false;

    head32[12] = (xres&255);
    head32[13] = (xres>>8)&255;
    head32[14] = (yres&255);
    head32[15] = (yres>>8)&255;

    FILE *fo = _wfopen( nombre, L"wb" );
    if( !fo )
        return false;
    fwrite( head32, 1, 18, fo );

    for( int i=0; i<yres; i++ )
    {
        fseek( fo, 18 + 4*xres*(yres-i-1), SEEK_SET );
        fwrite( buffer, xres, 4, fo );
        buffer += xres;
    }

    fclose( fo );

    return true;
}

bool TGA_Salva32to24(const  char *nombre, int *buffer, int xres, int yres )
{
    unsigned char   *ubuffer = (unsigned char *)buffer;

    if( xres<=0 || yres<=0 )
        return false;

    head24[12] = (xres&255);
    head24[13] = (xres>>8)&255;
    head24[14] = (yres&255);
    head24[15] = (yres>>8)&255;

    FILE *fo = fopen( nombre, "wb" );
    if( !fo )
        return false;

    if( fwrite( head24, 1, 18, fo ) !=18 )
    {
        fclose( fo );
        return false;
    }

    ubuffer = ubuffer+(xres*yres-xres)*4;
    for( int i=0; i<yres; i++ )
    {
        unsigned char *ptr = ubuffer;
        for( int j=0; j<xres; j++ )
        {
            if( fwrite( ptr, 1, 3, fo )!=3 )
            {
                fclose( fo );
                return( 0 );
                }
            ptr += 4;
        }
        ubuffer -= xres<<2;
    }

    fclose( fo );

    return true;
}

bool TGA_Save(const piImage *bmp, const wchar_t *name)
{
    const int xres = bmp->GetXRes();
    const int yres = bmp->GetYRes();
    const void *data = bmp->GetData();

    if (bmp->GetFormat() == piImage::FORMAT_I_RGB)
        return TGA_Salva24( name, (long*)data, xres, yres );
/*
    if( bmp->format==IMAGE_FORMAT_I_RGBA )
        return TGA_Salva32To24( name, (long*)bmp->data, bmp->xres, bmp->yres ) )
*/
    if (bmp->GetFormat() == piImage::FORMAT_I_RGBA)
        return TGA_Salva32( name, (int*)data, xres, yres );
       
    if (bmp->GetFormat() == piImage::FORMAT_I_GREY)
        return TGA_Salva8( name, (unsigned char*)data, xres, yres );

	return false;
}

}