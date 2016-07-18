#pragma once

namespace piLibs {

typedef void * piDLL;

piDLL piDLL_Load( const wchar_t *name );
void *piDLL_GetFunction( piDLL h, const wchar_t *name );
void  piDLL_Free( piDLL h );

} // namespace piLibs