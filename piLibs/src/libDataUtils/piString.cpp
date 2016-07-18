#ifdef WINDOWS
#include <windows.h>
#endif
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <wchar.h>
#include "piString.h"
#include "../libSystem/piStr.h"

namespace piLibs {

piString::piString()
{
    mBuffer = nullptr;
    mMax = 0;
    mNum = 0;
}

piString::~piString()
{
}


int piString::Init( unsigned long max )
{
    if( max<0 )
        return( 0 );

    mBuffer = (wchar_t*)malloc( (max+1)*sizeof(wchar_t) );
    if( !mBuffer )
        return( 0 );

    mBuffer[0] = 0;
    mMax = max;
    mNum = 0;

    return( 1 );
}

bool piString::IsNull(void) const
{
    return mNum==0;
}

void piString::SetNull( void )
{
    mNum = 0;
}

void piString::End( void )
{
    if( mBuffer != nullptr ) free( mBuffer );
    mBuffer = nullptr;
	mMax = 0;
	mNum = 0;
}

int piString::Copy( const piString *ori )
{
    const unsigned int len = ori->mNum;
	if( !Init( ori->mMax ) )
		return( 0 );
	mNum = ori->mNum;
	memcpy( mBuffer, ori->mBuffer, len*sizeof(wchar_t) );
	mBuffer[len] = 0;
	return( 1 );
}

int piString::CopyS( const char *ori )
{
    const int len = (int)strlen( ori );

	mNum = len;

    #ifdef WINDOWS
    if( !MultiByteToWideChar(CP_ACP,0,ori,len,mBuffer,len) )
        return 0;
    #endif

    mBuffer[len] = 0;

	return( 1 );
}

int piString::InitCopyS( const char *ori, unsigned int numChars )
{
	if( !ori ) { mNum=0; mBuffer=nullptr; mMax=0; return 1; }
 
	const unsigned int len = numChars;

	if( !Init( len ) )
		return( 0 );
 	mNum = len;

    #ifdef WINDOWS
    if( !MultiByteToWideChar(CP_ACP,0,ori,len,mBuffer,len) )
        return 0;
    #endif
    mBuffer[len] = 0;
 return( 1 );
}

int piString::InitCopy( const piString *ori )
{
    if( !ori ) { mNum=0; mBuffer=nullptr; mMax=0; return 1; }
    const unsigned int len = ori->mNum;
    if( !Init( len ) )
	    return( 0 );
	mNum = len;
	memcpy( mBuffer, ori->mBuffer, len*sizeof(wchar_t) );
	mBuffer[len] = 0;
	return( 1 );
}

int piString::InitCopyW( const wchar_t *ori )
{
    if( !ori ) { mNum=0; mBuffer=nullptr; mMax=0; return 1; }

    const unsigned int len = piwstrlen( ori );
    if( !Init( len ) )
	    return( 0 );
	mNum = len;
    if( len>0 )
	memcpy( mBuffer, ori, len*sizeof(wchar_t) );
	mBuffer[len] = 0;
	return( 1 );
}

int piString::InitConcatW( const wchar_t *ori1, const wchar_t *ori2 )
{
    const unsigned int len1 = piwstrlen( ori1 );
    const unsigned int len2 = piwstrlen( ori2 );
    if( !Init( len1+len2 ) )
	    return( 0 );
	mNum = len1+len2;
	memcpy( mBuffer,      ori1, len1*sizeof(wchar_t) );
	memcpy( mBuffer+len1, ori2, len2*sizeof(wchar_t) );

	mBuffer[len1+len2] = 0;
	return( 1 );
}

int piString::InitConcatW( const wchar_t *ori1, const wchar_t *ori2, const wchar_t *ori3 )
{
    const unsigned int len1 = piwstrlen( ori1 );
    const unsigned int len2 = piwstrlen( ori2 );
    const unsigned int len3 = piwstrlen( ori3 );
    const unsigned int lent = len1+len2+len3;

    if( !Init( lent ) )
	    return( 0 );
	mNum = lent;
	memcpy( mBuffer,           ori1, len1*sizeof(wchar_t) );
	memcpy( mBuffer+len1,      ori2, len2*sizeof(wchar_t) );
	memcpy( mBuffer+len1+len2, ori3, len3*sizeof(wchar_t) );

	mBuffer[lent] = 0;
	return( 1 );
}

int piString::InitCopyS( const char *ori )
{
    if( !ori ) { mNum=0; mBuffer=0; mMax=0; return 1; }

    const unsigned int len = (unsigned int)strlen( ori );

    if( !Init( len ) )
	    return( 0 );
	mNum = len;

    #ifdef WINDOWS
    if( !MultiByteToWideChar(CP_ACP,0,ori,len,mBuffer,len) )
        return 0;
    #endif

	mBuffer[len] = 0;

	return 1;
}

int piString::CopyW( const wchar_t *ori )
{
    const unsigned int len = piwstrlen( ori );
    if( len>mMax )
        return 0;

	mNum = len;
	memcpy( mBuffer, ori, len*sizeof(wchar_t) );
	mBuffer[len] = 0;
	return( 1 );
}

int piString::WrapW( const wchar_t *ori )
{
    if( ori==0 )
    {
	    mMax = 0;
	    mNum = 0;
        mBuffer = 0;
    }   
    else
    {
        const unsigned int len = piwstrlen( ori );
	    mMax = len;
	    mNum = len;
        mBuffer = (wchar_t*)ori;
    }
	return( 1 );
}

int piString::GetLength( void ) const
{
    return( mNum );
}

void piString::SetLength( unsigned int len )
{
	mNum = len;
	mBuffer[len] = 0;
}


int piString::iReallocate(void)
{
	long newmax = 4*mMax/3;
	if( newmax<4 ) newmax = 4;

	mBuffer = (wchar_t*)realloc( mBuffer, (1+newmax)*sizeof(wchar_t) );
	if( !mBuffer )
		return( 0 );

	memset( mBuffer+mMax, 0, (newmax-mMax) );
	mMax = newmax;

	return 1;
}

int piString::AppendWC( wchar_t obj )
{
    if( mNum>=(mMax-1) )
    {
        if (!iReallocate())
			return 0;
    }

    mBuffer[ mNum++ ] = obj;
    mBuffer[ mNum ] = 0;

    return( 1 );
}

int piString::AppendAC( char cobj )
{
    wchar_t obj = cobj;
    if( mNum>=(mMax-1) )
    {
        if (!iReallocate())
			return 0;
    }

    mBuffer[ mNum++ ] = obj;
    mBuffer[ mNum ] = 0;

    return( 1 );
}


int piString::AppendCAt( wchar_t obj, unsigned int pos )
{
	if( pos>=mNum )
		return AppendWC(obj);

	const unsigned int len = mNum;
    if( len>=(mMax-1) )
	{
        if (!iReallocate())
			return 0;
	}

	for( unsigned int i=len; i>pos; i-- )
	{
		mBuffer[i] = mBuffer[i-1];
	}
	mBuffer[pos] = obj;
	mBuffer[len+1] = 0;
	mNum++;

	return 1;
}

int piString::RemoveCAt( unsigned int pos )
{
	const unsigned int len = mNum;
	if( len<1 ) return 1;
	for( unsigned int i=pos; i<len; i++ )
	{
		mBuffer[i] = mBuffer[i+1];
	}
	mBuffer[len-1] = 0;
	mNum--;

	return 1;
}

int piString::AppendS( const piString *str )
{
    const unsigned int len = str->mNum;
    for( unsigned int i=0; i<len; i++ )
        if( !AppendWC(str->mBuffer[i]) )
            return( 0 );
    return( 1 );
}

int piString::AppendSW( const wchar_t *str )
{
    const unsigned int l = piwstrlen( str );
    for( unsigned int i=0; i<l; i++ )
        if( !AppendWC(str[i]) )
            return( 0 );
    return( 1 );
}



int piString::AppendSA(const char *str)
{
    const int len = (int)strlen(str);

#ifdef WINDOWS
    int n = MultiByteToWideChar(CP_ACP, 0, str, len, 0, 0);
    if (n<1) return 0;

    wchar_t *tmp = (wchar_t*)malloc((n + 1)*sizeof(wchar_t));
    if (!tmp) return 0;

    MultiByteToWideChar(CP_ACP, 0, str, len, tmp, n);

    for( int i = 0; i<n; i++ )
        if (!AppendWC(tmp[i]))
            return 0;

    free( tmp );

    return 1;

#else
    return 0;
#endif
}


int piString::EqualW( const wchar_t *str ) const
{
    const unsigned int len = piwstrlen( str );
	if( len != mNum ) return 0;
    for( unsigned int i=0; i<len; i++ )
        if( mBuffer[i] != str[i] )
            return 0;
    return 1;
}

int piString::Equal( const piString *str ) const
{
    const unsigned int len = str->mNum;
	if( len != mNum ) return 0;
    for( unsigned int i=0; i<len; i++ )
        if( mBuffer[i] != str->mBuffer[i] )
            return 0;
    return 1;

}

void piString::Reset( void )
{
    mNum = 0;
}


const wchar_t piString::GetC(const unsigned int n) const
{
    if( n<0 ) return 0;
    if( n>=mNum ) return( 0 );

    return mBuffer[n];
}


const wchar_t *piString::GetS(void) const
{
    //if( mNum==0 ) return 0;
    if( mBuffer != nullptr )
	    mBuffer[ mNum ] = 0;
    return mBuffer;
}


int piString::AppendPrintf( const wchar_t *format, ... )
{
    wchar_t tmpstr[4096];

    va_list arglist;
    va_start( arglist, format );
    const int res = pivwsprintf( tmpstr, 4095, format, arglist );
	va_end( arglist );

    return AppendSW( tmpstr );

}

int piString::Printf( const wchar_t *format,  ...  )
{
    va_list args;
    va_start( args, format );
    const int res = vswprintf_s( mBuffer, mMax, format, args );
    va_end( args );

	mNum = res;

    return 1;
}

int piString::ToSInt( int *res ) const
{
    wchar_t *ec;
    res[0] = wcstoul( mBuffer, &ec, 10 );
    if( ec[0]!=0 )
        return 0;
    return 1;
}

int piString::ToUInt( unsigned int *res ) const
{
    wchar_t *ec;
    res[0] = wcstoul( mBuffer, &ec, 10 );
    if( ec[0]!=0 )
        return 0;
    return 1;
}


int piString::ToFloat( float *res ) const
{
    wchar_t *ec;
    res[0] = (float)wcstod( mBuffer, &ec );
    if( ec[0]!=0 )
        return( 0 );

    return 1;    
}

bool piString::ContainsW( const wchar_t *str ) const
{
	return (wcsstr(mBuffer,str)!=NULL );
}


bool piString::RemoveOccurrences(const wchar_t what)
{
    const int num = mNum;
    int wptr = 0;

    for (int i = 0; i<num; i++)
    {
        const wchar_t c = mBuffer[i];
        if (c == what)
        {
        }
        else
        {
            mBuffer[wptr++] = c;
        }
    }
    mBuffer[wptr] = 0;
    mNum = wptr;
      
    return true;
}

bool piString::ReplaceOccurrences(const wchar_t what, const wchar_t bywhat)
{
    const int num = mNum;
    int wptr = 0;

    for( int i=0; i<num; i++ )
    {
        const wchar_t c = mBuffer[i];
        if( c==what )
        {
            mBuffer[wptr++] = bywhat;
        }
        else
        {
            mBuffer[wptr++] = c;
        }
    }
    mBuffer[wptr] = 0;
    mNum = wptr;

    return true;
}


}