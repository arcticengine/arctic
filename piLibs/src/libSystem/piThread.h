#pragma once

namespace piLibs {

typedef void *piThread;

typedef int (*piThreadDoworFunc)( void *data );

piThread piThread_Init( piThreadDoworFunc func, void *data );
void     piThread_End( piThread me );
void    *piThread_GetOSID( void );

} // namespace piLibs