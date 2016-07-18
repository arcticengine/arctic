#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <mmreg.h>
#include <msacm.h>
#include <wmsdk.h>
#include <malloc.h>

#pragma comment(lib, "msacm32.lib") 
#pragma comment(lib, "wmvcore.lib") 
#pragma comment(lib, "winmm.lib") 

#include "piDecompressMP3.h"
#include "../../libSystem/piFile.h"

namespace piLibs {

bool OpenMP3FromFile(piWav *dst, const wchar_t *name)
{
    piFile fp;
    if (!fp.Open( name, L"rb" ) )
        return false;

    uint64 len = fp.GetLength();
    unsigned char *buffer = (unsigned char *)malloc( (size_t)len );
    if( !buffer ) return false;

    fp.Read( buffer, len );

    bool res = OpenMP3FromMemory(dst, buffer, (int)len);

    free(buffer);
    fp.Close();

    return res;
}

// code ripped from here: http://code4k.blogspot.com/2010/05/playing-mp3-in-c-using-plain-windows.html
bool OpenMP3FromMemory(piWav *dst, unsigned char * mp3InputBuffer, int mp3InputBufferSize)
{
    // -----------------------------------------------------------------------------------
    // Extract and verify mp3 info : duration, type = mp3, sampleRate = 44100, channels = 2
    // -----------------------------------------------------------------------------------

    // Initialize COM
    CoInitialize(0);

    // Create SyncReader
    IWMSyncReader* wmSyncReader = nullptr;
    if (WMCreateSyncReader(NULL, WMT_RIGHT_PLAYBACK, &wmSyncReader) != S_OK)
        return false;

    // Alloc With global and create IStream
    HGLOBAL mp3HGlobal = GlobalAlloc(GPTR, mp3InputBufferSize);
    if (!mp3HGlobal ) return false;
    void* mp3HGlobalBuffer = GlobalLock(mp3HGlobal);
    memcpy(mp3HGlobalBuffer, mp3InputBuffer, mp3InputBufferSize);
    GlobalUnlock(mp3HGlobal);
    IStream* mp3Stream = nullptr;
    if (CreateStreamOnHGlobal(mp3HGlobal, FALSE, &mp3Stream) != S_OK)
        return false;

    // Open MP3 Stream
    HRESULT kk = wmSyncReader->OpenStream(mp3Stream);
    if (kk != S_OK )
        return false;

    // Get HeaderInfo interface
    IWMHeaderInfo* wmHeaderInfo = nullptr;
    if (wmSyncReader->QueryInterface(&wmHeaderInfo) != S_OK)
        return false;

    // Retrieve mp3 song duration in seconds
    WORD lengthDataType = sizeof(QWORD);
    QWORD durationInNano = 0;
    WORD wmStreamNum = 0;
    WMT_ATTR_DATATYPE wmAttrDataType = WMT_TYPE_DWORD;
    if (wmHeaderInfo->GetAttributeByName(&wmStreamNum, L"Duration", &wmAttrDataType, (BYTE*)&durationInNano, &lengthDataType) != S_OK)
        return false;
    double durationInSecond = ((double)durationInNano) / 10000000.0;
    DWORD durationInSecondInt = (int)(durationInNano / 10000000) + 1;

    // Sequence of call to get the MediaType
    // WAVEFORMATEX for mp3 can then be extract from MediaType
    IWMProfile* wmProfile = nullptr;
    if (wmSyncReader->QueryInterface(&wmProfile) != S_OK)
        return false;

    IWMStreamConfig* wmStreamConfig = nullptr;
    if (wmProfile->GetStream(0, &wmStreamConfig) != S_OK)
        return false;

    IWMMediaProps* wmMediaProperties = nullptr;
    if (wmStreamConfig->QueryInterface(&wmMediaProperties) != S_OK)
        return false;

    // Retrieve sizeof MediaType
    DWORD sizeMediaType = 0;
    if (wmMediaProperties->GetMediaType(NULL, &sizeMediaType) != S_OK)
        return false;

    // Retrieve MediaType
    WM_MEDIA_TYPE* mediaType = (WM_MEDIA_TYPE*)LocalAlloc(LPTR, sizeMediaType);
    if (wmMediaProperties->GetMediaType(mediaType, &sizeMediaType) != S_OK)
        return false;

    // Check that MediaType is audio
    if( mediaType->majortype != WMMEDIATYPE_Audio )
        return false;

    // Check that input is mp3
    WAVEFORMATEX* inputFormat = (WAVEFORMATEX*)mediaType->pbFormat;
    assert(inputFormat->wFormatTag == WAVE_FORMAT_MPEGLAYER3);

    
    int nChannels = inputFormat->nChannels;
    int nSamplesPerSec = inputFormat->nSamplesPerSec;
    int wBitsPerSample = inputFormat->wBitsPerSample;
    int nBlockAlign = inputFormat->nBlockAlign;
    int nAvgBytesPerSec = inputFormat->nAvgBytesPerSec;

    // Release COM interface
    // wmSyncReader->Close();
    wmMediaProperties->Release();
    wmStreamConfig->Release();
    wmProfile->Release();
    wmHeaderInfo->Release();
    wmSyncReader->Release();

    // Free allocated mem
    LocalFree(mediaType);


    dst->mNumChannels = nChannels;
    dst->mRate = nSamplesPerSec;
    dst->mBits = 16;//inputFormat->wBitsPerSample;
    dst->mNumSamples = 0;
    dst->mData = nullptr;
    dst->mDataSize = 0;

    wchar_t str1[256];

    swprintf_s(str1, 256, L"Channels: %d   %d Hz   %d bits   %d block   %d avbps", nChannels, nSamplesPerSec, wBitsPerSample, nBlockAlign, nAvgBytesPerSec);
    OutputDebugString(str1);


    // -----------------------------------------------------------------------------------
    // Convert mp3 to pcm using acm driver
    // The following code is mainly inspired from http://david.weekly.org/code/mp3acm.html
    // -----------------------------------------------------------------------------------


    // Define output format
    WAVEFORMATEX pcmFormat;
    memset(&pcmFormat, 0, sizeof(WAVEFORMATEX) );
    pcmFormat.wFormatTag = WAVE_FORMAT_PCM;
    pcmFormat.nChannels = nChannels;
    pcmFormat.wBitsPerSample = 16;//inputFormat->wBitsPerSample;
    pcmFormat.nSamplesPerSec = nSamplesPerSec;
    pcmFormat.nBlockAlign = pcmFormat.nChannels*pcmFormat.wBitsPerSample/8;
    pcmFormat.nAvgBytesPerSec = pcmFormat.nSamplesPerSec * pcmFormat.nBlockAlign;
    pcmFormat.cbSize = 0;


    const DWORD MP3_BLOCK_SIZE = 522;

    // Define input format
    MPEGLAYER3WAVEFORMAT mp3Format;
    memset(&mp3Format, 0, sizeof(MPEGLAYER3WAVEFORMAT));
    mp3Format.wID = MPEGLAYER3_ID_MPEG;
    mp3Format.fdwFlags = MPEGLAYER3_FLAG_PADDING_OFF;
    mp3Format.nBlockSize = MP3_BLOCK_SIZE;
    mp3Format.nFramesPerBlock = 1;
    mp3Format.nCodecDelay = 0;// 1393;
    mp3Format.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    mp3Format.wfx.nChannels = nChannels;
    mp3Format.wfx.nSamplesPerSec = nSamplesPerSec;
    mp3Format.wfx.nAvgBytesPerSec = nAvgBytesPerSec;
    mp3Format.wfx.nBlockAlign = nBlockAlign;
    mp3Format.wfx.wBitsPerSample = wBitsPerSample;
    mp3Format.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;


    // Get maximum FormatSize for all acm
    DWORD maxFormatSize = 0;
    if (acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &maxFormatSize) != S_OK)
        return false;

    // Allocate PCM output sound buffer
    dst->mDataSize = durationInSecondInt * pcmFormat.nAvgBytesPerSec;
    dst->mData = malloc(dst->mDataSize);
    if( !dst->mData ) return false;

    HACMSTREAM acmMp3stream = nullptr;
    switch (acmStreamOpen(&acmMp3stream,    // Open an ACM conversion stream
        NULL,                       // Query all ACM drivers
        (LPWAVEFORMATEX)&mp3Format, // input format :  mp3
        &pcmFormat,                 // output format : pcm
        NULL,                       // No filters
        0,                          // No async callback
        0,                          // No data for callback
        0                           // No flags
        )
        ) {
    case MMSYSERR_NOERROR:
        break; // success!
    case MMSYSERR_INVALPARAM:
        //assert(!"Invalid parameters passed to acmStreamOpen");
        return false;
    case ACMERR_NOTPOSSIBLE:
        //assert(!"No ACM filter found capable of decoding MP3");
        return false;
    default:
        //assert(!"Some error opening ACM decoding stream!");
        return false;
    }

    // Determine output decompressed buffer size
    unsigned long rawbufsize = 0;
    MMRESULT mmres = acmStreamSize(acmMp3stream, MP3_BLOCK_SIZE, &rawbufsize, ACM_STREAMSIZEF_SOURCE);
    if (mmres != 0)
        return false;
    if(rawbufsize < 1)
        return false;

    // allocate our I/O buffers
    static BYTE mp3BlockBuffer[MP3_BLOCK_SIZE];
    //LPBYTE mp3BlockBuffer = (LPBYTE) LocalAlloc( LPTR, MP3_BLOCK_SIZE );
    LPBYTE rawbuf = (LPBYTE)LocalAlloc(LPTR, rawbufsize);

    // prepare the decoder
    static ACMSTREAMHEADER mp3streamHead;
    // memset( &mp3streamHead, 0, sizeof(ACMSTREAMHEADER ) );
    mp3streamHead.cbStruct = sizeof(ACMSTREAMHEADER);
    mp3streamHead.pbSrc = mp3BlockBuffer;
    mp3streamHead.cbSrcLength = MP3_BLOCK_SIZE;
    mp3streamHead.pbDst = rawbuf;
    mp3streamHead.cbDstLength = rawbufsize;
    if (acmStreamPrepareHeader(acmMp3stream, &mp3streamHead, 0) != S_OK)
        return false;

    BYTE* currentOutput = (unsigned char*)dst->mData;
    DWORD totalDecompressedSize = 0;

    static ULARGE_INTEGER newPosition;
    static LARGE_INTEGER seekValue;
    if (mp3Stream->Seek(seekValue, STREAM_SEEK_SET, &newPosition) != S_OK)
        return false;

    while (1) 
    {
        // suck in some MP3 data
        ULONG count;
        if (mp3Stream->Read(mp3BlockBuffer, MP3_BLOCK_SIZE, &count) != S_OK)
            return false;

        if (count != MP3_BLOCK_SIZE)
            break;

        // convert the data
        if (acmStreamConvert(acmMp3stream, &mp3streamHead, ACM_STREAMCONVERTF_BLOCKALIGN) != S_OK)
            return false;

        // write the decoded PCM to disk
        memcpy(currentOutput, rawbuf, mp3streamHead.cbDstLengthUsed);
        totalDecompressedSize += mp3streamHead.cbDstLengthUsed;
        currentOutput += mp3streamHead.cbDstLengthUsed;
    };

    dst->mNumSamples = totalDecompressedSize / ((dst->mBits/8)*dst->mNumChannels);
    dst->mDataSize = totalDecompressedSize;

    if (acmStreamUnprepareHeader(acmMp3stream, &mp3streamHead, 0) != S_OK)
        return false;

    LocalFree(rawbuf);
    if (acmStreamClose(acmMp3stream, 0) != S_OK)
        return false;

    // Release allocated memory
    mp3Stream->Release();
    GlobalFree(mp3HGlobal);
    return true;
};



}