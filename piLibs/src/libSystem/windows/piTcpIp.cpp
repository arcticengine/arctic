#include <windows.h>
#include <malloc.h>
#include <string.h>

#include "../piStr.h"
#include "../piTcpIp.h"

#pragma comment(lib,"ws2_32.lib")


//int i = 1;
//setsockopt(iSock, IPPROTO_TCP, TCP_NODELAY, (void *)&i, sizeof(i));
//I also found running the socket in a separate thread at a high frequency also helps a lot for snappy response

namespace piLibs {


static WSADATA wsaData;
static int     status =0;

typedef struct
{
    SOCKET pSocket;
    struct sockaddr_in  pMyAddr;    
    struct sockaddr_in  pHisAddr;    
}ITCPSocket;


int piTcpIp_Init( void )
{
    if( status==0 )
    {
        if( WSAStartup(MAKEWORD(1,1), &wsaData) )
            return( 0 );
        status = 1;
    }

    return( 1 );
}

void piTcpIp_End( void )
{
    if( status==1 )
    {
        WSACleanup();
        status = 2;
    }
}


static int TCPIP_GetAddress( struct in_addr *adr )
{
    char strHost[64];
    if( gethostname(strHost,64)==SOCKET_ERROR )
        return 0;

    struct hostent *ho = gethostbyname( strHost );

    memcpy( adr, ho->h_addr,ho->h_length );

    return( 1 );
}


int piTcpIp_Receive( piTcpIpSocket *vme, char *data, int len )
{ 
    ITCPSocket *me = (ITCPSocket*)vme->internalData;

    int n = recv( me->pSocket, (char*)data, len, 0 );


    if( n==SOCKET_ERROR )
    {
        int err = WSAGetLastError();
        if( err==WSAECONNRESET ) 
            return -1;
        else
            return -2;
    }

    return n;
}
 

int piTcpIp_ReceiveExact( piTcpIpSocket *vme, char *data, int len )
{    
    ITCPSocket *me = (ITCPSocket*)vme->internalData;

    int rec = 0;

    while( rec<len )
    {
        int n = recv( me->pSocket, data+rec, len-rec, 0 );

        if( n==0 )
            return -3;

        if( n==SOCKET_ERROR )
        {
            int err = WSAGetLastError();
            if( err==WSAECONNRESET ) 
                return -1;
            else
                return -2;
        }

        rec += n;
    }

    return rec;
}

int piTcpIp_Send( piTcpIpSocket *vme, const char *data, int len )
{ 
    ITCPSocket *me = (ITCPSocket*)vme->internalData;

    int res = send( me->pSocket, (char*)data, len, 0 );
    if( res==SOCKET_ERROR )
        return -1;
    return res;
}


void piTcpIp_Close( piTcpIpSocket *vme )
{
    ITCPSocket *me = (ITCPSocket*)vme->internalData;

    closesocket( me->pSocket );
}


//-----------------------
// struct sockaddr_in      pServerAdr;


int piTcpIp_CreateAndConnectByURL( piTcpIpSocket *vme, const char *address, int port, bool disableBuffering )
{
    ITCPSocket *me = (ITCPSocket*)vme->internalData;


    me->pSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // IPPROTO_UDP
    if( me->pSocket == INVALID_SOCKET )
        return( 0 );

/*
    me->pMyAddr.sin_family = AF_INET;
    me->pMyAddr.sin_port = port;
    if( !TCPIP_GetAddress( &me->pMyAddr.sin_addr ) )
        return( 0 );
*/
    me->pHisAddr.sin_family = AF_INET;
    me->pHisAddr.sin_port = htons( port );
  
    struct hostent *ho = gethostbyname( (char*)address );
    if( !ho )
        return 0;

    memcpy( &me->pHisAddr.sin_addr, ho->h_addr,ho->h_length );
    
/*
    // by IP address
    me->pHisAddr.sin_addr.S_un.S_un_b.s_b1 = address[0];
    me->pHisAddr.sin_addr.S_un.S_un_b.s_b2 = address[1];
    me->pHisAddr.sin_addr.S_un.S_un_b.s_b3 = address[2];
    me->pHisAddr.sin_addr.S_un.S_un_b.s_b4 = address[3];
*/

    if (disableBuffering )
    {
        int i = 1;
        setsockopt(me->pSocket, IPPROTO_TCP, TCP_NODELAY, (const char *)&i, sizeof(i));
    }

    if( connect(me->pSocket, (struct sockaddr *)&me->pHisAddr, sizeof(sockaddr_in))==SOCKET_ERROR )
        return( 0 );

    return( 1 );
}


int piTcpIp_CreateAndListen(piTcpIpSocket *vme, int port, bool disableBuffering)
{
    ITCPSocket *me = (ITCPSocket*)vme->internalData;

    me->pMyAddr.sin_family = AF_INET;
    me->pMyAddr.sin_port = htons( port );
    if( !TCPIP_GetAddress( &me->pMyAddr.sin_addr ) )
        return( 0 );

    

    me->pSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( me->pSocket==0 )
        return( 0 );
 
    if (disableBuffering)
    {
        int i = 1;
        setsockopt(me->pSocket, IPPROTO_TCP, TCP_NODELAY, (const char *)&i, sizeof(i));
    }

    if( bind( me->pSocket, (const struct sockaddr *)&me->pMyAddr, sizeof(sockaddr_in) ) == SOCKET_ERROR )
        return( 0 );
 
    if( listen( me->pSocket, SOMAXCONN )==SOCKET_ERROR  )
        return( 0 );
 
    /*
    piLog_Printf( LT_EXTRA_PARAMS, LT_MESSAGE, L"Server installed at [%d.%d.%d.%d], listening to port %d", 
        me->pMyAddr.sin_addr.S_un.S_un_b.s_b1, me->pMyAddr.sin_addr.S_un.S_un_b.s_b2,
        me->pMyAddr.sin_addr.S_un.S_un_b.s_b3, me->pMyAddr.sin_addr.S_un.S_un_b.s_b4, port );
    */

    return( 1 );
}


int piTcpIp_Accept( piTcpIpSocket *vme, piTcpIpSocket *newsocket )
{
    int                heAdrLen;

    ITCPSocket *me = (ITCPSocket*)vme->internalData;
    ITCPSocket *he = (ITCPSocket*)newsocket->internalData;

    heAdrLen = sizeof(struct sockaddr);
    he->pSocket = accept( me->pSocket, (struct sockaddr *)&he->pMyAddr, &heAdrLen );            

    if( he->pSocket==INVALID_SOCKET  )
    {
        int err = WSAGetLastError();
        if( err==WSAEINTR )
        {
            return( 0 );
        }
        else
        {
            /*
            piLog_Printf( LT_EXTRA_PARAMS, LT_ERROR, L"accept()" );
            switch( err )
            {
                case WSAECONNRESET:     piLog_Printf( LT_EXTRA_PARAMS, LT_ERROR, L"WSAECONNRESET" ); break;
                case WSAEFAULT:         piLog_Printf( LT_EXTRA_PARAMS, LT_ERROR, L"WSAEFAULT" ); break;
                case WSAEINVAL:         piLog_Printf( LT_EXTRA_PARAMS, LT_ERROR, L"WSAEINVAL" ); break;
                case WSAEINPROGRESS:    piLog_Printf( LT_EXTRA_PARAMS, LT_ERROR, L"WSAEINPROGRESS" ); break;
                case WSAEMFILE:         piLog_Printf( LT_EXTRA_PARAMS, LT_ERROR, L"WSAEMFILE" ); break;
                case WSAENETDOWN:       piLog_Printf( LT_EXTRA_PARAMS, LT_ERROR, L"WSAENETDOWN" ); break;
                case WSAENOBUFS:        piLog_Printf( LT_EXTRA_PARAMS, LT_ERROR, L"WSAENOBUFS" ); break;
                case WSAEOPNOTSUPP:     piLog_Printf( LT_EXTRA_PARAMS, LT_ERROR, L"WSAEOPNOTSUPP" ); break;
                case WSAEWOULDBLOCK:    piLog_Printf( LT_EXTRA_PARAMS, LT_ERROR, L"WSAEWOULDBLOCK" ); break;
            }
            */
        }
    }

    he->pMyAddr = me->pMyAddr;

    return( 1 );
}


void piTcpIp_PrintMyADdr( const piTcpIpSocket *vme, wchar_t *buffer, int len )
{
    const ITCPSocket *me = (ITCPSocket*)vme->internalData;

    piwsprintf( buffer, len, L"%d.%d.%d.%d",  me->pMyAddr.sin_addr.S_un.S_un_b.s_b1, me->pMyAddr.sin_addr.S_un.S_un_b.s_b2,
        me->pMyAddr.sin_addr.S_un.S_un_b.s_b3, me->pMyAddr.sin_addr.S_un.S_un_b.s_b4 );
}

int piTcpIp_ConvertToNonBlocking(piTcpIpSocket *vme)
{
	ITCPSocket *me = (ITCPSocket*)vme->internalData;

	u_long iMode = 0;
	if ( ioctlsocket(me->pSocket, FIONBIO, &iMode) == SOCKET_ERROR )
		return 0;

	return 1;
}

int piTcpIp_Select( piTcpIpSocket  *vme )
{
    ITCPSocket *me = (ITCPSocket*)vme->internalData;

    fd_set listSocketsSelect;
    FD_ZERO(&listSocketsSelect);
    FD_SET( me->pSocket, &listSocketsSelect);


	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 1;

    int highsock = (int) me->pSocket;
    int readsocks = select(highsock+1, &listSocketsSelect, (fd_set *) 0, (fd_set *) 0, &timeout);
    if( readsocks == SOCKET_ERROR )
        return 0;
    return readsocks;
}
}