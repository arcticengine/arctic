// https://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.idirectsoundbuffer8.idirectsoundbuffer8.play(v=vs.85).aspx


#include <Dsound.h>
#include <math.h>

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")

#include "../../libMisc/formats/piWav.h"

#include "piSoundEngineDS.h"
#include "piDecompressMP3.h"

namespace piLibs {

piSoundEngineDS::piSoundEngineDS():piSoundEngine()
{
}
piSoundEngineDS::~piSoundEngineDS()
{
}


BOOL CALLBACK piSoundEngineDS::myDSEnumCallback(LPGUID lpGuid, const wchar_t * lpcstrDescription, const wchar_t * lpcstrModule, void * lpContext)
{
    piSoundEngineDS *me = (piSoundEngineDS*)lpContext;
    me->mDevice[me->mNumDevices].mGUID = lpGuid;
    const int len = wcslen(lpcstrDescription );
    me->mDevice[me->mNumDevices].mName = (wchar_t *)malloc( (len+2)*sizeof(wchar_t) );
    if( !me->mDevice[me->mNumDevices].mName )
        return FALSE;
    
    wcscpy_s(me->mDevice[me->mNumDevices].mName, len+2, lpcstrDescription);

    //wcscpy_s( me->mDevice[me->mNumDevices].mName, 63, lpcstrDescription );
    me->mNumDevices++;
    return TRUE;
}

int piSoundEngineDS::GetNumDevices(void)
{
    mNumDevices = 0;
    DirectSoundEnumerate(myDSEnumCallback, this);
    return mNumDevices;
}

const wchar_t *piSoundEngineDS::GetDeviceName(int id) const
{
    return mDevice[id].mName;
}

bool piSoundEngineDS::Init(void *hwnd, int deviceID)
{
    if (!mSounds.Init(1024, true ) )
        return false;

    

#if 1
    LPCGUID dv = NULL;
    if (deviceID>0) dv = mDevice[deviceID].mGUID;
    
    if (FAILED(DirectSoundCreate8(dv, &m_DirectSound, NULL)))
    {
        return false;
    }
#else

    static DSBUFFERDESC bufferDesc = {
        sizeof(DSBUFFERDESC),                       // DWORD           dwSize;
        DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME, // DWORD           dwFlags;
        0,                                          // DWORD           dwBufferBytes;
        0,                                          // DWORD           dwReserved;
        NULL,                                       // LPWAVEFORMATEX  lpwfxFormat;
        GUID_NULL                                   // GUID            guid3DAlgorithm;
    };
    if (FAILED(CoInitializeEx(NULL, 0)))
        return false;

    if (FAILED(CoCreateInstance(CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound8, (LPVOID*)&m_DirectSound)))
        return false;

    if (FAILED(m_DirectSound->Initialize(NULL)))
        return false;
#endif

    // Set the cooperative level to priority so the format of the primary sound buffer can be modified.
    if (FAILED(m_DirectSound->SetCooperativeLevel((HWND)hwnd, DSSCL_PRIORITY)))
        return false;

    #define WD_SAMPLERATE 44100
    #define WD_CHANNELS 2
    #define WD_BITS 16

    static WAVEFORMATEX waveFormat = {

        WAVE_FORMAT_PCM,       // wFormatTag;
        WD_CHANNELS,           // nChannels
        WD_SAMPLERATE,         // nSamplesPerSec 
        WD_SAMPLERATE*WD_BITS*WD_CHANNELS / 8, //nAvgBytesPerSec
        WD_BITS*WD_CHANNELS / 8, // nBlockAlign 
        WD_BITS,               // wBitsPerSample 
        0,                     // cbSize
    };

    DSBUFFERDESC bufferDesc = {
        sizeof(DSBUFFERDESC),                       // DWORD           dwSize;
        DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME, // DWORD           dwFlags;
        0,                                          // DWORD           dwBufferBytes;
        0,                                          // DWORD           dwReserved;
        NULL,                                       // LPWAVEFORMATEX  lpwfxFormat;
        GUID_NULL                                   // GUID            guid3DAlgorithm;
    };

#ifdef USE3D
    bufferDesc.dwFlags |= DSBCAPS_CTRL3D;
#endif

    // Get control of the primary sound buffer on the default sound device.
    if (FAILED(m_DirectSound->CreateSoundBuffer(&bufferDesc, &m_primaryBuffer, NULL)))
        return false;

    // Setup the format of the primary sound bufffer.
    // In this case it is a .WAV file recorded at 44,100 samples per second in 16-bit stereo (cd audio format).
    if (FAILED(m_primaryBuffer->SetFormat(&waveFormat)))
        return false;

    // Obtain a listener interface.
#ifdef USE3D
    if (FAILED(m_primaryBuffer->QueryInterface(IID_IDirectSound3DListener8, (void**)&m_listener)))
        return false;

    m_listener->SetPosition(0.0f, 0.0f, 0.0f, DS3D_IMMEDIATE);
    m_listener->SetDistanceFactor(1.0f, DS3D_IMMEDIATE);
    m_listener->SetRolloffFactor(1.0f, DS3D_IMMEDIATE);
#endif

    return true;
}


void piSoundEngineDS::Deinit(void)
{
    mSounds.End();

    // Release the primary sound buffer pointer.
    if (m_primaryBuffer)
    {
        m_primaryBuffer->Release();
        m_primaryBuffer = 0;
    }
    if (m_DirectSound)
    {
        m_DirectSound->Release();
        m_DirectSound = 0;
    }
}


int piSoundEngineDS::AddSound(const wchar_t *filename)
{
    int id = mSounds.GetLength();
    iSound *me = (iSound*)mSounds.Alloc(1, true);
    if (me == nullptr)
        return -1;

    me->mID = id;

    piWav wav;
    if (!wav.Open(filename))
    {
        if( !OpenMP3FromFile(&wav, filename) )
            return -1;
    }

    WAVEFORMATEX waveFormat = {

        WAVE_FORMAT_PCM,                // wFormatTag;
        wav.mNumChannels,               // nChannels
        wav.mRate,                      // nSamplesPerSec 
        wav.mRate*wav.mBits*wav.mNumChannels / 8, //nAvgBytesPerSec
        wav.mBits*wav.mNumChannels / 8, // nBlockAlign 
        wav.mBits,                      // wBitsPerSample 
        0,                              // cbSize
    };

    DSBUFFERDESC bufferDesc = { sizeof(DSBUFFERDESC), // dwSize;
                                DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS, // dwFlags;
                                (DWORD)wav.mDataSize,                     // dwBufferBytes;
                                0,                                        // dwReserved;
                                &waveFormat,                              // lpwfxFormat;
                                GUID_NULL                                 // guid3DAlgorithm;
                               };

    #ifdef USE3D
    if( wav.mNumChannels==1 )
        bufferDesc.dwFlags |= DSBCAPS_CTRL3D;
    #endif

    IDirectSoundBuffer* tempBuffer;
    // Create a temporary sound buffer with the specific buffer settings.
    HRESULT st = m_DirectSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL);
    if (FAILED(st))
    return -1;

    // Test the buffer format against the direct sound 8 interface and create the secondary buffer.
    if (FAILED(tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&me->mBuffer)))
        return -1;

#ifdef USE3D  
    if (wav.mNumChannels == 1)
    {
        if (FAILED(tempBuffer->QueryInterface(IID_IDirectSound3DBuffer8, (void**)&me->mBuffer3D)))
            return -1;
    }
#endif

  // Release the temporary buffer.
    tempBuffer->Release();
    tempBuffer = 0;

    //-------------------------

    unsigned short *bufferPtr;
    unsigned long bufferSize;
    if (FAILED(me->mBuffer->Lock(0, wav.mDataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0)))
    return -1;

    memcpy( bufferPtr, wav.mData, wav.mDataSize );

    // Unlock the secondary buffer after the data has been written to it.
    me->mBuffer->Unlock((void*)bufferPtr, bufferSize, NULL, 0);

    wav.Deinit();


    return id;
}


int piSoundEngineDS::AddSound(const SampleFormat *fmt, void *buffer)
{
    int id = mSounds.GetLength();
    iSound *me = (iSound*)mSounds.Alloc(1, true);
    if (me == nullptr)
        return -1;
    me->mID = id;

    int dataSize = (44100/10) * 2 * sizeof(short);

    // read this ffrom SampleFormat above
    #define WD_SAMPLERATE 44100
    #define WD_CHANNELS 2
    #define WD_BITS 16
    WAVEFORMATEX waveFormat = {

        WAVE_FORMAT_PCM,       // wFormatTag;
        WD_CHANNELS,           // nChannels
        WD_SAMPLERATE,         // nSamplesPerSec 
        WD_SAMPLERATE*WD_BITS*WD_CHANNELS / 8, //nAvgBytesPerSec
        WD_BITS*WD_CHANNELS / 8, // nBlockAlign 
        WD_BITS,               // wBitsPerSample 
        0,                     // cbSize
    };

    DSBUFFERDESC bufferDesc = {
        sizeof(DSBUFFERDESC),                       // DWORD           dwSize;
        DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS, // DWORD           dwFlags;
        (DWORD)dataSize,                                          // DWORD           dwBufferBytes;
        0,                                          // DWORD           dwReserved;
        &waveFormat,                                       // LPWAVEFORMATEX  lpwfxFormat;
        GUID_NULL                                   // GUID            guid3DAlgorithm;
    };

    IDirectSoundBuffer* tempBuffer;
    // Create a temporary sound buffer with the specific buffer settings.
    if (FAILED(m_DirectSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL)))
        return -1;

    // Test the buffer format against the direct sound 8 interface and create the secondary buffer.
    if (FAILED(tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&me->mBuffer)))
        return -1;

    // Release the temporary buffer.
    tempBuffer->Release();
    tempBuffer = 0;

    //-------------------------

    unsigned short *bufferPtr;
    unsigned long bufferSize;
    if (FAILED(me->mBuffer->Lock(0, dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0)))
        return -1;

    // Copy the wave data into the buffer.
    //memcpy(bufferPtr, waveData, dataSize);
    for (int i = 0; i < 44100/10; i++)
    {
        float t = float(i) / 44100.0f;
        float f = 440.0f * 6.2831f * t;
        float y = sinf(f);
        int   s = int(32767.0f*y);
        bufferPtr[2 * i + 0] = s;
        bufferPtr[2 * i + 1] = s;
    }

    // Unlock the secondary buffer after the data has been written to it.
    me->mBuffer->Unlock((void*)bufferPtr, bufferSize, NULL, 0);

    return id;
}

void piSoundEngineDS::DelSound( int id )
{
    iSound *me = (iSound*)mSounds.GetAddress(id);
    if (me == nullptr)
        return;

    // TODO
    // destryoy DX resource
}


bool piSoundEngineDS::Play( int id, bool doLoop, float volume )
{
    iSound *me = (iSound*)mSounds.GetAddress(id);
    if (me == nullptr)
        return false;

    //-------------------------

    int v = (int)(10.0f*100.0f * log2f( fmaxf(volume,0.00001f) ));
         if (v>DSBVOLUME_MAX) v = DSBVOLUME_MAX;
    else if (v<DSBVOLUME_MIN) v = DSBVOLUME_MIN;

    // Set position at the beginning of the sound buffer.
    if (FAILED(me->mBuffer->SetCurrentPosition(0)))
        return false;

    //DSEFFECTDESC fx;
    //fx.dwSize = sizeof(DSEFFECTDESC);
    //fx.dwFlags = DSFX_LOCSOFTWARE;
    //fx.guidDSFXClass = GUID_DSFX_STANDARD_CHORUS;// GUID_DSFX_STANDARD_COMPRESSOR, GUID_DSFX_STANDARD_DISTORTION, GUID_DSFX_STANDARD_ECHO, GUID_DSFX_STANDARD_FLANGER, GUID_DSFX_STANDARD_GARGLE, GUID_DSFX_STANDARD_I3DL2REVERB, GUID_DSFX_STANDARD_PARAMEQ, GUID_DSFX_WAVES_REVERB
    //me->mBuffer->SetFX(1, &fx, nullptr);
    //IDirectSoundFXChorus8 ch;
    //me->mBuffer->GetObjectInPath(..., &ch);
    //ch->SetAllParameters(

    //me->mBuffer3D->SetPosition(2.0f, 0.0f, 0.0f, DS3D_IMMEDIATE);
    if (FAILED(me->mBuffer->Play(0, 0, (doLoop) ? DSBPLAY_LOOPING  : 0)))
        return false;

      if (FAILED(me->mBuffer->SetVolume(v)))
        return false;

    return true;
}

bool piSoundEngineDS::Stop(int id)
{
    iSound *me = (iSound*)mSounds.GetAddress(id);
    if (me == nullptr)
        return false;

    if (FAILED(me->mBuffer->Stop()))// DSBPLAY_LOOPING
        return false;

    return true;
}

bool piSoundEngineDS::SetVolume(int id, float volume)
{
    iSound *me = (iSound*)mSounds.GetAddress(id);
    if (me == nullptr)
        return false;

    //-------------------------

    int v = (int)(10.0f*100.0f * log2f(fmaxf(volume, 0.00001f)));
    if (v>DSBVOLUME_MAX) v = DSBVOLUME_MAX;
    else if (v<DSBVOLUME_MIN) v = DSBVOLUME_MIN;

    if (FAILED(me->mBuffer->SetVolume(v)))
        return false;

    return true;
}

bool piSoundEngineDS::GetIsPlaying(int id)
{
    iSound *me = (iSound*)mSounds.GetAddress(id);
    if (me == nullptr)
        return false;

    DWORD status;
    me->mBuffer->GetStatus(&status);

    return (status&DSBSTATUS_PLAYING) != 0;
}

bool piSoundEngineDS::SetPosition(int id, const float *pos)
{
    iSound *me = (iSound*)mSounds.GetAddress(id);
    if (me == nullptr)
        return false;
#ifdef USE3D
    //DS3DBUFFER info;
    //me->mBuffer3D->GetAllParameters(&info);
    //me->mBuffer3D->SetAllParameters(&info);

    me->mBuffer3D->SetPosition(pos[0], pos[2], pos[1], DS3D_IMMEDIATE);
#endif
    return true;
}


void piSoundEngineDS::SetListener(const float *pos, const float *dir, const float *up)
{
#ifdef USE3D
    m_listener->SetPosition(pos[0], pos[2], pos[1], DS3D_DEFERRED);
    m_listener->SetOrientation(dir[0], dir[2], dir[1], up[0], up[2], up[1], DS3D_DEFERRED);
    m_listener->CommitDeferredSettings();
#endif
}


}
