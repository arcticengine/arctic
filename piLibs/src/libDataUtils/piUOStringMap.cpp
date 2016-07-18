#include <malloc.h>
#include <string.h>
#include "piUOStringMap.h"
#include "../libSystem/piTypes.h"

namespace piLibs {

piUOStringMap::piUOStringMap()
{
    mMax = 0;
    mNum = 0;
    mObjectsize = 0;
    mBuffer = nullptr;
}

piUOStringMap::~piUOStringMap()
{
}

bool piUOStringMap::Init(unsigned int max, unsigned int valuesize )
{
    mValuesize = valuesize;
    mObjectsize = sizeof(piString)+mValuesize;
    mMax = max;
    mNum = 0;
    mBuffer = (unsigned char*)malloc(max*mObjectsize);
    if( !mBuffer )
        return false;

    memset(mBuffer, 0, max*mObjectsize);

    return true;
}

void piUOStringMap::End(void)
{
    if( mBuffer ) free( mBuffer );
    mBuffer = 0;
	mMax = 0;
	mNum = 0;
	mObjectsize = 0;
}

void piUOStringMap::Reset( void )
{
    memset( mBuffer, 0, mMax*mObjectsize );
    mNum = 0;
}

unsigned int piUOStringMap::GetLength(void)
{
    return mNum;
}

unsigned int piUOStringMap::GetMaxLength(void)
{
    return mMax;
}

void *piUOStringMap::Find(const piString * value) const
{
    const int num = mNum;
    unsigned char *ptr = mBuffer;
    for( int i=0; i<num; i++ )
    {
        piString *str = (piString*)ptr;
        if (str->Equal(value))
            return ptr + sizeof(piString);
        ptr += mObjectsize;
    }

    return nullptr;
}

piString *piUOStringMap::GetKey(unsigned int id) const
{
    if( id<0 || id>mNum ) return nullptr;

    unsigned char *ptr = mBuffer + id*mObjectsize;

    return (piString*)ptr;
}

void *piUOStringMap::GetValue(unsigned int id) const
{
    if (id<0 || id>mNum) return nullptr;

    unsigned char *ptr = mBuffer + id*mObjectsize;

    return ptr + sizeof(piString);
}

void *piUOStringMap::AllocUnique( const piString * value, bool *wasThere, const bool doexpand )
{
    void *ele = Find(value);

    if (*wasThere) *wasThere = (ele==nullptr);

    if (ele) return ele;

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

    piString *dst = (piString*)ptr;

    if (!dst->InitCopy(value))
        return nullptr;

    mNum ++;

    return ptr + sizeof(piString);
}

}