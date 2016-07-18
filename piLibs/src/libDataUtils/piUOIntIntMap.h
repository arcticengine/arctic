#pragma once

#include "../libSystem/piTypes.h"

namespace piLibs {

class piUOIntIntMap
{
public:
    piUOIntIntMap();
    ~piUOIntIntMap();

    bool Init(unsigned int max);
    void End(void);

    uint32 FindFirst(const uint32 first) const;
    uint32 FindSecond(const uint32 second) const;
    bool   Find(const uint32 first, const uint32 second) const;

    bool Add(const uint32 first, const uint32 second, const bool doexpand);

private:
    uint32       *mBuffer;
    unsigned int  mMax;
    unsigned int  mNum;
};


} // namespace piLibs