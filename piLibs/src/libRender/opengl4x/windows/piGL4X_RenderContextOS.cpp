#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <GL/gl.h>
#include <malloc.h>
#include "wglext.h"

#include "../piGL4X_RenderContext.h"

namespace piLibs {

typedef bool (APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int);

typedef struct
{
	int                       mNumWindows;					 
	HWND                      pWnd[8];
	HDC                       pDC[8];
	int                       mActualWindow;
    HGLRC                     pRC;
    int                       pIsDoubleBuffered;
    HDC                       pPrevHD;
    HGLRC                     pPrevRC;
}OglRenderContextOS;

static PIXELFORMATDESCRIPTOR pfd = {
	    sizeof(PIXELFORMATDESCRIPTOR),
	    1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,             // Color depth
        0,              // cRedBits     // do not set to 8! : this was the famous 16bit desktop bug
        0,              // cRedShiftc
        0,              // cGreenBits   // do not set to 8! : this was the famous 16bit desktop bug
        0,              // cGreenShift
        0,              // cBlueBits    // do not set to 8! : this was the famous 16bit desktop bug
        0,              // cBlueShift
        0,              // cAlphaBits   // do not set to 8! : this was the famous 16bit desktop bug
        0,              // cAlphaShift
        0,              // cAccumBits
        0,              // cAccumRedBits
        0,              // cAccumGreenBits
        0,              // cAccumBlueBits
        0,              // cAccumAlphaBits
        24,             // cDepthBits
        8,              // cStencilBits
        0,              // cAuxBuffers
        PFD_MAIN_PLANE, // iLayerType
        0,              // bReserved
        0,              // dwLayerMask
        0,              // dwVisibleMask
        0}; 



static int iiSetPixelFormat( HDC hdc, int doublebuffered )
{
    unsigned int      PixelFormat;

    PIXELFORMATDESCRIPTOR pfdGDI = pfd;
	if(!doublebuffered)
	{
		pfdGDI.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_SUPPORT_GDI;
	}

    #if 1
    PixelFormat = ChoosePixelFormat( hdc, &pfdGDI);
    #else

    int attributes[] =
    {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
    WGL_COLOR_BITS_ARB,     24,
    WGL_ALPHA_BITS_ARB,     8,
    WGL_DEPTH_BITS_ARB,     24,
    WGL_STENCIL_BITS_ARB,   8,
    WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
    WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
    WGL_SAMPLES_ARB,        4,
    0, 0
    };
    unsigned int numFormats = 0;

    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress( "wglChoosePixelFormatARB" );
    if( wglChoosePixelFormatARB )
    {
        int res = wglChoosePixelFormatARB( hdc, attributes, 0, 1, (int*)&PixelFormat, &numFormats );
        if( res!=0 && numFormats>0 )
        {
            numFormats = 0;
        }
    }
    #endif

	if( !PixelFormat )
		return 0 ;

	if( !SetPixelFormat( hdc, PixelFormat, &pfdGDI ))
		return 0 ;

    // intercept software rendering...
    if( !DescribePixelFormat( hdc, PixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfdGDI) )
        return( 0 );

    if( pfdGDI.dwFlags & PFD_GENERIC_FORMAT )
    {
        //"No HW acceleration"
        return( 0 );
    }

    return 1;
}


int piGL4X_RenderContext::Create( const void **hwnd, int numwindows, bool disableVSynch, bool doublebuffered, bool antialias )
{
	OglRenderContextOS *meOS = (OglRenderContextOS*)malloc( sizeof(OglRenderContextOS) );
    if( !meOS )
        return 0;

    mData = meOS;
	
	meOS->mNumWindows = numwindows;
	meOS->mActualWindow = 0;
    meOS->pIsDoubleBuffered = doublebuffered;
    meOS->pPrevHD = 0;
    meOS->pPrevRC = 0;


	for( int i=0; i<numwindows; i++ )
	{
		meOS->pWnd[i] = (HWND)(hwnd[i]);

		meOS->pDC[i] = GetDC(meOS->pWnd[i]);
		if( !meOS->pDC[i] )
			return 0 ;

		if( !iiSetPixelFormat(meOS->pDC[i],meOS->pIsDoubleBuffered) )
			return 0;
	}

    meOS->pRC = wglCreateContext(meOS->pDC[0]);
	if( !meOS->pRC )
		return 0 ;


    {
	    int attribs[] = {
		    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		    WGL_CONTEXT_MINOR_VERSION_ARB, 5,
		    //WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            WGL_CONTEXT_FLAGS_ARB , WGL_CONTEXT_DEBUG_BIT_ARB,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		    0 };

        HGLRC cc = wglGetCurrentContext();
        HDC   cd = wglGetCurrentDC();

        if( !wglMakeCurrent(meOS->pDC[0],meOS->pRC) )
            return( 0 );
    	
	    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress( "wglGetExtensionsStringARB" );
	    if( !wglGetExtensionsStringARB )
		    return 0;

	    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress( "wglCreateContextAttribsARB" );
	    if( !wglCreateContextAttribsARB )
		    return 0;

        if (disableVSynch)
        {
            PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
            if( wglSwapIntervalEXT )
                wglSwapIntervalEXT(false);
        }


	    HGLRC h42 = wglCreateContextAttribsARB(meOS->pDC[0],0, attribs);
	    if( !h42 )
		    return 0;
	    wglMakeCurrent(cd,cc);
	    wglDeleteContext(meOS->pRC);
	    meOS->pRC = h42;
    }

    return 1 ;
}

int piGL4X_RenderContext::SetActiveWindow(  int id )
{
    OglRenderContextOS  *meOS = (OglRenderContextOS*)mData;

    if( id == meOS->mActualWindow ) 
        return 1;

	meOS->mActualWindow = id;

	if( !wglMakeCurrent(meOS->pDC[id],meOS->pRC) )
        return 0;

    return 1;
}

void piGL4X_RenderContext::Destroy( void )
{
    OglRenderContextOS  *meOS = (OglRenderContextOS*)mData;

    if( meOS->pRC )
    {
        wglMakeCurrent(0,0);
        wglDeleteContext(meOS->pRC);
    }

	for( int i=0; i<meOS->mNumWindows; i++ )
	{
        ReleaseDC(meOS->pWnd[i],meOS->pDC[i]);
	}
}


void piGL4X_RenderContext::Enable( void )
{
    OglRenderContextOS  *meOS = (OglRenderContextOS*)mData;

    meOS->pPrevHD = wglGetCurrentDC();
    meOS->pPrevRC = wglGetCurrentContext();

	if( !wglMakeCurrent(meOS->pDC[meOS->mActualWindow],meOS->pRC) )
        return;
}

void piGL4X_RenderContext::Disable( bool doSwapBuffers )
{
    OglRenderContextOS  *meOS = (OglRenderContextOS*)mData;

    if( doSwapBuffers )
		::SwapBuffers(meOS->pDC[meOS->mActualWindow]);

    wglMakeCurrent( meOS->pPrevHD, meOS->pPrevRC );
}

void piGL4X_RenderContext::SwapBuffers( void )
{
    OglRenderContextOS  *meOS = (OglRenderContextOS*)mData;
	::SwapBuffers( meOS->pDC[meOS->mActualWindow] );
}


void piGL4X_RenderContext::Delete( void )
{
    OglRenderContextOS  *meOS = (OglRenderContextOS*)mData;
    free( meOS );
}
/*
static void iGetDimensions( piGLRenderContext *me, int *res )
{
    OglRenderContextOS  *meOS = (OglRenderContextOS*)me->mData;

    RECT rect;
    GetClientRect( meOS->pWnd, &rect );

    res[0] = rect.left;
    res[1] = rect.top;
    res[2] = rect.right - rect.left;
    res[3] = rect.bottom - rect.top;
}
*/

}