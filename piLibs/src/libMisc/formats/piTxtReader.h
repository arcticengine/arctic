#pragma once

namespace piLibs {

typedef void * piTxtReader;

//piTxtReader piXmlReader_Open( const wchar_t *filename );
//void        piXmlReader_Close( piTxtReader vme );

char * piTxtReader_ReadAll( const wchar_t *filename, int *len );
bool   piTxtReader_Save( const wchar_t *filename, const char *text, int len );

} // namespace piLibs