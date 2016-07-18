#ifndef _PIMIDIIN_
#define _PIMIDIIN_

typedef void *piMidiIn;

//typedef void (*piMidiInEventCallback)( void *opaque, int type, int chan, int ctrl, int value );


int      piMidiIn_GetNumInputDevices( void );
bool     piMidiIn_GetInputDeviceInfo( int id, wchar_t *name );
piMidiIn piMidiIn_Create( int id );
void     piMidiIn_Destroy( piMidiIn me );
bool     piMidiIn_GetController( piMidiIn me, int chan, int ctrl, int *result );

#endif