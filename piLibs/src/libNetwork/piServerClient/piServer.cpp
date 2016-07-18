
#include "../../libLog/piLog.h"
#include "../../libCamera/piCam.h"
#include "../../libSystem/piStr.h"
#include "../../libSystem/piFile.h"
#include "../../libDataUtils/piString.h"
#include "../../libDataUtils/piArray.h"
#include "../../libMath/piShading.h"
#include "../../libImage/piImage.h"

#include "piServer.h"

namespace piLibs {


piServer::piServer()
{
}

piServer::~piServer()
{
}

int piServer::myServerThread(void *data)
{
    piServer *me = (piServer*)data;
    piLog * log = me->mLog;

    bool done = false;
    while( !done )
    {
        wchar_t buf[256];
        piTcpIpSocket newSocket;

        piTcpIp_PrintMyADdr(&me->mSocket, buf, 256);
        log->Printf(LT_MESSAGE, L"Waiting for connections at %s...", buf);
        if (!piTcpIp_Accept(&me->mSocket, &newSocket))
        {
            log->Printf(LT_ERROR, L"Could not accept connection");
            return false;
        }

        if (!me->AddClient(&newSocket))
        {
            log->Printf(LT_ERROR, L"Could not add new client");
            continue;
        }

        piTcpIp_PrintMyADdr(&newSocket, buf, 256);
        log->Printf(LT_MESSAGE, L"New client has IP %s", buf );
    }

    return 0;
}

bool piServer::Init(piLog * log, int port, bool disableBuffering, ProcessMsgFunc pmFunc, ClientAddedFunc caFunc, ClientRemovedFunc crFunc)
{
    if (!piTcpIp_Init())
    {
        log->Printf(LT_ERROR, L"Could not initialize Network stack");
        return false;
    }

    const int num = 256;

    if (!mClients.Init(num, true))
        return false;

    mClients.SetLength(num);
   
    for (int i = 0; i<num; i++)
    {
        SClient *client = (SClient*)mClients.GetAddress(i);
        client->mUsed = false;
    }

    mProcessMsgFunc = pmFunc;
    mClientAddedFunc = caFunc;
    mClientRemovedFunc = crFunc;

    mMutex = piMutex_Init();
    if( mMutex==nullptr )
        return false;

    if (!piTcpIp_CreateAndListen(&mSocket, port, disableBuffering))
        return false;
    mLog = log;

    mThread = piThread_Init(myServerThread, this);
    if (!mThread )
        return false;

    return true;
}

void piServer::Deinit(piLog * log)
{
    piMutex_Free( mMutex );
    mClients.End();
    piTcpIp_Close(&mSocket);
    piThread_End(mThread);
}


int piServer::GetNumClients(void)
{
    const int num = mClients.GetLength();
    int numUsed = 0;
    for (int i = 0; i<num; i++)
    {
        SClient *client = (SClient*)mClients.GetAddress(i);
        if (client->mUsed == false) continue;
        numUsed++;
    }
    return numUsed;
}

int piServer::GetClients( piTArray<int> *ar )
{
    const int num = mClients.GetLength();
    int numUsed = 0;
    ar->SetLength(0);
    for (int i = 0; i<num; i++)
    {
        SClient *client = (SClient*)mClients.GetAddress(i);
        if (client->mUsed == false) continue;
        ar->Append(client->mID, true);
        numUsed++;
    }
    return numUsed;
}


bool piServer::AddClient(piTcpIpSocket *socket)
{
    // find an empty slot

    piMutex_Lock(mMutex);

    SClient *client = nullptr;
    int id = -1;

    const int num = mClients.GetLength();
    for( int i=0; i<num; i++ )
    {
        client = (SClient*)mClients.GetAddress(i);
        if( client->mUsed == false )
        {
            id = i;
            break;
        }
    }

    piMutex_UnLock(mMutex);

    if (id == -1) return false;

    client->mUsed = true;
    client->mSocket = *socket;
    client->mID = id;
    client->mServer = this;
    client->mThread = piThread_Init(myServerClientThread, client);
    if (!client->mThread)
        return false;
    
    mClientAddedFunc( id );

    return true;
}

bool piServer::DelClient(int id)
{
    mLog->Printf( LT_MESSAGE, L"Deleting client %d", id);

    bool wasAlive = true;

    piMutex_Lock(mMutex);

    SClient *client = (SClient*)mClients.GetAddress(id);
    wasAlive = client->mUsed;
    if( wasAlive )
    {
        piTcpIp_Close(&client->mSocket);
        client->mUsed = false;
    }

    piMutex_UnLock(mMutex);

    // tell all clients a peer is gone
    if (wasAlive)
    {
        mClientRemovedFunc( id );
    }

    return true;
}

int piServer::myServerClientThread(void *data)
{
    SClient *me = (SClient*)data;
    piLog * log = me->mServer->mLog;

    bool done = false;
    while (!done)
    {
        piMessage msg;

        int n = piTcpIp_ReceiveExact(&me->mSocket, (char*)msg.GetPacket(), msg.GetHeaderSize()); if (n<0) break;
            n = piTcpIp_ReceiveExact(&me->mSocket, (char*)msg.GetData(),   msg.GetDataSize());   if (n<0) break;

            me->mServer->mProcessMsgFunc(me->mID, msg);
    }

    me->mServer->DelClient(me->mID);

    return 0;
}

void piServer::Broadcast(int from, const piMessage * msg)
{
    const int num = mClients.GetLength();
    int numUpdateClients = 0;
    for (int i = 0; i<num; i++)
    {
        SClient *client = (SClient*)mClients.GetAddress(i);
        if (client->mUsed == false) continue;
        if (client->mID == from) continue; // do not broadcast to ourselves

        int n = piTcpIp_Send(&client->mSocket, (const char*)msg->GetPacket(), msg->GetPacketSize());
        if (n<0)
        {
            DelClient(i);
        }

        numUpdateClients++;
    }
    //mLog->Printf(LT_EXTRA_PARAMS, LT_MESSAGE, L"From client %d, broadcasted MID: %d to %d clients", from, msg->GetID(), numUpdateClients);
}

void piServer::Send(int id, const piMessage * msg)
{
    SClient *client = (SClient*)mClients.GetAddress(id);
    if (client->mUsed == false) return;

    int n = piTcpIp_Send(&client->mSocket, (const char*)msg->GetPacket(), msg->GetPacketSize());
    if (n<0)
    {
        DelClient(id);
    }
}


} // namespace Quill