#pragma once

class piCamera
{
public:
    virtual bool Start( void ) = 0;
    virtual bool Stop( void ) = 0;

    virtual bool IsNewFrame( void ) = 0;
    virtual void *GetBuffer( void ) = 0;
};

class piCameraMgr
{
public:
    static piCameraMgr * Create( void );
    static void          Destroy(piCameraMgr *me);

    virtual bool  Init( void ) = 0;
    virtual void  End( void ) = 0;
    virtual int   GetNumDevices( void ) = 0;
    virtual bool  GetDeviceInfo( int id, wchar_t **name, int *numcaps, int **caps ) = 0;

    virtual piCamera *CreateCamera( int deviceID, int capID, int *xres, int *yres, int *rate ) = 0;
    virtual void      DestroyCamera( piCamera *cam ) = 0;
};

