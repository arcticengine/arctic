#include <malloc.h>
#include <string.h>
#include "../../libLog/piLog.h"
#include "../../libCamera/piCam.h"
#include "../../libSystem/piStr.h"
#include "../../libSystem/piFile.h"
#include "../../libDataUtils/piString.h"
#include "../../libDataUtils/piArray.h"

#include "piMessage.h"

namespace piLibs {


piMessage::piMessage()
{
}

piMessage::~piMessage()
{
}

int piMessage::GetDataSize(void) const
{
    return mHeader.mDataSize;
}

int piMessage::GetHeaderSize(void) const
{
    return sizeof(mHeader.mID) + sizeof(mHeader.mDataSize);
}

int piMessage::GetPacketSize(void) const
{
    return mHeader.mDataSize + GetHeaderSize();
}

const void *piMessage::GetPacket(void) const
{
    return this;
}

const void *piMessage::GetPacket(void)
{
    return this;
}
const void *piMessage::GetHeader(void)
{
    return &mHeader;
}
const void *piMessage::GetHeader(void) const
{
    return &mHeader;
}

const void *piMessage::GetData(void) const
{
    return mData;
}
const void *piMessage::GetData(void)
{
    return mData;
}

int piMessage::GetID(void) const
{
    return mHeader.mID;
}

void piMessage::AddStart(int id)
{
    mHeader.mID = id;
    mHeader.mDataSize = 0;
}

void piMessage::AddMatrix4x4f(const mat4x4 & v)  { Add(v.m, 16 * sizeof(float)); }
void piMessage::AddFloat(const float v)          { Add(&v, 1 * sizeof(float)); }
void piMessage::AddInt(const int v)              { Add(&v, 1 * sizeof(int)); }
void piMessage::AddBoolean(const bool v)         { char c = (v) ? 1 : 0;  Add(&c, 1 * sizeof(char)); }
void piMessage::AddString(const piString & name) { const int num = name.GetLength(); AddInt( num ); Add( name.GetS(), num*sizeof(wchar_t) ); }

void piMessage::Add(const void *data, int size)
{
    memcpy(mData + mHeader.mDataSize, data, size);
    mHeader.mDataSize += size;
}

int    piMessage::ReadInt(void)        { const int    res = *((int  *)(mData + mReadPtr)); mReadPtr += sizeof(  int); return res; }
float  piMessage::ReadFloat(void)      { const float  res = *((float*)(mData + mReadPtr)); mReadPtr += sizeof(float); return res; }
bool   piMessage::ReadBoolean(void)    { const char   res = *((char *)(mData + mReadPtr)); mReadPtr += sizeof( char); return (res==1); }
mat4x4 piMessage::ReadMatrix4x4f(void) { const float *res = (const float*)(mData + mReadPtr); mReadPtr += 16*sizeof(float); return mat4x4(res); }

void piMessage::ReadStart(void)
{
    mReadPtr = 0;
}

//====================================

piMessageBuffer::piMessageBuffer()
{
}

piMessageBuffer::~piMessageBuffer()
{
}

bool piMessageBuffer::Init( void )
{

    mNumMessages = 0;
    mReadPtr = 0;
    mWritePtr = 0;
    mMaxSize = 65536;

    mBuffer = (char*)malloc(mMaxSize);
    if( !mBuffer )
        return false;

    mMutex = piMutex_Init();
    if (mMutex == nullptr)
        return false;

    return true;
}

void piMessageBuffer::Deinit(void)
{
    piMutex_Free(mMutex);
    free(mBuffer);
}

int piMessageBuffer::StartProcessingMessages(void)
{
    piMutex_Lock(mMutex);
    mReadPtr = 0;
    return mNumMessages;
}

void piMessageBuffer::StopProcessingMessages(void)
{
    mNumMessages = 0;
    mWritePtr = 0;
    piMutex_UnLock(mMutex);
}

void piMessageBuffer::GetMessage(piMessage *msg)
{
    const int mhs = msg->GetHeaderSize();

    const int  *header = (int*)(mBuffer + mReadPtr);
    const char *data = mBuffer + mReadPtr + mhs;
    const int id = header[0];
    const int ds = header[1];

    msg->AddStart( id );
    msg->Add(data, ds);
    msg->ReadStart();
    mReadPtr += mhs + ds;
}


bool piMessageBuffer::AddMessage(const piMessage *msg)
{
    piMutex_Lock(mMutex);

    const int mhs = msg->GetHeaderSize();
    const int mds = msg->GetDataSize();

    if (mWritePtr + mhs + mds > mMaxSize)
    {
        piMutex_UnLock(mMutex);
        return false;
    }

    memcpy(mBuffer + mWritePtr, msg->GetHeader(), mhs);
    mWritePtr += mhs;

    memcpy(mBuffer + mWritePtr, msg->GetData(), mds);
    mWritePtr += mds;

    mNumMessages += 1;

    piMutex_UnLock(mMutex);

    return true;
}


} // namespace Quill