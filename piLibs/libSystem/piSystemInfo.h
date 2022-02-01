#pragma once

#include "piTypes.h"

namespace piLibs {


void   piSystemInfo_getFreeRAM( uint64 *avail, uint64 *total );
int    piSystemInfo_getCPUs( void );
void   piSystemInfo_getOS( wchar_t *str, int length );
void   piSystemInfo_getProcessor( wchar_t *str, int length, int *mhz );
void   piSystemInfo_getTime( wchar_t *str, int length );
void   piSystemInfo_getGfxCardIdentification( wchar_t *vendorID, int vlen, wchar_t *deviceID, int dlen);
uint64 piSystemInfo_getVideoMemory( void );
void   piSystemInfo_getScreenResolution( int *res );
int    piSystemInfo_getIntegratedMultitouch( );
int    piSystemInfo_getNumMonitors( void );

} // namespace piLibs