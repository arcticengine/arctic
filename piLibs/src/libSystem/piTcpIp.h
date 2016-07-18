#pragma once

namespace piLibs {
typedef struct
{
    int    internalData[64];
}piTcpIpSocket;


int  piTcpIp_Init( void );
void piTcpIp_End( void );

// -1:desconnect, -2:error, >=0: recieveb byets
int  piTcpIp_Receive( piTcpIpSocket *me, char *data, int len );
int  piTcpIp_Send( piTcpIpSocket *me, const char *data, int len );
int  piTcpIp_ReceiveExact( piTcpIpSocket *me, char *data, int len );

int  piTcpIp_CreateAndListen(piTcpIpSocket *vme, int port, bool disableBuffering);
int  piTcpIp_Accept( piTcpIpSocket *vme, piTcpIpSocket *newsocket );
int  piTcpIp_Select( piTcpIpSocket  *vme );

int  piTcpIp_CreateAndConnectByURL(piTcpIpSocket *vme, const char *address, int port, bool disableBuffering);
void piTcpIp_Close( piTcpIpSocket *me );

void piTcpIp_PrintMyADdr( const piTcpIpSocket *vme, wchar_t *buffer, int len );
int  piTcpIp_ConvertToNonBlocking(piTcpIpSocket *vme);

} // namespace piLibs