
#include "../../libSystem/piTypes.h"
#include "../../libSystem/piStr.h"
#include "../../libSystem/piFile.h"
#include "../../libLog/piLog.h"
#include "../../libDataUtils/piString.h"
#include "../../libDataUtils/piArray.h"

#include "piMessage.h"
#include "piClient.h"

namespace piLibs {

piClient::piClient()
{
}
piClient::~piClient()
{
}

bool piClient::Init(piLog * log, const  wchar_t *ipAddress, int port, bool disableBuffering)
{
    const char * address = piws2str(ipAddress);
    if (address==nullptr )
        return false;

    if (!mMessageBuffer.Init() )
        return false;

    if (!piTcpIp_Init())
    {
        log->Printf(LT_ERROR, L"Could not initialize Network stack");
        return false;
    }

    if (!piTcpIp_CreateAndConnectByURL(&mSocket, address, port, disableBuffering))
    {
        log->Printf(LT_ERROR, L"Could not connected to server %s...", ipAddress);
        return false;
    }
    log->Printf(LT_MESSAGE, L"Connected to server %s", ipAddress);
    
    mLog = log;

    mThread = piThread_Init(myClientThread, this);
    if (!mThread)
        return false;

    return true;
}

void piClient::Deinit(piLog * log)
{
    piTcpIp_Close(&mSocket);
    mMessageBuffer.Deinit();
}

int piClient::myClientThread(void *data)
{
    piClient *me = (piClient*)data;
    piLog * log = me->mLog;
    for( ;; )
    {
        piMessage msg;

        int n = piTcpIp_ReceiveExact(&me->mSocket, (char*)msg.GetHeader(), msg.GetHeaderSize()); if (n<0) break;
            n = piTcpIp_ReceiveExact(&me->mSocket, (char*)msg.GetData(),   msg.GetDataSize());   if (n<0) break;

        if( !me->mMessageBuffer.AddMessage(&msg) )
        {
            log->Printf( LT_WARNING, L"Could not store message");
        }
    }

    log->Printf( LT_ERROR, L"Ending Client thread");

    return 0;
}

//------------------------------------------------------------

bool piClient::GetPackets(GetPacketFunc func, piLog * log)
{
    piMessage msg;

    const int num = mMessageBuffer.StartProcessingMessages();
    for( int i=0; i<num; i++ )
    {
        mMessageBuffer.GetMessage(&msg);
        func( msg );
    }
    mMessageBuffer.StopProcessingMessages();

    return true;
}

bool piClient::SendPacket(const piMessage & msg, piLog * log)
{
    int n = piTcpIp_Send(&mSocket, (const char*)msg.GetPacket(), msg.GetPacketSize());
    if (n<0)
    {
        log->Printf( LT_MESSAGE, L"Server disconnected!");
        return false;
    }

    return true;
}


} // namespace Quill