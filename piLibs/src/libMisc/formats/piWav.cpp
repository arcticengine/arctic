#include <malloc.h>
#include "../../libSystem/piTypes.h"
#include "../../libSystem/piFile.h"

#include "piWav.h"


namespace piLibs {

#define  WAV_PCM      1                  // PCM (uncompressed) wave file
#define  WAV_RIFF     0x46464952         // "RIFF"
#define  WAV_WAVE     0x45564157         // "WAVE"
#define  WAV_FMT      0x20746D66         // " fmt"
#define  WAV_DATA     0x61746164         // "data"

typedef struct                   // wave chunk header
{
   uint32 ID;
   uint32 SIZE;
}CHUNKSTRUCT;

typedef struct
{
   uint16 FORMAT;
   uint16 CHANNEL;
   uint32 RATE;
   uint32 AVGBYTES;
   uint16 BLOCKALIGN;
   uint16 BIT;
}FMTSTRUCT;




typedef struct
{
  CHUNKSTRUCT  riff;
  uint32       wave;
  CHUNKSTRUCT  fmt;
  FMTSTRUCT    format;
  CHUNKSTRUCT  data;
}HEADERSTRUCT;


piWav::piWav()
{
}

piWav::~piWav()
{
}

bool piWav::Open(const wchar_t *name)
{
    piFile          fp;
    if( !fp.Open(name, L"rb") )
        return false;

    HEADERSTRUCT    header;
    fp.Read(&header, sizeof(HEADERSTRUCT));

    if( header.riff.ID != WAV_RIFF || header.wave != WAV_WAVE || 
        header.fmt.ID != WAV_FMT || header.format.FORMAT != WAV_PCM )
    {
        fp.Close();
        return false;
    }

    while (header.data.ID != WAV_DATA )
    {
        //0x74786562 // bext
        //0x6b6e756a // junk
        fp.Seek(header.data.SIZE, piFile::CURRENT );
        fp.Read(&header.data, sizeof(CHUNKSTRUCT) );
    }

    const int datalength = header.data.SIZE;

    mData = (void *)malloc(datalength);
    if (!mData)
        return(0);

    fp.Read(mData, datalength);

    fp.Close();

    mNumChannels = header.format.CHANNEL;
    mRate = header.format.RATE;
    mBits = header.format.BIT;
    mNumSamples = datalength / (mNumChannels*mBits/8);
    mDataSize = datalength;

    return true;
}


void piWav::Deinit(void)
{
    free( mData );
}



/*
int WAV_Save( const wchar_t *name, void *data, int samples, int bytespersample, int channels, int freq )
{
    HEADERSTRUCT    header;
    FILE            *fp;
    long            datalength;

    datalength = samples*bytespersample*channels;
  
    fp = _wfopen( name, 0 );
    if( !fp )
        return( 0 );

    header.riff.ID = WAV_RIFF;
    header.riff.SIZE = datalength + sizeof(HEADERSTRUCT) - sizeof(CHUNKSTRUCT);
    header.wave = WAV_WAVE;
    header.fmt.ID = WAV_FMT;
    header.fmt.SIZE = sizeof( FMTSTRUCT);
    header.format.FORMAT = WAV_PCM;
    header.format.CHANNEL = channels;

    header.format.RATE = freq;

    header.format.AVGBYTES = freq * bytespersample * channels;
    header.format.BLOCKALIGN = 1 * bytespersample * channels;
    header.format.BIT = 8*bytespersample;
    header.data.ID = WAV_DATA;
    header.data.SIZE = datalength;

    fwrite( &header, sizeof(HEADERSTRUCT), 1, fp );

    fwrite( data, 1, datalength, fp );

    fclose( fp );
    return( 1 );
}
*/


/*
bool piWav_Open( const wchar_t *name, FILE **fpr, WAVINFO *info )
{
    HEADERSTRUCT    header;

    if( _wfopen_s( fpr, name, L"rb" )!=0 )
        return false;

    fread( &header, sizeof(HEADERSTRUCT), 1, *fpr );

    if( header.riff.ID != WAV_RIFF || header.wave          != WAV_WAVE ||
        header.fmt.ID  != WAV_FMT  || header.format.FORMAT != WAV_PCM )
    {
        fclose( *fpr );
        return false;
    }

    info->CHANNEL = header.format.CHANNEL;
    info->RATE    = header.format.RATE;
    info->BIT     = header.format.BIT;
    info->nsamp   = header.data.SIZE;
    info->header  = sizeof(HEADERSTRUCT);

    return true;
}
*/
/*
FILE *piWav_Open( const wchar_t *name, WAVINFO *info )
{
    HEADERSTRUCT    header;

    FILE *fp = _wfopen( name, L"rb" );
    if( !fp )
        return 0;

    fread( &header, sizeof(HEADERSTRUCT), 1, fp );

    if( header.riff.ID != WAV_RIFF || header.wave          != WAV_WAVE ||
        header.fmt.ID  != WAV_FMT  || header.format.FORMAT != WAV_PCM )
    {
        fclose( fp );
        return 0;
    }

    info->CHANNEL = header.format.CHANNEL;
    info->RATE    = header.format.RATE;
    info->BIT     = header.format.BIT;
    info->nsamp   = header.data.SIZE;
    info->header  = sizeof(HEADERSTRUCT);

    return fp;
}


/*
void *WAV_Load( char *name, WAVINFO *info )
{

    HEADERSTRUCT    header;
    int             datalength;
    FILE            *fp;
    void            *data;

 
    fp = fopen( name, "rb" );
    if( !fp )
        return( 0 );


    fread( &header, sizeof(HEADERSTRUCT), 1, fp );

    if( header.riff.ID       != WAV_RIFF ||
        header.wave          != WAV_WAVE ||
        header.fmt.ID        != WAV_FMT  ||
        header.format.FORMAT != WAV_PCM )
        {
        fclose( fp );
        return( 0 );
        }
        

    //datalength = header.riff.SIZE - sizeof(HEADERSTRUCT) + sizeof(CHUNKSTRUCT);
    datalength = header.data.SIZE;

    data = (void *)malloc( datalength );
    if( !data )
        return( 0 );

    fread( data, 1, datalength, fp );

    fclose( fp );

    info->CHANNEL = header.format.CHANNEL;
    info->RATE    = header.format.RATE;
    info->BIT     = header.format.BIT;
    info->nsamp   = datalength;

    return( data );
}
*/

}