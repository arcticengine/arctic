#pragma once

#include "../../libSystem/piFile.h"

namespace piLibs {

class piWav
{
public:
    piWav();
    ~piWav();

    bool  Open(const wchar_t *name);
    void  Deinit(void);

    int   mNumChannels;
    int   mRate;
    int   mBits;
    int   mNumSamples;
    void *mData;
    int   mDataSize; // mNumSamples * mNumChannels * mBits / 8
};


} // namespace piLibs