#pragma once

#include "../libSystem/piTypes.h"

namespace piLibs {

class piUOIntMap
{
public:
    piUOIntMap();
    ~piUOIntMap();

    bool         Init(unsigned int max, unsigned int valuesize );
    void         End(void);
    unsigned int GetLength(void);
    unsigned int GetMaxLength(void);

    uint32  GetKey(unsigned int id) const;
    void   *GetValue(unsigned int id) const;

    void   Reset( void );
    void   SetLength( unsigned int num);

    void  *Find(const uint32 key, int *eleID) const;
    void  *AllocUnique(const uint32 key, int *eleId, const bool doexpand);


private:
    unsigned char *mBuffer;
    unsigned int  mMax;
    unsigned int  mNum;
    unsigned int  mValuesize;
    unsigned int  mObjectsize;
};

} // namespace piLibs