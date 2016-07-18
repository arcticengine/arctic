#pragma once

#include <functional>
#include "../../libLog/piLog.h"
#include "../../libSystem/piTcpIp.h"
#include "../../libSystem/piThread.h"
#include "../../libSystem/piMutex.h"
#include "../../libDataUtils/piTArray.h"
#include "../../libMath/piVecTypes.h"

#include "piMessage.h"

namespace piLibs {

class piServer
{
public:
    piServer();
    ~piServer();

    typedef std::function<void(int from, piMessage & msg)> ProcessMsgFunc;
    typedef std::function<void(int id)> ClientAddedFunc;
    typedef std::function<void(int id)> ClientRemovedFunc;

    bool Init(piLog * log, int port, bool disableBuffering, ProcessMsgFunc pmFunc, ClientAddedFunc caFunc, ClientRemovedFunc crFunc);
    void Deinit(piLog * log);
    int  GetNumClients(void);
    int  GetClients(piTArray<int> *ar);

    void Broadcast(int excludeID, const piMessage * msg); // send to all except "excludeID"
    void Send(int toID, const piMessage * msg);           // send to "toID"


private:
    struct SClient
    {
        bool          mUsed;
        piTcpIpSocket mSocket;
        int           mID;
        piThread      mThread;
        piServer      *mServer;
    };

    piTcpIpSocket mSocket;
    piMutex       mMutex;
    piThread      mThread;
    piLog        *mLog;
    piTArray<SClient>  mClients;
    ProcessMsgFunc    mProcessMsgFunc;
    ClientAddedFunc   mClientAddedFunc;
    ClientRemovedFunc mClientRemovedFunc;

    static int myServerThread(void *data);
    static int myServerClientThread(void *data);


    bool AddClient(piTcpIpSocket *socket);
    bool DelClient(int id);
    
};


}