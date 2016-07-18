#pragma once

namespace piLibs {

class piGL4X_RenderContext
{
public:
	piGL4X_RenderContext() {}
	~piGL4X_RenderContext() {}

public:
    int  Create( const void **hwnd, int num, bool disableVSynch, bool doublebuffered, bool antialias );
    int  SetNewWindowHandle( void *hwnd );
    int  SetActiveWindow( int id );
    void Destroy( void );
    void Enable( void );
    void Disable( bool doSwapBuffers );
    void SwapBuffers( void );
    void Delete( void );
public:
    void *mData;
};

} // namespace piLibs