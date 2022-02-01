#pragma once

namespace piLibs {


typedef struct
{
    wchar_t mName[128];
}piFileData;


typedef void * piFindFiles;

piFindFiles    piFindFiles_Init( void );
void           piFindFiles_Start( piFindFiles me, const wchar_t *mask, int recursive );
void           piFindFiles_End( piFindFiles me );
int            piFindFiles_GetLength( piFindFiles me );
const piFileData *piFindFiles_Get( piFindFiles me, int i );

} // namespace piLibs