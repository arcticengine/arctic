#pragma once

namespace piLibs {

// return a new string with the path of the file
wchar_t *piFileName_ExtractPath( const wchar_t *file );

// return a new string with the name of the file without the path
wchar_t *piFileName_ExtractName( const wchar_t *file );

wchar_t *piFileName_ExtractNameWithoutExtension( const wchar_t *file );

// return a pointer to the dot of the extension
wchar_t *piFileName_GetExtension( wchar_t *file );

} // namespace piLibs
