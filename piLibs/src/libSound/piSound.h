#pragma once

#include "../libSystem/piTypes.h"

namespace piLibs {

typedef void *piSoundLibMgr;
typedef void *piSoundInput;
typedef void *piSoundOutput;
typedef void *piSoundSynth;

piSoundLibMgr piSoundLib_Create( void );
void       piSoundLib_Destroy( piSoundLibMgr me );
int        piSoundLib_GetNumInputDevices( piSoundLibMgr me );
bool       piSoundLib_GetInputDeviceInfo( piSoundLibMgr me, int id, wchar_t *name );
int        piSoundLib_GetNumOutputDevices( piSoundLibMgr me );
bool       piSoundLib_GetOutputDeviceInfo( piSoundLibMgr me, int id, wchar_t *name );

piSoundInput piSoundInput_Create( piSoundLibMgr me, int deviceID, int capID );
bool		 piSoundInput_Start( piSoundInput me );
bool		 piSoundInput_Stop( piSoundInput me );
bool		 piSoundInput_Destroy( piSoundInput me );
int			 piSoundInput_GetFFT( piSoundInput vme, float *buffer1024 );
int			 piSoundInput_GetWave( piSoundInput vme, float *buffer, int numsamples );


piSoundOutput piSoundOutput_Create( piSoundLibMgr me, int deviceID, int capID );
int           piSoundOutput_LoadMusic( piSoundOutput vme, const char *fila, int isMemory );
int			  piSoundOutput_SetMusicPosition( piSoundOutput vme, unsigned int offset );
unsigned int  piSoundOutput_GetMusicPosition( piSoundOutput vme );
int			  piSoundOuput_GetFFT( piSoundOutput vme, float *buffer1024 );

typedef unsigned long  (IQAPICALL *piSoundSynth_Callback)( void *h, short *buffer, unsigned long length, void *user );

piSoundSynth piSoundSynth_Create( piSoundLibMgr memgr, piSoundSynth_Callback func, void *data );
void         piSoundSynth_Stop( piSoundSynth me );


//====================================================
// new class, to replace all of the above (slowly)

class piSoundEngine
{
public:
    piSoundEngine();
    virtual ~piSoundEngine();

    static piSoundEngine * Create(void);

    virtual bool Init(void *hwnd, int deviceID) = 0;
    virtual void Deinit(void) = 0;
    virtual int  GetNumDevices( void ) = 0;
    virtual const wchar_t *GetDeviceName(int id) const = 0;

    typedef struct       // Has to be mono. Then engine will spacialize
    {
        int mFrequency;  // samples per second (eg, 44100)
        int mPrecision;  // 8, 16 or 24 bits per sample
        int mLength;     // in samples
    }SampleFormat;

    virtual int  AddSound( const wchar_t *filename ) = 0; // wav file
    virtual int  AddSound( const SampleFormat *fmt, void *buffer ) = 0;
    virtual void DelSound(int id) = 0;
    virtual bool Play(int id, bool loop, float volume) = 0;
    virtual bool Stop(int id) = 0;
    virtual bool GetIsPlaying(int id) = 0;
    virtual bool SetVolume(int id, float volume) = 0;

    virtual void  SetListener(const float *pos, const float *dir, const float *up) = 0;
    virtual bool  SetPosition(int id, const float *pos) = 0;

};


} // namespace piLibs
