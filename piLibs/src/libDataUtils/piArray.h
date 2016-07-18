#pragma once

#include "../libSystem/piTypes.h"

namespace piLibs {

class piArray
{
public:
    piArray();
    ~piArray();

    int          Init( unsigned int max, unsigned int objectsize);
    void         End(void);
    unsigned int GetLength(void) const;
    unsigned int GetMaxLength(void) const;
    bool         IsInitialized( void ) const;

    void   Reset(void);
    void   SetLength( unsigned int num);
    void  *GetAddress(const  unsigned int n) const;

    void  *Alloc( unsigned int num, bool doexpand = false);
    void  *Append( const void *obj, bool doexpand = false);
    void  *Insert( const void *obj, unsigned int pos, bool doexpand = false);
    bool   Set( const void *obj, unsigned int pos);
    void   RemoveAndShift(unsigned int pos);

    bool   AppendUInt8(uint8  v, bool doExpand = false);
    bool   AppendSInt8(sint8  v, bool doExpand = false);
    bool   AppendUInt16(uint16 v, bool doExpand = false);
    bool   AppendSInt16(sint16 v, bool doExpand = false);
    bool   AppendUInt32(uint32 v, bool doExpand = false);
    bool   AppendSInt32(sint32 v, bool doExpand = false);
    bool   AppendUInt64(uint64 v, bool doExpand = false);
    bool   AppendSInt64(sint64 v, bool doExpand = false);
    bool   AppendFloat(float  v, bool doExpand = false);
    bool   AppendDouble(double v, bool doExpand = false);
    bool   AppendPtr( void * v, bool doExpand=false);

    void   SetPtr( unsigned int id, const void *val);

    uint64 GetUint64(int id) const;
    sint64 GetSint64(int id) const;
    uint32 GetUint32(int id) const;
    sint32 GetSint32(int id) const;
    uint16 GetUint16(int id) const;
    sint16 GetSInt16(int id) const;
    uint8  GetUint8(int id) const;
    sint8  GetSint8(int id) const;
    void  *GetPtr(int id) const;

private:
    unsigned char *mBuffer;
    unsigned int  mMax;
    unsigned int  mNum;
    unsigned int  mObjectsize;
};

} // namespace piLibs
