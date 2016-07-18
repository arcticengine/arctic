#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <cmath>
#include "../../libSystem/piStr.h"
#include "../../libImage/piImage.h"
#include "../../libImage/processing/piImageFlip.h"
#include "../../libRender/piRenderer.h"
#include "../../libMath/piShading.h"
#include "piRenderFont2D.h"


namespace piLibs {

static bool ColIsBlack( const unsigned char *buffer, const int xres, const int yres, const int ix, const int iy, const int w, const int h, const int i )
{
	for( int j=0; j<h; j++ )
		if( buffer[xres*((iy+j))+(ix+i)] & 0xff )
			return false;
	return true;
}

static void FillCharInfo( piImage *image, int ix, int iy, int *rmi, int *rma, const int fontType )
{
	const unsigned char *buffer = (unsigned char*)image->GetData();
	const int xres = image->GetXRes();
	const int yres = image->GetYRes();
	const int w = xres / 16;
	const int h = w;//yres / 8;

    if( fontType==IQRFONT_COURIER )
    {
		*rmi = w/4;
        *rma = w-w/4;
    }
    else
    {
        *rmi = 0;
	    for( int i=0; i<w; i++ )
	    {
		    if( !ColIsBlack(buffer,xres,yres,ix,iy,w,h,i) )
		    {
			    *rmi = i;
			    break;
		    }
	    }

	    *rma = 0;
	    for( int i=0; i<w; i++ )
	    {
		    int ii = w-1-i;
		    if( !ColIsBlack(buffer,xres,yres,ix,iy,w,h,ii) )
		    {
			    *rma = ii;
			    break;
		    }
	    }
    }
}

void piRenderFont2D::FillInfo(  const int fontType )
{
	const int w = mImage.GetXRes() / 16;
	const int h = w;//me->mImage[0].yres / 8;

	for( int j=0; j<8; j++ )
	for( int i=0; i<16; i++ )
	{
		int xmi = 0;
		int xma = 0;
		FillCharInfo( &mImage, i*w, j*h, &xmi, &xma, fontType );
		mCharInfo[16*j+i].xmin = w*i + xmi-1;
		mCharInfo[16*j+i].xmax = w*i + xma+1;
	}

}

static const char vs[] = \
"layout (location=1) uniform vec4 uXYUV[2];"

"layout (location=0) in vec2 inVertex;"
"layout (location=1) in vec2 inUV;"


"out vec2 vfUV;"

"void main(void)"
"{"
    "vec4 xyuv = uXYUV[0] + (uXYUV[1]-uXYUV[0])*(0.5+0.5*inVertex.xyxy);"
	"gl_Position = vec4(xyuv.xy,0.0,1.0);"
	"vfUV = xyuv.zw;"
"}";

static const char fs[] = \
"layout (location=0) uniform vec4 uColor;"
"layout (binding=0) uniform sampler2D t0;"

"in vec2 vfUV;"
"out vec4 outColor;"

"void main(void)"
"{"
	"float f = texture(t0,vfUV).x;"
    "outColor = vec4( uColor.xyz, f*uColor.w );"
"}";


static const char vs3d[] = \
    "layout (location=0) in vec2 inVertex;"
    "layout (location=1) uniform vec4 uXYUV[2];"
    "layout (location=3) uniform float z;"
    "layout (location=4) uniform mat4x4 mMatrix_CamPrj;"

    "out vec2 vfUV;"

    "void main(void)"
    "{"
        "vec4 xyuv = uXYUV[0] + (uXYUV[1]-uXYUV[0]) * (0.5+0.5*inVertex.xyxy);"
        "gl_Position = mMatrix_CamPrj * vec4(xyuv.xy, z, 1.0);"
        "vfUV = xyuv.zw;"
    "}";

static const char fs3d[] = \
    "layout (location=0) uniform vec4 uColor;"
    "layout (binding=0) uniform sampler2D t0;"

    "in vec2 vfUV;"
    "out vec4 outColor;"

    "void main(void)"
    "{"
        "float f = texture(t0,vfUV).x;"
        "outColor = vec4( uColor.xyz, f*uColor.w );"
    "}";

piRenderFont2D::piRenderFont2D()
{
}

piRenderFont2D::~piRenderFont2D()
{
}

bool piRenderFont2D::Init( piRenderer *renderer, const int fontType, const wchar_t *path)
{
    mFontType = fontType;
    if( fontType==IQRFONT_TAHOMA )
    {
        wchar_t fname[256];        
        piwsprintf( fname, 256, L"%s/fontTahoma.tga", path );
        if (!mImage.Load(fname)) return false;
    }
    else if( fontType==IQRFONT_COURIER )
    {
        wchar_t fname[256];        
        piwsprintf( fname, 256, L"%s/fontCourier2.tga", path );
        if (!mImage.Load(fname)) return false;
    }
    else
    {
        return false;
    }

    //piImage_Flip(&mImage);

    const piTextureInfo tinfo0 = { piTEXTURE_2D, piFORMAT_C1I8, false,  mImage.GetXRes(), mImage.GetYRes(), mImage.GetZRes(), 1 };
    mTexture = renderer->CreateTexture( 0, &tinfo0, piFILTER_MIPMAP, piWRAP_CLAMP, 1.0f, mImage.GetData() );
    if( !mTexture )
        return false;

    FillInfo(fontType);

    char error[2048];
    mShader = renderer->CreateShader(nullptr, vs, 0, 0, 0, fs, error);
    if( mShader==nullptr )
        return false;

    mShader3d = renderer->CreateShader(nullptr, vs3d, 0, 0, 0, fs3d, error);
    if( mShader3d== nullptr)
        return false;

    return true;
}

void piRenderFont2D::Deinit( piRenderer *renderer )
{
    renderer->DestroyShader( mShader );
    renderer->DestroyShader( mShader3d );

    mImage.Free();
	renderer->DestroyTexture( mTexture );
}

static void drawCursorF(  piRenderer *renderer, float x, float y, double cursorTime, float ita, float size, float cx, float cy )
{
    float an = 0.5f + 0.5f*(float)std::sin(6.2831*cursorTime);

    float tx0 = (       24.0f)/2048.0f;
    float tx1 = (128.0f-24.0f)/2048.0f;

    float ty0 = (       24.0f)/1024.0f;
    float ty1 = (       32.0f)/1024.0f;
    float ty2 = (128.0f-32.0f)/1024.0f;
    float ty3 = (128.0f-24.0f)/1024.0f;

    const float x1 = x - cx*0.15f;
    const float x2 = x + cx*0.15f;

    const float h = 0.3f + 0.2f*an;

    {
    const float xyuv[8] = { x1, y + cy*(0.5f-h-0.1f), tx0, ty0, 
                            x2, y + cy*(0.5f-h), tx1, ty1 };
    renderer->SetShaderConstant4F( 1, xyuv, 2 );
    renderer->DrawUnitQuad_XY(1);
    }

    {
    const float xyuv[8] = { x1, y + cy*(0.5f-h), tx0, ty1, 
                            x2, y + cy*(0.5f+h), tx1, ty2 };
    renderer->SetShaderConstant4F( 1, xyuv, 2 );
    renderer->DrawUnitQuad_XY(1);
    }

    {
    const float xyuv[8] = { x1, y + cy*(0.5f+h), tx0, ty2, 
                            x2, y + cy*(0.6f+h), tx1, ty3 };
    renderer->SetShaderConstant4F( 1, xyuv, 2 );
    renderer->DrawUnitQuad_XY(1);
    }
}


void piRenderFont2D::Prints( piRenderer *renderer, float x, float y, float sizex, float sizey, bool doItalic,
						  bool drawCursor, int cursorPos, double cursorTime,
						  const wchar_t *str, const float *color, const drawCharacterCB * dcCB, float *finalXY )
{
    const int xres = mImage.GetXRes();
	const float csale = (mFontType==IQRFONT_COURIER) ? 0.0f : 16.0f/(float)xres;

	const float ixres = 1.0f /(float)xres;

	float ita = 0.0f;
	if( doItalic ) ita = csale;

	float xo = x;
	const float cy = (1.0f/16.0f) * sizey;
	float cx;
	//-------------
    renderer->SetState( piSTATE_DEPTH_TEST, false );
    renderer->SetState( piSTATE_CULL_FACE, false );
    renderer->SetState(piSTATE_BLEND, true);
    renderer->SetBlending(0, piBLEND_ADD, piBLEND_SRC_ALPHA, piBLEND_ONE_MINUS_SRC_ALPHA,
                             piBLEND_ADD, piBLEND_SRC_ALPHA, piBLEND_ONE_MINUS_SRC_ALPHA);

	renderer->AttachShader( mShader );
	renderer->SetShaderConstant4F( 0, color, 1 );
    renderer->AttachTextures( 1, mTexture );

	const int len = str?piwstrlen( str ):0;

	if( cursorPos==0 )
	{
		if( drawCursor  )
			drawCursorF(  renderer, x, y, cursorTime, ita, sizey, csale, cy );
	}

    if( len==0 ) return;

	int lid = 0;
    int cid = 0;
	for( int i=0; i<len; i++ )
	{
		const wchar_t c = str[i];
		if( c==10 )//L'\n' )
		{
			x  = xo;
			y -= cy;
            lid += 1;
            cid = 0;
		}
		if( c==32 )
		{
            if( mFontType==IQRFONT_COURIER )
            {
			    const float tx0 = (float)mCharInfo['a'].xmin;
			    const float tx1 = (float)mCharInfo['a'].xmax;
			    cx = (tx1 - tx0)*sizex * ixres;
            }
            else
			    cx = csale;
			x += cx + (csale/8.0f)*sizex;
            cid+=1;
		}
		else
		{
			if( c<32 || c>127 ) continue;

			const int iy = c >> 4;

			const float tx0 = (float)mCharInfo[c].xmin * ixres;
			const float tx1 = (float)mCharInfo[c].xmax * ixres;

			const float ty = (float)iy/8.0f;
			const float th = 1.0f/8.0f;

			cx = (tx1 - tx0)*sizex;

            const float xyuv[8] = { x, y, tx0, ty + th, x + cx, y + cy, tx1, ty };
            renderer->SetShaderConstant4F( 1, xyuv, 2 );
            renderer->DrawUnitQuad_XY(1);

			x += cx + 4.0f*(csale/8.0f)*sizex;

            cid+=1;
		}

		if( drawCursor && (i+1)==cursorPos )
			drawCursorF( renderer, x, y, cursorTime, ita, sizey, cx, cy );
	}

	renderer->DettachTextures();
	renderer->DettachShader();

    renderer->SetState(piSTATE_BLEND, false);

    if( finalXY ) { finalXY[0] = x;  finalXY[1] = y; }
}


void piRenderFont2D::Prints( piRenderer *renderer, float x, float y, float z, const mat4x4& mMVP, float size, bool doItalic,
                           bool drawCursor, int cursorPos, double cursorTime,
                           const wchar_t *str, const float *color, const drawCharacterCB * dcCB, float *finalXY )
{
    const int xres = mImage.GetXRes();
    const float csale = (mFontType==IQRFONT_COURIER) ? 0.0f : 16.0f/(float)xres;

    const float ixres = 1.0f /(float)xres;

    float ita = 0.0f;
    if( doItalic ) ita = csale;

    float xo = x;
    const float cy = (1.0f/16.0f) * size;
    float cx;
    //-------------
    renderer->SetState(piSTATE_BLEND, true);
    renderer->SetBlending(0, piBLEND_ADD, piBLEND_SRC_ALPHA, piBLEND_ONE_MINUS_SRC_ALPHA,
                             piBLEND_ADD, piBLEND_SRC_ALPHA, piBLEND_ONE_MINUS_SRC_ALPHA);
    renderer->SetState(piSTATE_DEPTH_TEST, false);
    renderer->SetState( piSTATE_CULL_FACE, false );

    renderer->AttachShader( mShader3d );
    renderer->SetShaderConstant4F( 0, color, 1 );
    renderer->AttachTextures( 1, mTexture );

    const int len = str?piwstrlen( str ):0;

    if( cursorPos==0 )
    {
        if( drawCursor  )
            drawCursorF( renderer, x, y, cursorTime, ita, size, csale, cy );
    }

    if( len==0 ) return;

    int lid = 0;
    int cid = 0;
    for( int i=0; i<len; i++ )
    {
        const wchar_t c = str[i];
        if( c==10 )//L'\n' )
        {
            x  = xo;
            y -= cy;
            lid += 1;
            cid = 0;
        }
        if( c==32 )
        {
            if( mFontType==IQRFONT_COURIER )
            {
                const float tx0 = (float)mCharInfo['a'].xmin;
                const float tx1 = (float)mCharInfo['a'].xmax;
                cx = (tx1 - tx0)*size * ixres;
            }
            else
                cx = csale;
            x += cx + (csale/8.0f)*size;
            cid+=1;
        }
        else
        {
            if( c<32 || c>127 ) continue;

            const int iy = c >> 4;

            const float tx0 = (float)mCharInfo[c].xmin * ixres;
            const float tx1 = (float)mCharInfo[c].xmax * ixres;

            const float ty = (float)iy/8.0f;
            const float th = 1.0f/8.0f;

            cx = (tx1 - tx0)*size;

            const float xyuv[8] = { x, y, tx0, ty + th, x + cx, y + cy, tx1, ty };
            renderer->SetShaderConstant4F( 1, xyuv, 2 );
            renderer->SetShaderConstantMat4F( 4, (float*)&mMVP, 1, true);
            renderer->SetShaderConstant1F( 3, &z, 1 );
            renderer->DrawUnitQuad_XY(1);

            x += cx + 4.0f*(csale/8.0f)*size;

            cid+=1;
        }

        if( drawCursor && (i+1)==cursorPos )
            drawCursorF( renderer, x, y, cursorTime, ita, size, cx, cy );
    }

    renderer->DettachTextures();
    renderer->DettachShader();

    renderer->SetState(piSTATE_BLEND, false);

    if( finalXY ) { finalXY[0] = x;  finalXY[1] = y; }
}


static bool isletter( const wchar_t c )
{
    if( c>=L'a' && c<L'z' ) return true;
    if( c>=L'A' && c<L'Z' ) return true;
    return false;
}

static bool tokencompare( const wchar_t *str, const wchar_t *token, int *tlen )
{
    int i;
    for( i=0; token[i]; i++ )
        if( str[i] != token[i] ) 
            return false;

    if( isletter(str[i]) ) return false;

    tlen[0] = i;
    return true;
}


static bool istoken( const wchar_t *str, const wchar_t **tokens, const int numtokens, int *len )
{
    for( int i=0; i<numtokens; i++ )
        if( tokencompare(str, tokens[i], len ) ) 
            return true; 
    return false;
}


void piRenderFont2D::PrintsSyntax( piRenderer *renderer, float x, float y, float sizex, float sizey, bool doItalic,
						  bool drawCursor, int cursorPos, double cursorTime,
                          const int selectionEnabled, const int selectionStart, const int selectionEnd,
						  const wchar_t *str, const piRenderFontSyntax *syntax )
{
    const int xres = mImage.GetXRes();
    const float ixres = 1.0f / (float)xres;
	const float csale = (mFontType==IQRFONT_COURIER) ? 0.0f : 16.0f/(float)xres;
	
    float curColor[4] = { 1.0f, 0.4f, 0.3f, 1.0f };

	float ita = 0.0f;
	if( doItalic )
		ita = csale;

	float xo = x;
	const float cy = (1.0f/16.0f) * sizey;
	float cx;
	//-------------
    renderer->SetState(piSTATE_BLEND, true);
    renderer->SetBlending(0, piBLEND_ADD, piBLEND_SRC_ALPHA, piBLEND_ONE_MINUS_SRC_ALPHA,
                             piBLEND_ADD, piBLEND_SRC_ALPHA, piBLEND_ONE_MINUS_SRC_ALPHA);
    renderer->SetState(piSTATE_DEPTH_TEST, false);
    renderer->SetState( piSTATE_CULL_FACE, false );
    renderer->SetState( piSTATE_FRONT_FACE, true );

	float params[4];

	params[0] = syntax->mDefaultStyle.mColor[0];
	params[1] = syntax->mDefaultStyle.mColor[1];
	params[2] = syntax->mDefaultStyle.mColor[2];
	params[3] = syntax->mDefaultStyle.mColor[3];

	renderer->AttachShader( mShader );
	renderer->SetShaderConstant4F( 0, params, 1 );
	renderer->AttachTextures( 1, mTexture );

	const int len = str?piwstrlen( str ):0;

    int tlen = 0;

    int mode = 0;  // 0=normal, 1=token
/*
	if( len==0 || cursorPos==0 )
	{
		if( drawCursor  )
		{
            renderer->SetShaderConstant4F( 0, curColor, 4 );
			drawCursorF( me, renderer, x, y, cursorTime, ita, sizey, 0.0f, cy );
            renderer->SetShaderConstant4F( 0, params, 4 );
		}
	}
*/
    renderer->AttachTextures( 1, mTexture );

	for( int i=0; i<len; i++ )
	{
        //const bool selected = selectionEnabled && (i>=selectionStart) && (i<=selectionEnd);

        if( mode==0 )
        {
            if( i==0 || ((i>0) && !isletter(str[i-1])   ) )
            {
			const int numGroups = syntax->mNumGroups;
            for( int j=0; j<numGroups; j++ )
            {
                const piRenderFontSyntax_Tokens *sgr = syntax->mGroups + j;
                if( istoken( str+i, sgr->mTokens, sgr->mNumTokens, &tlen ) )
                {
                    mode = 1;
                    params[0] = sgr->mStyle.mColor[0];
                    params[1] = sgr->mStyle.mColor[1];
                    params[2] = sgr->mStyle.mColor[2];
                    params[3] = sgr->mStyle.mColor[3];
                    renderer->SetShaderConstant4F( 0, params, 4 );
                    tlen++;
                    break;
                }
            }
            }
        }

		if( mode==1 ) 
        { 
            tlen--; 
            if( tlen==0 ) 
            { 
                mode=0; 
	            params[0] = syntax->mDefaultStyle.mColor[0];
	            params[1] = syntax->mDefaultStyle.mColor[1];
	            params[2] = syntax->mDefaultStyle.mColor[2];
	            params[3] = syntax->mDefaultStyle.mColor[3];
	            renderer->SetShaderConstant4F( 0, params, 4 );
            }
        }

		const wchar_t c = str[i];
		if( c==10 )//L'\n' )
		{
			x  = xo;
			y -= cy;
		}
		if( c==32 )
		{
            if( mFontType==IQRFONT_COURIER )
            {
			    const float tx0 = (float)mCharInfo['a'].xmin;
			    const float tx1 = (float)mCharInfo['a'].xmax;
			    cx = (tx1 - tx0)*sizex * ixres;
            }
            else
			    cx = csale;
			x += cx + (csale/8.0f)*sizex;;
		}
		else
		{
			if( c<32 || c>127 ) continue;

			const int iy = c >> 4;
			const float tx0 = (float)mCharInfo[c].xmin * ixres;
			const float tx1 = (float)mCharInfo[c].xmax * ixres;
			const float ty = (float)iy/16.0f;
			const float th = 1.0f/16.0f;
			cx = (tx1 - tx0)*sizex;

//            renderer->DrawRect_2F_2F( x, y, tx0, ty+th, x+cx, y+cy, tx1, ty );

			x += cx + (csale/8.0f)*sizex;
		}

		if( drawCursor && (i+1)==cursorPos )
		{
			drawCursorF( renderer, x, y, cursorTime, ita, sizey, cx, cy );
		}

	}

	renderer->DettachTextures();
	renderer->DettachShader();

    renderer->SetState(piSTATE_BLEND, false);
}

void piRenderFont2D::Length( float sizex, float sizey, bool doItalic, const wchar_t *str, float *res )
{
    const int xres = mImage.GetXRes();
    const float ixres = 1.0f / (float)xres;
	const float csale = (mFontType==IQRFONT_COURIER) ? 0.0f : 16.0f/(float)xres;

	float ita = 0.0f;
	if( doItalic )
		ita = csale;

    float x = 0.0f;
    float y = 0.0f;
	float xo = 0.0f;
	const float cy = (1.0f/16.0f) * sizey;
	float cx;
	//-------------

	const int len = piwstrlen( str );


	for( int i=0; i<len; i++ )
	{
		
		const wchar_t c = str[i];
		if( c==10 )//L'\n' )
		{
			x  = xo;
			y -= cy;
		}
		if( c==32 )
		{
            if( mFontType==IQRFONT_COURIER )
            {
			    const float tx0 = (float)mCharInfo['a'].xmin ;
			    const float tx1 = (float)mCharInfo['a'].xmax;
			    cx = (tx1 - tx0)*sizex/(float)xres;
            }
            else
			    cx = csale;
			x += cx + (csale/8.0f)*sizex;;
		}
		else
		{
			if( c<32 || c>127 ) continue;

			const int iy = c >> 4;
			const float tx0 = (float)mCharInfo[c].xmin / (float)xres;
			const float tx1 = (float)mCharInfo[c].xmax / (float)xres;
			const float ty = (float)iy/16.0f;
			const float th = 1.0f/16.0f;
			cx = (tx1 - tx0)*sizex;
			x += cx + (csale/8.0f)*sizex;
		}

	}

    res[0] = fabsf(x);
    res[1] = fabsf(y);
}

}