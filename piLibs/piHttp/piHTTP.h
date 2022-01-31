#pragma once

namespace piLibs {


typedef void *piHTTP_Session;
typedef void *piHTTP_Session;

typedef enum
{
    piHHTPMode_GET  = 0,
    piHHTPMode_POST = 1,
}piHTTP_Mode;

typedef struct
{
    char *mBuffer;
    int   mLength;
}piHTTP_RequestResult;

piHTTP_Session piHTTP_Open( const wchar_t *name, const wchar_t *url, int port );
void           piHTTP_Close( piHTTP_Session me );
int            piHTTP_SendRequest( piHTTP_Session me, piHTTP_Mode, const wchar_t *url, piHTTP_RequestResult * result );

//    char szReq[1024] = "";
    //if( !HttpSendRequest(hHttpRequest, szHeaders, piwstrlen(szHeaders), szReq, strlen(szReq))) 


}