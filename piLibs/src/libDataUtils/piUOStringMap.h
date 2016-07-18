#pragma once

#include "../libSystem/piTypes.h"
#include "piString.h"

namespace piLibs {

class piUOStringMap
{
public:
    piUOStringMap();
    ~piUOStringMap();

    bool         Init(unsigned int max, unsigned int valuesize );
    void         End(void);
    unsigned int GetLength(void);
    unsigned int GetMaxLength(void);

    piString *GetKey(unsigned int id) const;
    void     *GetValue(unsigned int id) const;

    void   Reset( void );
    void   SetLength( unsigned int num);

    void  *Find(const piString * key) const;
    void  *AllocUnique(const piString * key, bool *wasThere, const bool doexpand);


private:
    unsigned char *mBuffer;
    unsigned int  mMax;
    unsigned int  mNum;
    unsigned int  mValuesize;
    unsigned int  mObjectsize;
};


} // namespace piLibs
