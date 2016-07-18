#include <windows.h>
#include <vfw.h>

#include <malloc.h>
#include <string.h>
#include "piAvi.h"

namespace piLibs {

typedef struct
{
    PAVISTREAM         m_ps;
    PAVISTREAM	       m_psCompressed;
    AVISTREAMINFO      m_strhdr;
    AVICOMPRESSOPTIONS m_opts;
    PAVIFILE	       m_pfile;
    BITMAPINFOHEADER   m_alpbi;
    unsigned int       m_width;
    unsigned int       m_height;
    unsigned int       m_frameCount;
}AVIWRITER;

piAviWriter piAviWriter_Init( const wchar_t *name, int xres, int yres, int fps )
{
    AVIWRITER	*me;

    me = (AVIWRITER*)malloc( sizeof(AVIWRITER) );
    if( !me )
        return( 0 );

    me->m_width = xres;
    me->m_height = yres;

    AVIFileInit();
    AVIFileOpen(&me->m_pfile, name, OF_WRITE | OF_CREATE, NULL);

    memset(&me->m_strhdr, 0, sizeof(me->m_strhdr));
    me->m_strhdr.fccType = streamtypeVIDEO;
    me->m_strhdr.fccHandler = 0;
    me->m_strhdr.dwScale = 1;
    me->m_strhdr.dwRate = fps;
    me->m_strhdr.dwSuggestedBufferSize = 3 * xres * yres;
    SetRect( &me->m_strhdr.rcFrame, 0, 0, xres, yres );

    if (AVIFileCreateStream(me->m_pfile, &me->m_ps, &me->m_strhdr) != 0)
        return( 0 );

    memset(&me->m_opts, 0, sizeof(me->m_opts));
    me->m_opts.fccType = streamtypeVIDEO;
    //me->m_opts.fccHandler = mmioFOURCC('t', 'u', 'i', 'd');
    //me->m_opts.fccHandler = mmioFOURCC('c', 'v', 'i', 'd');
    //me->m_opts.fccHandler = mmioFOURCC('d', 'i', 'v', 'x');
    me->m_opts.fccHandler = mmioFOURCC('D','I','B',' '); // uncompressed frames
//me->m_opts.fccHandler = mmioFOURCC('H','2','6','4'); // uncompressed frames
    //me->m_opts.fccHandler = mmioFOURCC('W', 'M', 'V', '1');
    //me->m_opts.fccHandler = mmioFOURCC('H', 'F', 'Y', 'U');

    me->m_opts.dwKeyFrameEvery = 32;
    me->m_opts.dwQuality = 100;
    me->m_opts.dwBytesPerSecond = 0;
    me->m_opts.dwFlags = AVICOMPRESSF_DATARATE | AVICOMPRESSF_KEYFRAMES;

    if (AVIMakeCompressedStream(&me->m_psCompressed, me->m_ps, &me->m_opts, NULL) != AVIERR_OK)
        return( 0 );

    memset(&me->m_alpbi, 0, sizeof(me->m_alpbi));
    me->m_alpbi.biSize = sizeof(me->m_alpbi);
    me->m_alpbi.biWidth = xres;
    me->m_alpbi.biHeight = yres;
    me->m_alpbi.biPlanes = 1;
    me->m_alpbi.biBitCount = 24;
    me->m_alpbi.biCompression = BI_RGB;

    if (AVIStreamSetFormat(me->m_psCompressed, 0, &me->m_alpbi, sizeof(me->m_alpbi)) != 0)
        return( 0 );

    me->m_frameCount = 0;

    return( (piAviWriter)me );
}

void piAviWriter_End( piAviWriter vme )
{
    AVIWRITER *me = (AVIWRITER*)vme;

    AVIStreamClose( me->m_ps );
    AVIStreamClose( me->m_psCompressed );
    AVIFileClose( me->m_pfile );
    AVIFileExit();

    free( me );
}

int piAviWriter_DoFrame( piAviWriter vme, const unsigned char *data )
{
    AVIWRITER *me = (AVIWRITER*)vme;

    if (AVIStreamWrite(me->m_psCompressed, me->m_frameCount, 1, (void*)data, 3 * me->m_width * me->m_height, AVIIF_KEYFRAME, NULL, NULL) != 0)
        return( 0 );

    me->m_frameCount++;

    return 1;
}

//------------------------------------------------

typedef struct
{
    PAVISTREAM mStream;
    PGETFRAME  mFrame;
    int mXres;
    int mYres;
    int mNumframes;
    int mFps;
}iIqAviReader;


void *piAviReader_Init( const wchar_t *name )
{
    iIqAviReader *me = (iIqAviReader*)malloc( sizeof(iIqAviReader) );
    if( !me )
        return( 0 );
    memset( me, 0, sizeof(iIqAviReader) );
    //------------------
    AVIFileInit();
    //------------------
    PAVIFILE avi=NULL;
    int res = AVIFileOpen( &avi, name, OF_READ, NULL );
    if (res!=AVIERR_OK)
    {
        if (avi!=NULL) AVIFileRelease(avi);
        free( me );
        return NULL;
    }
    //------------------
    AVIFILEINFO avi_info;
    AVIFileInfo( avi, &avi_info, sizeof(AVIFILEINFO) );

    me->mXres = avi_info.dwWidth;
    me->mYres = avi_info.dwHeight;
    me->mNumframes = avi_info.dwLength;
    me->mFps = avi_info.dwRate / avi_info.dwScale;
    //----------------
    res=AVIFileGetStream( avi, &me->mStream, streamtypeVIDEO, 0 );
    if (res!=AVIERR_OK)
    {
        if (me->mStream!=NULL) AVIStreamRelease(me->mStream);
        if (avi!=NULL) AVIFileRelease(avi);
        AVIFileExit();
        return NULL;
    }
    //-------------------
    const int iNumFrames = AVIStreamLength( me->mStream );
    const int iFirstFrame = AVIStreamStart( me->mStream );
    if( iFirstFrame==-1 || iNumFrames==-1 )
    {
        if (me->mStream!=NULL) AVIStreamRelease( me->mStream );
        if (avi!=NULL) AVIFileRelease(avi);
        AVIFileExit();
        return NULL;
    }
    //---------------
    me->mFrame = AVIStreamGetFrameOpen(me->mStream, NULL );
	if( me->mFrame==NULL )
		return NULL;

    return( (piAviReader)me );
}

void piAviReader_End( piAviReader vme )
{
    iIqAviReader *me = (iIqAviReader*)vme;

    AVIStreamGetFrameClose( me->mFrame );
    if (me->mStream!=NULL) AVIStreamRelease(me->mStream);
    AVIFileExit();

    free( me );
}

void piAviReader_GetInfo( piAviReader vme, int *xres, int *yres, int *numframes, int *fps )
{
    iIqAviReader *me = (iIqAviReader*)vme;

    *xres = me->mXres;
    *yres = me->mYres;
    *numframes = me->mNumframes;
    *fps = me->mFps;
}

unsigned char * piAviReader_GetFrame( piAviReader vme, int index )
{
    iIqAviReader *me = (iIqAviReader*)vme;

    unsigned char *ptr =(unsigned char*)AVIStreamGetFrame(me->mFrame, index);
	if( !ptr ) 
		return 0;

    BITMAPINFOHEADER *header = (BITMAPINFOHEADER*)ptr;

    return ptr + sizeof(BITMAPINFOHEADER);
}

}