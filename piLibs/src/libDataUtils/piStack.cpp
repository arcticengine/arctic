#include <string.h>
#include <malloc.h>

#include "piStack.h"

namespace piLibs {

piStack::piStack()
{
}

piStack::~piStack()
{
}

int piStack::Init(int amount, int objectsize)
{
    if( amount<4 )
        return( 0 );

    mBuffer = (unsigned char*)malloc( amount*objectsize );
    if( !mBuffer )
        return( 0 );

    mAmount = amount;
    mNum = 0;
    mObjectsize = objectsize;

    return( 1 );
}

void piStack::End( void )
{
    if( mBuffer ) free( mBuffer );
}


/*
void *piStack::Push(int num)
{
    unsigned char *ptr;

    if( (mNum+num)>mAmount )
        return( 0 );

    ptr = mBuffer + mNum*mObjectsize;

    mNum += num;

    return( ptr );
}


void piStack::Pop(void *ptr, int num)
{

	mNum -= num;
	memcpy( ptr, mBuffer + mNum*mObjectsize, num );
}*/

int piStack::IsEmpty( void )
{
    return( mNum==0 );
}

void piStack::Reset( void )
{
    mNum = 0;
}


void * piStack::GetLastData(void)
{
    if( mNum<1 ) return nullptr;
    return (void*)(mBuffer + (mNum-1)*mObjectsize);
}

int piStack::GetHeadPos(void)
{
    return mNum;
}
/*
void piStack::SetPos(int pos)
{
    mNum = pos;
}*/

void piStack::Push2(void *obj )
{
    memcpy( mBuffer+mNum*mObjectsize, obj, mObjectsize );
    mNum++;
}

void *piStack::Pop2( void )
{
    mNum--;

    return mBuffer+mNum*mObjectsize;
}


}