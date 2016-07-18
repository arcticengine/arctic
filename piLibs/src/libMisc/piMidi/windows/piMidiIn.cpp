#include <windows.h>
#include <string.h>
#include "../piMidiIn.h"

typedef struct
{
    HMIDIIN mHandle;
    void    *mOpaque;
//    piMidiInEventCallback mEventCB;

	unsigned char mControllers[128*16];
}iIqMidiIn;



void CALLBACK MyMidiInProc( HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) 
{
    iIqMidiIn *me = (iIqMidiIn*)dwInstance;

    switch (wMsg) 
    {
        case MM_MIM_OPEN:
            break;
        case MM_MIM_CLOSE:
            break;
        case MM_MIM_DATA:
		{

            //me->mEventCB( me->mOpaque, (dwParam1>>4) & 0x0f, dwParam1 & 0x0f, (dwParam1>>8) & 0xff, (dwParam1>>16) & 0xff );
            const int midiMsg  = (dwParam1>>4) & 0x0f;
			const int midiChan = dwParam1 & 0x0f;
            const int midiCtrl = (dwParam1>>8) & 0x7f;
            const int midiVal  = (dwParam1>>16) & 0x7f;

			// controller change
			if( midiMsg==11 ) 
			{
				//if( midiVal != me->mControllers[midiCtrl] )
				me->mControllers[midiCtrl+128*midiChan] = 128 + midiVal;
			}
			// controller change
			if( midiMsg==9 ) 
			{
				me->mControllers[midiCtrl+128*midiChan] = 128 + midiVal;
			}
			if( midiMsg==8 ) 
			{
				me->mControllers[midiCtrl+128*midiChan] = 128 + 0;
			}

            break;
		}
        default:
            break;
    }
}


/*
    int n = midiInGetNumDevs();
    for (int i = 0; i < n; i++) 
    {
        MIDIINCAPS caps;
        midiInGetDevCaps(i,&caps,sizeof(MIDIINCAPS));
        gLog->Printf(LT_EXTRA_PARAMS, LT_MESSAGE, L"%d - %d - %d - %08X - %s\n",i,caps.wMid,caps.wPid,caps.vDriverVersion,caps.szPname);
    }
*/
  

int piMidiIn_GetNumInputDevices( void )
{
    return midiInGetNumDevs();
}

bool piMidiIn_GetInputDeviceInfo( int id, wchar_t *name )
{
	MIDIINCAPS a;
    midiInGetDevCaps( id, &a, sizeof(MIDIINCAPS) );
    for( int i=0; i<32; i++ )
		name[i] = a.szPname[i];
    return true;
}


piMidiIn piMidiIn_Create( int id )
{
    iIqMidiIn *me = (iIqMidiIn*)malloc( sizeof(iIqMidiIn) );
    if( !me ) return 0;


    if( midiInOpen(&me->mHandle,id,(DWORD)MyMidiInProc,(DWORD_PTR)me,CALLBACK_FUNCTION) != MMSYSERR_NOERROR )
    {
        me->mHandle = 0;
        return false;
    }

	memset( me->mControllers, 0, 128*16 );

    midiInStart(me->mHandle);

    return me;
}

void piMidiIn_Destroy( piMidiIn vme )
{
    iIqMidiIn *me = (iIqMidiIn*)vme;

    if( me->mHandle )
    {
        midiInStop(me->mHandle);
        midiInClose(me->mHandle);
    }
}

bool piMidiIn_GetController( piMidiIn vme, int chan, int ctrl, int *result )
{
    iIqMidiIn *me = (iIqMidiIn*)vme;

	if( chan<0 )
	{
        for( int c=0; c<16; c++ )
		{
			int  v =  me->mControllers[ctrl+128*c] & 127;
			if( (me->mControllers[ctrl+128*c] & 128) !=0 )
			{
				me->mControllers[ctrl+128*c] = v;
				result[0] = v;
				return true;
			}
		}

		return false;
	}
	else
	{
		int  v =  me->mControllers[ctrl+128*chan] & 127;
		bool r = (me->mControllers[ctrl+128*chan] & 128) !=0;

		me->mControllers[ctrl+128*chan] = v;
		result[0] = v;

		return r;
	}

}
/*
bool piMidiIn_GetButton( piMidiIn vme, int ctrl, bool *result )
{
    iIqMidiIn *me = (iIqMidiIn*)vme;

	int  v =  me->mButtons[ctrl] & 127;
	bool r = (me->mButtons[ctrl] & 128) !=0;

    me->mButtons[ ctrl ] = v;
	result[0] = (v!=0);

    return r;
}*/

