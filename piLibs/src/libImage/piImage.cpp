#include <string.h>
#include <malloc.h>
#include <math.h>

#include "piImage.h"
#include "../libSystem/piStr.h"

#include "formats/f_bmp.h"
#include "formats/f_gif.h"
#include "formats/f_pcx.h"
#include "formats/f_tga.h"
#include "formats/f_jpg.h"
#include "formats/f_png.h"

namespace piLibs {

typedef int (*LOADFUN)( piImage *img, const wchar_t *name );
typedef bool (*SAVEFUN)( const piImage *img, const wchar_t *name);

#define NUmFormatS	5

typedef struct
{
	LOADFUN	load;
	SAVEFUN	save;
	wchar_t	ext[4];
}FILEIO;

static FILEIO fileio[NUmFormatS] = 
{
	{TGA_Load, TGA_Save, L"tga" },
	{GIF_Load, GIF_Save, L"gif" },
	{BMP_Load, BMP_Save, L"bmp" },
	{JPG_Load, 0,        L"jpg" },
    {0 ,       PNG_Save, L"png" }

};

//--------------------------------

static int getType( int xres, int yres, int zres )
{
	if( zres>1 ) return( piImage::TYPE_3D );
	if( yres>1 ) return( piImage::TYPE_2D );
	return( piImage::TYPE_1D );
}

static __inline int rgb2grey( int r, int g, int b )
{
    return (r*306 + g*601 + 117*b) >> 10;
}



//--------------------------------


// return the extension
static const wchar_t *getextension( const wchar_t *name )
{
	const size_t l = piwstrlen( name );

	for( size_t i=l-1; i>0; i-- )
	{
		if( name[i]=='.' )
		{	
			return( name+i+1 );
		}
	}
	return( 0 );
}

static int compare( const wchar_t *str1, const wchar_t *str2 )
{
	int l = piwstrlen( str1 );
	if( piwstrlen( str2 ) != l )
		return( 0 );

    for (int i = 0; i<l; i++)
		{
		int c1 = str1[i];
        int c2 = str2[i];
		
		if( c1>='a' && c1<='z' ) c1 -= 'a'-'A';
		if( c2>='a' && c2<='z' ) c2 -= 'a'-'A';

		if( c1!=c2 )
			return( 0 );
		}

	return( 1 );
}

//-------------------------------------------------------------------------
void piImage::Init( void )
{
    memset( this, 0, sizeof(piImage) );
}

bool piImage::Load(const wchar_t *name)
{
	int	i;
	const wchar_t *ext = getextension( name );
	if( !ext )
        return false;

	for( i=0; i<NUmFormatS; i++ )
        {
        if( compare( ext, fileio[i].ext) )
            {
		    if( fileio[i].load( this, name ) )
			    return true;
            }
        }

	return false;
}


static int iGetBpp(piImage::Format type)
{
    switch (type)
    {
    case piImage::FORMAT_I_INDEXED: return(1);
    case piImage::FORMAT_I_GREY:    return(1);
    case piImage::FORMAT_I_15BIT:   return(2);
    case piImage::FORMAT_I_16BIT:   return(2);
    case piImage::FORMAT_I_RG:      return(2);
    case piImage::FORMAT_I_RGB:     return(3);
    case piImage::FORMAT_I_RGBA:    return(4);
    case piImage::FORMAT_F_GREY:    return(4);
    case piImage::FORMAT_F_RGB:     return(12);
    case piImage::FORMAT_F_RGBA:    return(16);
    }

    return(0);
}


int piImage::GetBpp(void) const
{
    return iGetBpp( mFormat );
}

bool piImage::Save(const wchar_t *name) const
{
	int		i;

	const wchar_t *ext = getextension( name );
	if( !ext )
		return false;


	for( i=0; i<NUmFormatS; i++ )
    {
		if( compare( ext, fileio[i].ext) )
        {
			if( fileio[i].save( this, name ) )
				return true;
        }
    }

	return false;
}

void piImage::Replace( piImage *img )
{
    Free();
    mXres = img->mXres;
    mYres = img->mYres;
    mZres = img->mZres;
    mFormat = img->mFormat;
    mType = img->mType;
    mData = img->mData;
    mPallette = img->mPallette;
}

bool piImage::Make( Type type, int xres, int yres, int zres, Format format )
{
    int	bpp = iGetBpp(format);
	if( !bpp )
		return false;
    
    const int layers = (type == piImage::TYPE_CUBE) ? 6 : 1;

	mData = malloc( xres*yres*zres*bpp*layers );
	if( !mData )
		return( 0 );

	mType = type;
	mXres = xres;
	mYres = yres;
	mZres = zres;
	mFormat = format;
	mPallette = nullptr;

	if( format==piImage::FORMAT_I_INDEXED )
	{
		mPallette = (unsigned char*)malloc( 3*256 );
		if( !mPallette )
		{
			free( mData );
			return false;
		}
	}
	
	return true;
}

void piImage::Free( void )
{
	if( mData )     free( mData );
	if( mPallette ) free( mPallette );
}


void piImage::Create(Type type, int xres, int yres, int zres, Format format, void *buffer)
{
	mType = type;
    mData = buffer;
    mPallette = nullptr;
    mXres = xres;
    mYres = yres;
	mZres = zres;
    mFormat = format;
    //mbpp = getBpp( format );
}

const piImage::Format piImage::GetFormat(void) const
{
    return mFormat;
}

const piImage::Type piImage::GetType(void) const
{
    return mType;
}

const int piImage::GetXRes(void) const
{
    return mXres;
}
const int piImage::GetYRes(void) const
{
    return mYres;
}
const int piImage::GetZRes(void) const
{
    return mZres;
}

void * piImage::GetData(void)
{
    return mData;
}

void * piImage::GetData(void) const
{
    return mData;
}

unsigned char * piImage::GetPallette(void)
{
    return mPallette;
}

unsigned char * piImage::GetPallette(void) const
{
    return mPallette;
}

//----------------------------------------------------------------------------

bool piImage::Convert(Format format, bool swapRB)
{
    if( format==mFormat ) return true;

	if( format==piImage::FORMAT_I_RGBA && mFormat==piImage::FORMAT_I_RGB )
    {
        const int l = mXres*mYres*mZres;

        unsigned char *aux = (unsigned char*)malloc(l * 4);
        if (!aux)
            return false;

        const unsigned char *ori = (unsigned char*)mData;
        unsigned char *dst = aux;
        for (int i = 0; i<l; i++)
        {
            dst[0] = ori[0];
            dst[1] = ori[1];
            dst[2] = ori[2];
            dst[3] = 255;
            dst += 4;
            ori += 3;
        }

        free(mData);

        mData = (void*)aux;
        mFormat = piImage::FORMAT_I_RGBA;
        return true;
    }
	else if( format==piImage::FORMAT_I_RGB && mFormat==piImage::FORMAT_F_RGB )
    {
        int l = 3*mXres*mYres*mZres;

        unsigned char *aux = (unsigned char*)malloc(l);
        if (!aux)
            return false;

        float *ori = (float*)mData;
        float min = ori[0];
        float max = ori[0];
        for (int i = 1; i<l; i++)
        {
            float f = ori[i];
            if (f<min) min = f; else if (f>max) max = f;
        }

        float f = max - min;
        if (f <= 0.0f) { free(aux); return(0); }

        f = 255.0f / f;
        for( int i = 0; i<l; i++ ) aux[i] = (unsigned char)((ori[i] - min)*f);

        free( mData );

        mData = (void*)aux;
        mFormat = piImage::FORMAT_I_RGB;
        return true;
    }
	else if( format==piImage::FORMAT_I_RGBA && mFormat==piImage::FORMAT_I_INDEXED )
    {
        int l = mXres*mYres*mZres;

        unsigned char *aux = (unsigned char*)malloc(l * 4);
        if (!aux)
            return(0);

        unsigned char *ori = (unsigned char*)mData;
        unsigned char *pal = mPallette;
        for( int i = 0; i<l; i++ )
        {
            const int c = ori[i];
            aux[4 * i + 0] = pal[3 * c + 0];
            aux[4 * i + 1] = pal[3 * c + 1];
            aux[4 * i + 2] = pal[3 * c + 2];
            aux[4 * i + 3] = 255;
        }

        free(mData);
        mData = (void*)aux;
        mFormat = piImage::FORMAT_I_RGBA;
        return true;
    }
    else if( format==piImage::FORMAT_I_GREY && mFormat==piImage::FORMAT_I_INDEXED )
        {
        unsigned char *ptr = (unsigned char*)mData;
        const unsigned int num = mXres*mYres*mZres;
        for (unsigned int i = 0; i<num; i++)
        {
            const int c = ptr[i];
            int r = mPallette[3*c+0];
            int g = mPallette[3*c+1];
            int b = mPallette[3*c+2];
            ptr[i] = (r + g + b) / 3;
        }

        mFormat = piImage::FORMAT_I_GREY;
        return true;
    }
	else if( format==piImage::FORMAT_I_GREY && mFormat==piImage::FORMAT_I_RGB )
    {
        const int l = mXres*mYres*mZres;

        unsigned char *aux = (unsigned char*)malloc(l);
        if (!aux)
            return false;

        unsigned char *ori = (unsigned char*)mData;

        for( int i = 0; i<l; i++)
        {
            int b = ori[3 * i + 0];
            int g = ori[3 * i + 1];
            int r = ori[3 * i + 2];
            aux[i] = rgb2grey(r, g, b);
        }

        free(mData);
        mData = (void*)aux;
        mFormat = piImage::FORMAT_I_GREY;
        return true;
    }
	if( format==piImage::FORMAT_I_RGB && mFormat==piImage::FORMAT_I_RGBA )
    {
        const int l = mXres*mYres*mZres;
        unsigned char *aux = (unsigned char*)malloc(l * 3);
        if (!aux)
            return false;

        unsigned char *ori = (unsigned char*)mData;
        unsigned char *dst = aux;

        if(swapRB )
        {
            for (int i = 0; i<l; i++)
            {
                dst[2] = ori[0];
                dst[1] = ori[1];
                dst[0] = ori[2];
                dst += 3;
                ori += 4;
            }
        }
        else
        {
            for (int i = 0; i<l; i++)
            {
                dst[0] = ori[0];
                dst[1] = ori[1];
                dst[2] = ori[2];
                dst += 3;
                ori += 4;
            }
        }

        free( mData );
        mData = (void*)aux;
        mFormat = piImage::FORMAT_I_RGB;
        return true;
    }

    return false;
}



void piImage::SwapRB(void)
{
	if( mFormat==piImage::FORMAT_I_RGBA  )
    {
		unsigned char *ptr = (unsigned char*)mData;
		const unsigned int num = mXres*mYres*mZres;
		for( unsigned int i=0; i<num; i++ )
		{
			int j = ptr[0];
			ptr[0] = ptr[2];
			ptr[2] = j;
			ptr += 4;
		}
	}
	else if( mFormat==piImage::FORMAT_I_RGB  )
    {
        unsigned char *ptr = (unsigned char*)mData;
        const unsigned int num = mXres*mYres*mZres;
        for (unsigned int i = 0; i<num; i++)
		{
			int j = ptr[0];
			ptr[0] = ptr[2];
			ptr[2] = j;
			ptr += 3;
		}
	}
}

int piImage::Index2Color( unsigned char idx )
{
    int r = mPallette[3*idx+0];
    int g = mPallette[3*idx+1];
    int b = mPallette[3*idx+2];

    return 0xff000000 | (r<<16) | (g<<8) | b;
}

bool piImage::ExtractComponent(piImage *img, int compo)
{
    if( mFormat==piImage::FORMAT_I_INDEXED )
    {
        if (!img->Make(mType, mXres, mYres, mZres, piImage::FORMAT_I_GREY))
            return false;

        const unsigned char *src = (unsigned char*)mData;
        unsigned char *dst = (unsigned char*)img->mData;
        const int l = mXres*mYres*mZres;
        for (int i = 0; i<l; i++)
        {
            const int c = src[i];
            dst[i] = mPallette[3*c + (2-compo)];
        }
        return true;
    }

    if( mFormat==piImage::FORMAT_I_RGB )
    {
        if (!img->Make(mType, mXres, mYres, mZres, piImage::FORMAT_I_GREY))
            return false;

        const unsigned char *src = (unsigned char*)mData;
        unsigned char *dst = (unsigned char*)img->mData;
        const int l = mXres*mYres*mZres;
        for (int i = 0; i<l; i++)
        {
            dst[i] = src[3*i+compo];
        }
        return true;
    }

    return false;
}

int piImage::RGB2YUV( void )
{
    const int xres = mXres;
    const int yres = mYres;
    const int zres = mZres;
    const int len = xres*yres*zres;

    if( mFormat==FORMAT_I_RGB )
        {

        unsigned char *src = (unsigned char*)mData;
        for( int i=0; i<len; i++ )
            {
            int b = src[3*i+0];
            int g = src[3*i+1];
            int r = src[3*i+2];

            src[3*i+0] = (r  + (g<<1) + b ) >>2;
            src[3*i+1] = (b - g)/2;
            src[3*i+2] = (r - g)/2;
            }
        return 1;

        }


    return( 0 );
}


bool piImage::Compatibles( const piImage *b )
{
    if( mXres   !=b->mXres    ) return false;
    if( mYres   != b->mYres   ) return false;
    if( mZres   != b->mZres   ) return false;
    if( mFormat != b->mFormat ) return false;
    if( mType   != b->mType   ) return false;

    return true;
}

int piImage::Clone( const piImage *b )
{
    if( !Compatibles( b ) )
        return false;

    const int bpp = iGetBpp(mFormat);
    
    const int amount = mXres*mYres*mZres*bpp;
    memcpy( mData, b->mData, amount );
    return( 1 );
}
    


void piImage::SampleBilinear(float x, float y, float *data) const
{
	const int xres = mXres;
	const int yres = mYres;

    const float fx = x*(float)xres;
    const float fy = y*(float)yres;

	const float ifx = floorf( fx );
	const float ify = floorf( fy );

	const float ffx = fx - ifx;
	const float ffy = fy - ify;

	const int tuA = ((int)ifx) % xres;
	const int tvA = ((int)ify) % yres;

	const int tuB = (1+tuA) % xres;
	const int tvB = (1+tvA) % yres;

	float a[4];
	float b[4];
	float c[4];
	float d[4];

	if( mFormat == piImage::FORMAT_F_RGBA )
	{
		const float *ptr = (float*)mData;
		a[0] = ptr[ 4*(tvA*xres+tuA) + 0 ];
        a[1] = ptr[ 4*(tvA*xres+tuA) + 1 ];
        a[2] = ptr[ 4*(tvA*xres+tuA) + 2 ];
        a[3] = ptr[ 4*(tvA*xres+tuA) + 3 ];
		b[0] = ptr[ 4*(tvA*xres+tuB) + 0 ];
        b[1] = ptr[ 4*(tvA*xres+tuB) + 1 ];
        b[2] = ptr[ 4*(tvA*xres+tuB) + 2 ];
        b[3] = ptr[ 4*(tvA*xres+tuB) + 3 ];
		c[0] = ptr[ 4*(tvB*xres+tuA) + 0 ];
        c[1] = ptr[ 4*(tvB*xres+tuA) + 1 ];
        c[2] = ptr[ 4*(tvB*xres+tuA) + 2 ];
        c[3] = ptr[ 4*(tvB*xres+tuA) + 3 ];
		d[0] = ptr[ 4*(tvB*xres+tuB) + 0 ];
        d[1] = ptr[ 4*(tvB*xres+tuB) + 1 ];
        d[2] = ptr[ 4*(tvB*xres+tuB) + 2 ];
        d[3] = ptr[ 4*(tvB*xres+tuB) + 3 ];

        float ab[4] = { a[0] + (b[0] - a[0])*ffx,
            a[1] + (b[1] - a[1])*ffx,
            a[2] + (b[2] - a[2])*ffx,
            a[3] + (b[3] - a[3])*ffx };
        float cd[4] = { c[0] + (d[0] - c[0])*ffx,
            c[1] + (d[1] - c[1])*ffx,
            c[2] + (d[2] - c[2])*ffx,
            c[3] + (d[3] - c[3])*ffx };

        data[0] = ab[0] + (cd[0] - ab[0])*ffy;
        data[1] = ab[1] + (cd[1] - ab[1])*ffy;
        data[2] = ab[2] + (cd[2] - ab[2])*ffy;
        data[3] = ab[3] + (cd[3] - ab[3])*ffy;

	}
	else if( mFormat == piImage::FORMAT_I_RGBA )
	{
		const unsigned char *ptr = (unsigned char*)mData;
		a[0] = ptr[ 4*(tvA*xres+tuA) + 0 ] * (1.0f/255.0f);
        a[1] = ptr[ 4*(tvA*xres+tuA) + 1 ] * (1.0f/255.0f);
        a[2] = ptr[ 4*(tvA*xres+tuA) + 2 ] * (1.0f/255.0f);
        a[3] = ptr[ 4*(tvA*xres+tuA) + 3 ] * (1.0f/255.0f);
		b[0] = ptr[ 4*(tvA*xres+tuB) + 0 ] * (1.0f/255.0f);
        b[1] = ptr[ 4*(tvA*xres+tuB) + 1 ] * (1.0f/255.0f);
        b[2] = ptr[ 4*(tvA*xres+tuB) + 2 ] * (1.0f/255.0f);
        b[3] = ptr[ 4*(tvA*xres+tuB) + 3 ] * (1.0f/255.0f);
		c[0] = ptr[ 4*(tvB*xres+tuA) + 0 ] * (1.0f/255.0f);
        c[1] = ptr[ 4*(tvB*xres+tuA) + 1 ] * (1.0f/255.0f);
        c[2] = ptr[ 4*(tvB*xres+tuA) + 2 ] * (1.0f/255.0f);
        c[3] = ptr[ 4*(tvB*xres+tuA) + 3 ] * (1.0f/255.0f);
		d[0] = ptr[ 4*(tvB*xres+tuB) + 0 ] * (1.0f/255.0f);
        d[1] = ptr[ 4*(tvB*xres+tuB) + 1 ] * (1.0f/255.0f);
        d[2] = ptr[ 4*(tvB*xres+tuB) + 2 ] * (1.0f/255.0f);
        d[3] = ptr[ 4*(tvB*xres+tuB) + 3 ] * (1.0f/255.0f);

        float ab[4] = { a[0] + (b[0] - a[0])*ffx,
            a[1] + (b[1] - a[1])*ffx,
            a[2] + (b[2] - a[2])*ffx,
            a[3] + (b[3] - a[3])*ffx };
        float cd[4] = { c[0] + (d[0] - c[0])*ffx,
            c[1] + (d[1] - c[1])*ffx,
            c[2] + (d[2] - c[2])*ffx,
            c[3] + (d[3] - c[3])*ffx };

        data[0] = ab[0] + (cd[0] - ab[0])*ffy;
        data[1] = ab[1] + (cd[1] - ab[1])*ffy;
        data[2] = ab[2] + (cd[2] - ab[2])*ffy;
        data[3] = ab[3] + (cd[3] - ab[3])*ffy;

	}
    else if (mFormat == piImage::FORMAT_I_RGB)
    {
        const unsigned char *ptr = (unsigned char*)mData;
        a[0] = ptr[3 * (tvA*xres + tuA) + 0] * (1.0f / 255.0f);
        a[1] = ptr[3 * (tvA*xres + tuA) + 1] * (1.0f / 255.0f);
        a[2] = ptr[3 * (tvA*xres + tuA) + 2] * (1.0f / 255.0f);
        b[0] = ptr[3 * (tvA*xres + tuB) + 0] * (1.0f / 255.0f);
        b[1] = ptr[3 * (tvA*xres + tuB) + 1] * (1.0f / 255.0f);
        b[2] = ptr[3 * (tvA*xres + tuB) + 2] * (1.0f / 255.0f);
        c[0] = ptr[3 * (tvB*xres + tuA) + 0] * (1.0f / 255.0f);
        c[1] = ptr[3 * (tvB*xres + tuA) + 1] * (1.0f / 255.0f);
        c[2] = ptr[3 * (tvB*xres + tuA) + 2] * (1.0f / 255.0f);
        d[0] = ptr[3 * (tvB*xres + tuB) + 0] * (1.0f / 255.0f);
        d[1] = ptr[3 * (tvB*xres + tuB) + 1] * (1.0f / 255.0f);
        d[2] = ptr[3 * (tvB*xres + tuB) + 2] * (1.0f / 255.0f);

        float ab[3] = { a[0] + (b[0] - a[0])*ffx,
                        a[1] + (b[1] - a[1])*ffx,
                        a[2] + (b[2] - a[2])*ffx };
        float cd[3] = { c[0] + (d[0] - c[0])*ffx,
                        c[1] + (d[1] - c[1])*ffx,
                        c[2] + (d[2] - c[2])*ffx };

        data[0] = ab[0] + (cd[0] - ab[0])*ffy;
        data[1] = ab[1] + (cd[1] - ab[1])*ffy;
        data[2] = ab[2] + (cd[2] - ab[2])*ffy;
    }
    else
    {
        data[0] = 0.0f;
        data[1] = 0.0f;
        data[2] = 0.0f;
        data[3] = 0.0f;
    }

}

}