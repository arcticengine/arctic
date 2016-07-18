#pragma once

#include "../../libMath/piVecTypes.h"
#include "../../libSystem/piMutex.h"

namespace piLibs {

class piMessage
{
public:
    piMessage();
    ~piMessage();

    void AddStart(int id);
    void AddMatrix4x4f(const mat4x4 & matrix);
    void AddFloat(const float v);
    void AddInt(const int v);
    void AddBoolean( const bool v );
    void AddString( const piString & name );
    void Add(const void *data, int size);

    void ReadStart( void );
    float ReadFloat(void);
    int  ReadInt(void);
    bool ReadBoolean( void );
    mat4x4 ReadMatrix4x4f( void );

    int  GetID( void ) const;
    int  GetHeaderSize(void) const;
    int  GetPacketSize(void) const;
    int  GetDataSize(void) const;
    const void *GetData(void);
    const void *GetData(void) const;
    const void *GetHeader(void);
    const void *GetHeader(void) const;
    const void *GetPacket(void) const;
    const void *GetPacket(void);

private:
    struct Header
    {
        int  mID;
        int  mDataSize;
    }mHeader;
    char mData[1024];
    int  mReadPtr;
};

class piMessageBuffer
{
public:
    piMessageBuffer();
    ~piMessageBuffer();

    bool Init( void );
    void Deinit( void );

    // for the producer : FIX -> this should be a ring buffer for minimal locking
    bool AddMessage( const piMessage *msg );
    // for the consumer : FIX -> this should be a ring buffer for minimal locking
    int  StartProcessingMessages(void);
    void GetMessage(piMessage *msg );
    void StopProcessingMessages(void);

private:
    piMutex  mMutex;
    int      mNumMessages;
    int      mWritePtr;
    int      mMaxSize;
    char    *mBuffer;
    int      mReadPtr;

};

} // namespace Quill