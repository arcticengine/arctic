#pragma once

namespace piLibs {

typedef void *piDynamicLibrary;

piDynamicLibrary piDynamicLibrary_Load( const wchar_t *name );
void            *piDynamicLibrary_GetFunction( piDynamicLibrary me, const char *name );
void             piDynamicLibrary_Unload( piDynamicLibrary me );

} // namespace piLibs