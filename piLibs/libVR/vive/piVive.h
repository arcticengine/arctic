#pragma once

#include "../piVR.h"

namespace piLibs {

class piVRVive : public piVR
{
public:
    bool      Init(void);
    void      DeInit(void);
    piVRHMD  *CreateHmd(int deviceID, float pixelDensity);
    void      DestroyHmd(piVRHMD * me);
};


}