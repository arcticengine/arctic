#pragma once

#include <functional>
#include "../../libLog/piLog.h"
#include "../../libSystem/piTcpIp.h"
#include "../../libSystem/piThread.h"
#include "../../libDataUtils/piArray.h"
#include "../../libMath/piVecTypes.h"

#include "piMessage.h"

namespace piLibs {

class piClient
{
public:
    piClient();
    ~piClient();

    bool Init(piLog * log, const  wchar_t *ipAddress, int port, bool disableBuffering);
    void Deinit(piLog * log);

    typedef std::function<void(piMessage & msg)> GetPacketFunc;

    bool GetPackets( GetPacketFunc func , piLog * log);
    bool SendPacket(const piMessage & msg, piLog * log);

private:
    piTcpIpSocket     mSocket;
    piThread          mThread;
    piLog            *mLog;
    piMessageBuffer   mMessageBuffer;
    //QClientCallbacks *mCallbacks;

    static int myClientThread(void *data);

};


} // namespace Quill