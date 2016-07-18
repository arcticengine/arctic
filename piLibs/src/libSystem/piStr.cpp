#ifdef WINDOWS
#include <windows.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

namespace piLibs {

char *pistrtok( char *strToken, const char *strDelimit, char **context )
{
    #ifdef WINDOWS
        #if _MSC_VER<1300
        return strtok( strToken, strDelimit );
        #else
        return strtok_s( strToken, strDelimit, context );
        #endif
    #else
    return strtok( strToken, strDelimit );
    #endif

#if 0
    const char *sep;
    char *start;
    char *string;

   
    if (_string!=0) string = _string; else string = *temp;

	if( _string==0 && *temp==0 ) return 0;

    start = string;
   
    while ((*string)!=0)
    {
        sep = _sep;
        while ((*sep)!=0)
        {
            if ((*string)==(*sep))
            {
                *string=0;
                *temp=string+1;
                return(start);
            }
        sep++;
        }
        string++;
    }
    *temp=0;
    return(start);
#endif

}

int pistrlen( const char *str )
{
    return (int)strlen( str );
}

int pistrcmp( const char *stra, const char *strb )
{
    return strcmp(stra,strb);
}

void pistrncpy( char *strDest, size_t sizeInBytes, const char *strSource, size_t count )
{
    #ifdef WINDOWS
        #if _MSC_VER<1300
        strncpy( strDest, strSource, sizeInBytes );
        #else
        if( count==0 ) count = _TRUNCATE;
        strncpy_s( strDest, sizeInBytes, strSource, count );
        #endif
    #else
    strncpy( strDest, strSource, sizeInBytes );
    #endif
}

void pistrcpy( char *strDestination, size_t sizeInBytes, const char *strSource )
{
    #ifdef WINDOWS
        #if _MSC_VER<1300   
        strcpy( strDestination, strSource ); 
        #else
        strcpy_s( strDestination, sizeInBytes, strSource ); 
        #endif
    #else
    strcpy( strDestination, strSource ); 
    #endif
}


void pistrncat( char *strDest, size_t bufferSizeInBytes, const char *strSource, size_t count )
{
    #ifdef WINDOWS
        #if _MSC_VER<1300   
        strncat( strDest, strSource, bufferSizeInBytes );
        #else
        if( count==0 ) count = _TRUNCATE;
        strncat_s( strDest, bufferSizeInBytes, strSource, count );
        #endif
    #else
    strncat( strDest, strSource, bufferSizeInBytes );
    #endif
}

char *pistrconcat( const char *stra, const char *strb )
{
    const int lena = (int)strlen( stra );
    const int lenb = (int)strlen(strb);
    char *ptr = (char*)malloc( lena + lenb + 1 );
    if( !ptr ) return NULL;
    strcpy( ptr, stra );
    strcat( ptr, strb );
    return ptr;
}


int pisprintf( char *buffer, size_t sizeOfBuffer, const char *format,  ...  )
{
    int res;

    va_list marker;

    va_start( marker, format );

    #ifdef WINDOWS
        #if _MSC_VER<1300   
        res = vsprintf( buffer, format, marker );
        #else
        res = vsprintf_s( buffer, sizeOfBuffer, format, marker );
        #endif
    #else
    res = vsprintf( buffer, format, marker );
    #endif

    va_end( marker );

    return res;
}

int pivsprintf( char *buffer, size_t sizeInBytes, const char *format, va_list arglist )
{
    int res;

    #ifdef WINDOWS
        #if _MSC_VER<1300   
        res = vsprintf( buffer, format, arglist );
        #else
        res = vsprintf_s( buffer, sizeInBytes, format, arglist );
        #endif
    #else
    res = vsprintf( buffer, format, arglist );
    #endif

    return res;
}

wchar_t *piwstrtok( wchar_t *strToken, const wchar_t *strDelimit, wchar_t **context )
{
    #ifdef WINDOWS
    return wcstok_s( strToken, strDelimit, context );
    #else
    return 0;
    #endif
}

int piwsprintf( wchar_t *buffer, size_t sizeOfBuffer, const wchar_t *format,  ...  )
{
    va_list args;
    va_start( args, format );
    const int res = vswprintf_s( buffer, sizeOfBuffer, format, args );
    va_end( args );

    return res;
}

int pivwsprintf( wchar_t *buffer, size_t sizeInBytes, const wchar_t *format, va_list arglist )
{
    int res;

    #ifdef WINDOWS
        #if _MSC_VER<1300   
        res = vsprintf( buffer, format, arglist );
        #else
        res = vswprintf_s( buffer, sizeInBytes, format, arglist );
        #endif
    #else
    res = vsprintf( buffer, format, arglist );
    #endif

    return res;

}

int pivscwprintf(const wchar_t *format, va_list arglist)
{
	return _vscwprintf(format, arglist);
}

void piwstrcat( wchar_t *strDest, size_t bufferSizeInBytes, const wchar_t *strSource )
{
    #ifdef WINDOWS
        wcscat_s( strDest, bufferSizeInBytes, strSource );
    #else
    #endif
}

const wchar_t *piwcsrchr( const wchar_t *str, wchar_t c )
{
    #ifdef WINDOWS
        return wcsrchr( str, c );
    #else
    #endif
}

void piwstrcpy( wchar_t *strDestination, size_t sizeInBytes, const wchar_t *strSource )
{
    #ifdef WINDOWS
    wcscpy( strDestination, strSource );
    #else
    #endif
}

int piwstrlen( const wchar_t *buffer )
{
    #ifdef WINDOWS
    return (int)wcslen( buffer );
    #else
    #endif
}

int piwstrcmp( const wchar_t *a, const wchar_t *b )
{
    #ifdef WINDOWS
    return wcscmp(a,b);
    #else
    #endif
}

void piwstrncpy( wchar_t *strDest, size_t sizeInBytes, const wchar_t *strSource, size_t count )
{
    #ifdef WINDOWS
    if( count==0 ) count = piwstrlen( strSource );
    wcsncpy( strDest, strSource, count );
    #else
    #endif
}


void piwstrncat( wchar_t *strDest, size_t bufferSizeInBytes, const wchar_t *strSource, size_t count )
{
    #ifdef WINDOWS
    wcsncat( strDest, strSource, count );
    #else
    #endif
}

int piwstr2int( const wchar_t *str, bool *status )
{
    #ifdef WINDOWS
    //int res = _wtoi( str );
    wchar_t *ec;
    int res = wcstoul( str, &ec, 10 );
    if( ec[0]!=0 )
        *status = false;
    else
        *status = true;
    #else
    #endif
    return res;
}

int piwstrcspn( const wchar_t *str, const wchar_t *charSet )
{
    #ifdef WINDOWS
    return (int)wcscspn( str, charSet  );
    #else
    return 0;
    #endif
}

wchar_t *piwstrdup( const wchar_t *str, int *err )
{
    if( str==0 ) {err[0] = 0; return 0; }
    // get length
    int len; for( len=0; str[len]; len++ ) {}
    // alloc
    wchar_t *res = (wchar_t*)malloc( (1+len)*sizeof(wchar_t) );
    if( !res ) { err[0] = 1; return 0; }
    // copy
    for( int i=0; i<len; i++ ) res[i] = str[i]; res[len] = 0;

    err[0] = 0;
    return res;
}


bool piwstrequ( const wchar_t *a, const wchar_t *b )
{
    if( a==0 || b==0 ) return false;

    for(int i=0; ; i++ )
    {
        if( a[i]!=b[i] ) return false;
        if( a[i]==0 ) return true;
        if( b[i]==0 ) return true;
    }
    return true;
}

wchar_t * pistr2ws(const char *ori)
{
    const int len = (int)strlen(ori);

#ifdef WINDOWS
    int n = MultiByteToWideChar(CP_ACP, 0, ori, len, 0, 0 );
    if( n<1 ) return NULL;

    wchar_t *dst = (wchar_t*)malloc( (n+1)*sizeof(wchar_t) );
    if( !dst ) return 0;

    MultiByteToWideChar(CP_ACP, 0, ori, len, dst, n);
    dst[n] = 0;

    return dst;
#else
    return 0;
#endif
}


char* piws2str(const wchar_t *ori)
{
    int num = piwstrlen(ori);
    char *res = (char*)malloc( num+1 );
    if( !res ) return nullptr;
    int l = (int)wcstombs(res, ori, num);
    res[l] = 0;
    return res;
}

}