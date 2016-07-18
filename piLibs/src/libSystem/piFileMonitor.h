#pragma once

namespace piLibs {


typedef void *piFileMonitor;

piFileMonitor piFileMonitor_Init( const wchar_t *filename );
int           piFileMonitor_Changed( piFileMonitor vme, int *res );
void          piFileMonitor_Destroy( piFileMonitor vme );

} // namespace piLibs