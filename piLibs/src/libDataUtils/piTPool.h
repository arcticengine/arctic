#pragma once

#include <string.h>
#include <malloc.h>
#include "../libSystem/piTypes.h"

namespace piLibs {

    template<typename piPoolType> class piTPool
    {
    public:
        piTPool();
        ~piTPool();

        bool         Init(unsigned int max);
        void         End(void);

        piPoolType * Allocate(bool * isNew, int *id);
        void         Reset(void);
        piPoolType * Get(const unsigned int n) const;
        unsigned int GetLength(void) const;



    private:
        typedef struct
        {
            piPoolType mData;
            bool       mUsed;
        }piPoolTypeInfo;

        piPoolTypeInfo *mBuffer;
        unsigned int    mMax;
        unsigned int    mNum;
    };

    //------------------------------------------------------------------------------------

    template<typename piPoolType>
    piTPool<piPoolType>::piTPool()
    {
        mBuffer = nullptr;
        mMax = 0;
        mNum = 0;
    }

    template<typename piPoolType>
    piTPool<piPoolType>::~piTPool()
    {
    }

    template<typename piPoolType>
    bool piTPool<piPoolType>::Init(unsigned int max)
    {
        mMax = max;
        mNum = 0;
        mBuffer = (piPoolTypeInfo*)malloc(max*sizeof(piPoolTypeInfo));
        if (mBuffer == nullptr)
            return false;

        return true;
    }

    template<typename piPoolType>
    void piTPool<piPoolType>::End(void)
    {
        if (mBuffer != nullptr) free(mBuffer);
        mBuffer = 0;
        mMax = 0;
        mNum = 0;
    }

    template<typename piPoolType>
    piPoolType * piTPool<piPoolType>::Allocate(bool * isNew, int *id)
    {
        // first, try to find an unused one
        const int num = mNum;
        for (int i = 0; i<num; i++)
        {
            piPoolTypeInfo *pti = mBuffer + i;
            if (pti->mUsed == false)
            {
                *isNew = false;
                pti->mUsed = true; // mark it as used
                if( id ) *id = i;
                return &pti->mData;
            }
        }

        // allocate a new one

        // grow if needed
        {
            if ((mNum + 1)>mMax)
            {
                long newmax = 4 * mMax / 3; if (newmax<4) newmax = 4;
                mBuffer = (piPoolTypeInfo*)realloc(mBuffer, newmax*sizeof(piPoolTypeInfo));
                if (!mBuffer)
                    return nullptr;
                mMax = newmax;
            }
        }

        // allocate
        piPoolTypeInfo *pti = mBuffer + mNum;
        if (id) *id = mNum;
        mNum += 1;
        pti->mUsed = true;

        *isNew = true;
        return &pti->mData;
    }

    template<typename piPoolType>
    void piTPool<piPoolType>::Reset(void)
    {
        // mark all as unused
        const int num = mNum;
        for (int i = 0; i<num; i++)
        {
            piPoolTypeInfo *pti = mBuffer + i;
            pti->mUsed = false;
        }
    }

    template<typename piPoolType>
    piPoolType * piTPool<piPoolType>::Get(const unsigned int n) const
    {
        return &mBuffer[n].mData;
    }

    template<typename piPoolType>
    unsigned int piTPool<piPoolType>::GetLength(void) const
    {
        return mNum;
    }

    /*
    template<typename piArrayType>
    void piTArray<piArrayType>::Reset(void)
    {
    memset(mBuffer, 0, mMax*sizeof(piArrayType));
    mNum = 0;
    }



    template<typename piArrayType>
    unsigned int piTArray<piArrayType>::GetMaxLength(void) const
    {
    return mMax;
    }

    template<typename piArrayType>
    piArrayType *piTArray<piArrayType>::GetAddress(const  unsigned int n) const
    {
    return(mBuffer + n);
    }



    template<typename piArrayType>
    piArrayType *piTArray<piArrayType>::Alloc(unsigned int num, bool doexpand)
    {
    if ((mNum + num)>mMax)
    {
    if (doexpand == false)
    return nullptr;

    long newmax = 4 * mMax / 3;
    if (newmax<4) newmax = 4;
    mBuffer = (piArrayType*)realloc(mBuffer, newmax*sizeof(piArrayType));
    if (!mBuffer)
    return nullptr;
    mMax = newmax;
    }

    piArrayType *ptr = mBuffer + mNum;

    mNum += num;

    return ptr;
    }


    template<typename piArrayType>
    piArrayType *piTArray<piArrayType>::Append(const piArrayType * obj, bool doexpand)
    {
    if (mNum >= mMax)
    {
    if (!doexpand)
    return nullptr;

    long newmax = 4 * mMax / 3;
    if (newmax<4) newmax = 4;
    mBuffer = (piArrayType*)realloc(mBuffer, newmax*sizeof(piArrayType));
    if (!mBuffer)
    return nullptr;
    mMax = newmax;
    }

    piArrayType *ptr = mBuffer + mNum;
    memcpy(ptr, obj, sizeof(piArrayType));
    mNum++;
    return ptr;
    }

    template<typename piArrayType>
    bool piTArray<piArrayType>::Append(const piArrayType v, bool doExpand)
    {
    return Append(&v, doExpand) != nullptr;
    }


    template<typename piArrayType>
    piArrayType *piTArray<piArrayType>::InsertAndShift(const piArrayType *obj, unsigned int pos, bool doexpand)
    {
    const unsigned int num = mNum;

    if (mNum >= mMax)
    {

    if (!doexpand)
    return nullptr;

    long newmax = 4 * mMax / 3;
    if (newmax<4) newmax = 4;
    mBuffer = (piArrayType*)realloc(mBuffer, newmax*sizeof(piArrayType));
    if (!mBuffer)
    return nullptr;
    mMax = newmax;
    }

    for (unsigned int i = num; i>pos; i--)
    {
    piArrayType *ori = mBuffer + (i - 1);
    piArrayType *dst = mBuffer + (i + 0);
    memcpy(dst, ori, sizeof(piArrayType));

    }

    piArrayType *ptr = mBuffer + pos;
    if (obj) memcpy(ptr, obj, sizeof(piArrayType));
    mNum++;
    return ptr;
    }

    template<typename piArrayType>
    bool piTArray<piArrayType>::Set(const piArrayType *obj, unsigned int pos)
    {
    if (pos >= mMax)
    return 0;
    mBuffer[pos] = *obj;
    return true;
    }


    template<typename piArrayType>
    void piTArray<piArrayType>::RemoveAndShift(unsigned int pos)
    {
    const unsigned int num = mNum;

    const unsigned int numelems = num - 1 - pos;
    if (numelems>0)
    {
    piArrayType *ptr = mBuffer + pos;
    for (unsigned int j = 0; j<numelems; j++)
    {
    ptr[j] = ptr[j + 1];
    }
    }

    mNum--;
    }

    template<typename piArrayType>
    void piTArray<piArrayType>::Swap(unsigned int ida, unsigned int idb)
    {
    piArrayType tmp = mBuffer[ida];
    mBuffer[ida] = mBuffer[idb];
    mBuffer[idb] = tmp;
    }

    template<typename piArrayType>
    void piTArray<piArrayType>::SetLength(unsigned int num)
    {
    mNum = num;
    }*/




} // namespace piLibs
