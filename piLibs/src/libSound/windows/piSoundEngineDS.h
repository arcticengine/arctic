#pragma once

#include <Dsound.h>

#include "../../libDataUtils/piTArray.h"

#include "../piSound.h"

namespace piLibs {

#define USE3D


class piSoundEngineDS : public piSoundEngine
{
public:
    piSoundEngineDS();
    ~piSoundEngineDS();

    bool Init(void *hwnd, int deviceID);
    void Deinit(void);
    int  GetNumDevices(void);
    const wchar_t *GetDeviceName(int id) const;

    int   AddSound( const wchar_t *filename ); // wav file
    int   AddSound( const SampleFormat *fmt, void *buffer );
    bool  Play(int id, bool loop, float volume);
    bool  Stop(int id);
    bool  SetVolume(int id, float volume);
    bool  GetIsPlaying(int id);
    void  DelSound(int id);

    

#ifdef USE3D
    void  SetListener(const float *pos, const float *dir, const float *up);
    bool  SetPosition(int id, const float *pos);
#endif

private:
    static BOOL CALLBACK myDSEnumCallback(LPGUID lpGuid, const wchar_t * lpcstrDescription, const wchar_t * lpcstrModule, void * lpContext);

    int mNumDevices;
    struct DeviceInfo
    {
        GUID     *mGUID;
        wchar_t  *mName;
    }mDevice[16];

    typedef struct
    {
        IDirectSoundBuffer8* mBuffer;
        #ifdef USE3D
        IDirectSound3DBuffer8* mBuffer3D;
        #endif
        int  mID;
    }iSound;

    IDirectSound8* m_DirectSound;
    IDirectSoundBuffer* m_primaryBuffer;
    piTArray<iSound> mSounds;
    IDirectSound3DListener8 *m_listener;

};

} // namespace piLibs
