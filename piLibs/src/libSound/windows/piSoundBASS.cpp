#include "bass.h"
#include <malloc.h>
#include "../piSound.h"

namespace piLibs {

typedef struct
{
	int kk;
}iIqSoundLibMgr;

piSoundLibMgr piSoundLib_Create( void )
{
	iIqSoundLibMgr *me = (iIqSoundLibMgr*)malloc( sizeof(iIqSoundLibMgr) );
	if( !me )
		return 0;
	return (piSoundLibMgr)me;
}

void piSoundLib_Destroy( piSoundLibMgr vme )
{
	iIqSoundLibMgr *me = (iIqSoundLibMgr*)vme;
	free( me );
}


int piSoundLib_GetNumInputDevices( piSoundLibMgr me )
{
	int a, count=0;
	BASS_DEVICEINFO info;
	for (a=0; BASS_RecordGetDeviceInfo(a, &info); a++)
		if (info.flags&BASS_DEVICE_ENABLED) // device is enabled
			count++; // count it 
	return count;
}

bool piSoundLib_GetInputDeviceInfo( piSoundLibMgr me, int id, wchar_t *name )
{
	BASS_DEVICEINFO info;
	BASS_RecordGetDeviceInfo(id, &info);
	
	int len = (int)strlen( info.name );
    if( !MultiByteToWideChar(CP_ACP,0,info.name,len,name,len) )
        return 0;
	name[len] = 0;
	return true;
}

int piSoundLib_GetNumOutputDevices( piSoundLibMgr me )
{
	int a, count=0;
	BASS_DEVICEINFO info;
	for (a=0; BASS_GetDeviceInfo(a, &info); a++)
    if (info.flags&BASS_DEVICE_ENABLED) // device is enabled
        count++;
	return count;
}

bool piSoundLib_GetOutputDeviceInfo( piSoundLibMgr me, int id, wchar_t *name )
{
	BASS_DEVICEINFO info;
	BASS_GetDeviceInfo(id, &info);
	
	int len = (int)strlen( info.name );
    if( !MultiByteToWideChar(CP_ACP,0,info.name,len,name,len) )
        return 0;
	name[len] = 0;
	return true;
}

//---------------------------------------------------
typedef struct
{
	int deviceID;
	int inputID;
	HRECORD chan;
}iIqSoundInput;

piSoundInput piSoundInput_Create( piSoundLibMgr memgr, int deviceID, int capID )
{
	iIqSoundInput *me = (iIqSoundInput*)malloc( sizeof(iIqSoundInput) );
	if( !me )
		return 0;

	if( !BASS_RecordInit(deviceID) )
		return 0;

    me->deviceID = deviceID;
	me->inputID = -1;

	for (int c=0;BASS_RecordGetInputName(c);c++) 
    {
		if (!(BASS_RecordGetInput(c,NULL)&BASS_INPUT_OFF)) 
        {
			me->inputID=c;
            break;
		}
    }

    if( me->inputID==-1 )
        return 0;

	return me;
}

static BOOL CALLBACK recordCB( HRECORD handle, const void *buffer, DWORD length, void *user )
{
    return 1;
}


bool piSoundInput_Start( piSoundInput vme )
{
	iIqSoundInput *me = (iIqSoundInput*)vme;

	//data.recordCb = recordCb;
    //if( recordCb )
    //    data.rrecordChan=BASS_RecordStart(44100,2,MAKELONG(0,25),recordingCallback,0);
    //else
    //me->chan=BASS_RecordStart(44100,2,MAKELONG(0,250),recordCB,0);
	//me->chan=BASS_RecordStart(44100,2,MAKELONG(0,250),recordCB,0);
    me->chan=BASS_RecordStart(44100,2,MAKELONG(0,50),recordCB,0);
	if (!me->chan)
		return false;

	BASS_RecordSetInput( -1, 0, 1.0f );
	return true;
}

bool piSoundInput_Stop( piSoundInput vme )
{
	iIqSoundInput *me = (iIqSoundInput*)vme;

	BASS_ChannelStop(me->chan);

	return true;
}

bool piSoundInput_Destroy( piSoundInput vme )
{
	iIqSoundInput *me = (iIqSoundInput*)vme;

	BASS_RecordFree();

	free( me );

	return true;
}

//-------------
int piSoundInput_GetFFT( piSoundInput vme, float *buffer1024 )
{		
	iIqSoundInput *me = (iIqSoundInput*)vme;

	if( BASS_ChannelGetData( me->chan, 0, BASS_DATA_AVAILABLE ) )
        return BASS_ChannelGetData( me->chan, buffer1024, BASS_DATA_FFT2048|BASS_DATA_FLOAT);

    return 0;
}


int piSoundInput_GetWave( piSoundInput vme, float *buffer, int numsamples )
{		
	iIqSoundInput *me = (iIqSoundInput*)vme;

    if( BASS_ChannelGetData( me->chan, 0, BASS_DATA_AVAILABLE ) )
	        return BASS_ChannelGetData( me->chan, buffer, BASS_DATA_FLOAT | numsamples )/4;
    return 0;
}

//==================================================================================

typedef struct
{
	int deviceID;
	int inputID;
	HRECORD chan;
	HSTREAM  stream;
}iIqSoundOutput;

piSoundOutput piSoundOutput_Create( piSoundLibMgr memgr, int deviceID, int capID )
{
	iIqSoundOutput *me = (iIqSoundOutput*)malloc( sizeof(iIqSoundOutput) );
	if( !me )
		return 0;

	if( !BASS_Init(-1,44100,0,0,NULL) )
		return 0;

    me->deviceID = deviceID;
	me->inputID = -1;
/*
	for (int c=0;BASS_GetOutputName(c);c++) 
    {
		if (!(BASS_GetDeviceInfo(c,NULL)&BASS_INPUT_OFF)) 
        {
			me->inputID=c;
            break;
		}
    }

    if( me->inputID==-1 )
        return 0;
*/
	return me;
}


int piSoundOutput_LoadMusic( piSoundOutput vme, const char *file, int isMemory )
{
	iIqSoundOutput *me = (iIqSoundOutput*)vme;

	//HMUSIC mod = BASS_MusicLoad( isMemory, file, 0, 0, BASS_MUSIC_RAMPS, 0 );
	me->stream = BASS_StreamCreateFile(isMemory,file,0,0,0);

	if( !me->stream )
		return 0;

	if( !BASS_ChannelPlay(me->stream,FALSE) )
		return 0;

	return 1;
}

int piSoundOutput_SetMusicPosition( piSoundOutput vme, unsigned int offset )
{
	iIqSoundOutput *me = (iIqSoundOutput*)vme;

	QWORD qpos = offset;
	return BASS_ChannelSetPosition(me->stream, qpos, BASS_POS_BYTE);

}

unsigned int piSoundOutput_GetMusicPosition( piSoundOutput vme )
{
	iIqSoundOutput *me = (iIqSoundOutput*)vme;
    return (unsigned int)BASS_ChannelGetPosition( me->stream, BASS_POS_BYTE );
}

int piSoundOuput_GetFFT( piSoundOutput vme, float *buffer1024 )
{		
	iIqSoundOutput *me = (iIqSoundOutput*)vme;

	if( BASS_ChannelGetData( me->stream, 0, BASS_DATA_AVAILABLE ) )
        return BASS_ChannelGetData( me->stream, buffer1024, BASS_DATA_FFT4096|BASS_DATA_FLOAT );//|BASS_DATA_FFT_NOWINDOW);
    return 0;
}

//==================================================================================

typedef struct
{
	//int deviceID;
	//int inputID;
	//HRECORD chan;
	HSTREAM  stream;
}iIqSoundSynth;


piSoundSynth piSoundSynth_Create( piSoundLibMgr memgr, piSoundSynth_Callback func, void *data )
{
	iIqSoundSynth *me = (iIqSoundSynth*)malloc( sizeof(iIqSoundSynth) );
	if( !me )
		return NULL;

	// 10ms update period
	int period = 10;
	BASS_SetConfig( BASS_CONFIG_UPDATEPERIOD, period );

	// setup output - get latency
	if( !BASS_Init(-1,44100,BASS_DEVICE_LATENCY,0,NULL))
		return( 0 );
    BASS_INFO info;
	BASS_GetInfo( &info );

	// default buffer size = update period + 'minbuf'
	int buflen = BASS_SetConfig(BASS_CONFIG_BUFFER,period+info.minbuf);

	// create a stream, stereo so that effects sound nice
	me->stream = BASS_StreamCreate(44100,2,0,(STREAMPROC*)func,data);//WriteStream,me);

#if 1
    HFX fx = BASS_ChannelSetFX(me->stream,BASS_FX_DX8_REVERB,0);
    BASS_DX8_REVERB p;
    BASS_FXGetParameters(fx,&p);
	p.fReverbMix=-20.0f;
    p.fReverbTime=500.0f;
	BASS_FXSetParameters(fx,&p);
#endif
	BASS_ChannelPlay(me->stream,FALSE);

    return me;       
}


void piSoundSynth_Stop( piSoundSynth me )
{
	BASS_Free();
}

}