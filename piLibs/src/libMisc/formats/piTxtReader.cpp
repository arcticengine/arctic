#include <malloc.h>

#include "../../libSystem/piFile.h"
#include "../../libSystem/piStr.h"
#include "../../libSystem/piFilename.h"

namespace piLibs {

char * piTxtReader_ReadAll( const wchar_t *filename, int *outLen )
{
    piFile fp; 
    if( !fp.Open(filename, L"rb") )
	    return nullptr;

    const int len = (int)fp.GetLength();

	char *buffer = (char*)malloc( len+1 );
	if( !buffer ) { fp.Close(); return 0; };

    fp.Read( (void*)buffer, len);
	buffer[len] = 0;

	fp.Close();

    if( outLen ) outLen[0] = len;

	return buffer;
}

bool piTxtReader_Save( const wchar_t *filename, const char *text, int len )
{
    piFile fp;
    if (!fp.Open(filename, L"wb"))
        return false;

    fp.Write( (void*)text, len);

	fp.Close();

	return true;
}

}