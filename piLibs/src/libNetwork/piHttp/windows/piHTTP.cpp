#include <windows.h>
#include <wininet.h>
#pragma comment(lib,"wininet.lib")
#include <malloc.h>
#include <string.h>

#include "../piHTTP.h"

namespace piLibs {

typedef struct
{
    HINTERNET hIntSession;
    HINTERNET hHttpSession;
}iSession;

piHTTP_Session piHTTP_Open( const wchar_t *name, const wchar_t *url, int port )
{
    iSession *me = (iSession*)malloc( sizeof(iSession) );
    if( !me )
        return NULL;

    me->hIntSession = InternetOpen( L"MyApp", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

    me->hHttpSession = InternetConnect(me->hIntSession, url, port, 0, 0, INTERNET_SERVICE_HTTP, 0, NULL);

    return (piHTTP_Session)me;
}

void piHTTP_Close( piHTTP_Session vme )
{
    iSession *me = (iSession*)vme;
    InternetCloseHandle(me->hHttpSession);
    InternetCloseHandle(me->hIntSession);
}

int piHTTP_SendRequest( piHTTP_Session vme, piHTTP_Mode mode, const wchar_t *url, piHTTP_RequestResult * result )
{
    iSession *me = (iSession*)vme;

    const wchar_t *method = (mode==piHHTPMode_GET) ? L"GET" : L"POST";

    HINTERNET hHttpRequest = HttpOpenRequest( me->hHttpSession, method,  url, 0, 0, 0, INTERNET_FLAG_RELOAD, 0);

    const wchar_t * szHeaders = L"Content-Type: text/html\nMySpecialHeder: whatever";
    char szReq[1024] = "";
    if (!HttpSendRequest(hHttpRequest, szHeaders, (int)wcslen(szHeaders), szReq, (int)strlen(szReq)))
    {
      int dwErr = GetLastError();
      /// handle error
      return 0;
    }

    char tmp[1024];
    const int tmpLen = sizeof(tmp);

    int     mMax = 0;
    result->mLength = 0;
    result->mBuffer = 0;

    DWORD dwRead = 0;
    while( InternetReadFile(hHttpRequest, tmp, tmpLen, &dwRead) && dwRead ) 
    {
      if( result->mLength + (int)dwRead > mMax )
      {
          const int ps1 = result->mLength + dwRead;
          const int ps2 = result->mLength*3/4;
          mMax = max( ps1, ps2 );
          result->mBuffer = (char*)realloc( result->mBuffer, mMax );
          if( !result->mBuffer )
                return 0;
      }

      memcpy( result->mBuffer + result->mLength, tmp, dwRead );
      result->mLength += dwRead;

      dwRead = 0;
    }

    InternetCloseHandle(hHttpRequest);

    return 1;
}

}