#include <malloc.h>
#include <string.h>
#include "piUOIntMap.h"
#include "../libSystem/piTypes.h"

namespace piLibs {

piUOIntMap::piUOIntMap()
{
    mMax = 0;
    mNum = 0;
    mObjectsize = 0;
    mBuffer = nullptr;
}

piUOIntMap::~piUOIntMap()
{
}

bool piUOIntMap::Init(unsigned int max, unsigned int valuesize)
{
    mValuesize = valuesize;
    mObjectsize = sizeof(uint32)+mValuesize;
    mMax = max;
    mNum = 0;
    mBuffer = (unsigned char*)malloc(max*mObjectsize);
    if( !mBuffer )
        return false;

    memset(mBuffer, 0, max*mObjectsize);

    return true;
}

void piUOIntMap::End(void)
{
    if( mBuffer ) free( mBuffer );
    mBuffer = 0;
	mMax = 0;
	mNum = 0;
	mObjectsize = 0;
}

void piUOIntMap::Reset(void)
{
    memset( mBuffer, 0, mMax*mObjectsize );
    mNum = 0;
}

unsigned int piUOIntMap::GetLength(void)
{
    return mNum;
}

unsigned int piUOIntMap::GetMaxLength(void)
{
    return mMax;
}

void *piUOIntMap::Find(const uint32 key, int *eleID) const
{
    const int num = mNum;
    unsigned char *ptr = mBuffer;
    for( int i=0; i<num; i++ )
    {
        if( ((uint32*)ptr)[0] == key )
        {
            if (eleID) eleID[0] = i;
            return ptr + sizeof(uint32);
        }
        ptr += mObjectsize;
    }

    if (eleID) eleID[0] = -1;
    return nullptr;
}

uint32 piUOIntMap::GetKey(unsigned int id) const
{
    if( id<0 || id>mNum ) return -1;

    unsigned char *ptr = mBuffer + id*mObjectsize;

    return *((uint32*)ptr);
}

void *piUOIntMap::GetValue(unsigned int id) const
{
    if (id<0 || id>mNum) return nullptr;

    unsigned char *ptr = mBuffer + id*mObjectsize;

    return ptr + sizeof(uint32);
}

void *piUOIntMap::AllocUnique(const uint32 key, int *eleId, const bool doexpand)
{
    void *ele = this->Find(key, eleId);

    bool found = (ele != nullptr);

    if (found ) return ele;

    if( (mNum+1)>mMax )
	{
		if( doexpand==false )
			return nullptr;

        long newmax = 4*mMax/3;
        if( newmax<4 ) newmax = 4;
        mBuffer = (unsigned char*)realloc( mBuffer, newmax*mObjectsize );
        if( !mBuffer )
            return nullptr;
        mMax = newmax;
	}

    unsigned char *ptr = mBuffer + mNum*mObjectsize;

    ((uint32*)ptr)[0] = key;

    mNum = mNum  + 1;

    return ptr + sizeof(uint32);
}

} // namespace piLibs