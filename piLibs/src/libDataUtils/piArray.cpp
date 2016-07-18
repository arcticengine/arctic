#include <malloc.h>
#include <string.h>
#include "piArray.h"
#include "../libSystem/piTypes.h"

namespace piLibs {

piArray::piArray()
{
    mBuffer = nullptr;
    mMax = 0;
    mNum = 0;
    mObjectsize = 0;
}

piArray::~piArray()
{
}

bool piArray::IsInitialized(void) const
{
    return (mObjectsize>0);
}

int piArray::Init(  unsigned int max, unsigned int objectsize )
{
	mObjectsize = objectsize;
    mMax = max;
    mNum = 0;
    mBuffer = (unsigned char*)malloc( max*objectsize );

    if( !mBuffer )
        return( 0 );

    memset( mBuffer, 0, max*objectsize );

    return( 1 );
}

void piArray::End(void)
{
    if( mBuffer ) free( mBuffer );
    mBuffer = 0;
	mMax = 0;
	mNum = 0;
	mObjectsize = 0;
}

void piArray::Reset(void)
{
    memset( mBuffer, 0, mMax*mObjectsize );
    mNum = 0;
}

unsigned int piArray::GetLength(void) const
{
    return( mNum );
}

unsigned int piArray::GetMaxLength(void) const
{
    return mMax;
}

void *piArray::GetAddress(const  unsigned int n) const
{
	return( (void*)(mBuffer+n*mObjectsize) );
}

void  *piArray::Alloc(  unsigned int num, bool doexpand )
{
    if( (mNum+num)>mMax )
	{
		if( doexpand==false )
			return( 0 );

        long newmax = 4*mMax/3;
        if( newmax<4 ) newmax = 4;
        mBuffer = (unsigned char*)realloc( mBuffer, newmax*mObjectsize );
        if( !mBuffer )
            return( 0 );
        mMax = newmax;
	}

    unsigned char *ptr = mBuffer + mNum*mObjectsize;

    mNum += num;

    return( ptr );
}



void *piArray::Append(  const void *obj, bool doexpand )
{
    if( mNum>=mMax )
    {
		if( !doexpand )
			return 0;

        long newmax = 4*mMax/3;
        if( newmax<4 ) newmax = 4;
        mBuffer = (unsigned char*)realloc( mBuffer, newmax*mObjectsize );
        if( !mBuffer )
            return( 0 );
        mMax = newmax;
    }
    unsigned char *ptr = mBuffer + mNum*mObjectsize;
    memcpy( ptr, obj, mObjectsize );
    mNum ++;
    return( ptr );
}


bool piArray::Set(  const void *obj, unsigned int pos )
{
	const unsigned int num = mNum;
	const unsigned int obs = mObjectsize;
	if( pos>=mMax )
        return 0;
    unsigned char *ptr = mBuffer + pos*obs;
	if( obj ) memcpy( ptr, obj, mObjectsize );
    return true;
}

void *piArray::Insert(  const void *obj, unsigned int pos, bool doexpand )
{
	const unsigned int num = mNum;
	const unsigned int obs = mObjectsize;

	if( mNum>=mMax )
    {

		if( !doexpand )
			return 0;

        long newmax = 4*mMax/3;
        if( newmax<4 ) newmax = 4;
        mBuffer = (unsigned char*)realloc( mBuffer, newmax*mObjectsize );
        if( !mBuffer )
            return( 0 );
        mMax = newmax;
    }

	for( unsigned int i=num; i>pos; i-- )
	{
		unsigned char *ori = mBuffer + (i-1)*obs;
		unsigned char *dst = mBuffer + (i+0)*obs;
		memcpy( dst, ori, obs );

	}

    unsigned char *ptr = mBuffer + pos*obs;
	if( obj ) memcpy( ptr, obj, mObjectsize );
    mNum ++;
    return( ptr );

}

void piArray::RemoveAndShift(  unsigned int pos )
{
	const unsigned int num = mNum;
	const unsigned int obs = mObjectsize;

	const unsigned int numelems = num-1-pos;
	if( numelems>0 )
	{
        unsigned char *ptr = mBuffer + pos*obs;
        for( unsigned int j=0; j<numelems; j++ )
        {
            for( unsigned int i=0; i<obs; i++ ) 
                ptr[i] = ptr[obs+i];
            ptr += obs;
        }
	}

    mNum --;
}

void piArray::SetLength(  unsigned int num )
{
	mNum = num;
}




/*



int ARRAY_CopyContent( ARRAY *dst, ARRAY *ori )
{
    if( dst->max<ori->num )
        return( 0 );

	dst->num = ori->num;
	memcpy( dst->buffer, ori->buffer, ori->num*ori->objectsize );

    return( 1 );
}

int ARRAY_CopyRawContent( ARRAY *dst, ARRAY *ori, unsigned int amount )
{
	memcpy( dst->buffer, ori->buffer, amount );

    return( 1 );
}


int ARRAY_Copy( ARRAY *dst, ARRAY *ori )
{
	if( !ARRAY_Init( dst, ori->max, ori->objectsize ) )
		return( 0 );

	dst->num = ori->num;
	memcpy( dst->buffer, ori->buffer, ori->num*ori->objectsize );

	return( 1 );
}




void ARRAY_Remove( ARRAY *me, unsigned int id )
{

    const unsigned int num = num - 1;
    const unsigned int objSize = objectsize;
    unsigned char *ptr = (unsigned char*)ARRAY_GetPointer( me, num );
    for( unsigned int i=id; i<num; i++ )
        memcpy( ptr+i*objSize, ptr+(i+1)*objSize, objSize );

    memset( ptr+id*objSize, 0, objSize );

    num--;
}




int ARRAY_AppendIntPtr( ARRAY *me, pint v )
{
    if( !ARRAY_InsertAndExpand( me, &v ) )
        return( 0 );

    return( 1 );
}

int ARRAY_AppendPtr( ARRAY *me, const void *ptr )
{
    pint i = (pint)ptr;

    if( !ARRAY_InsertAndExpand( me, &i ) )
        return( 0 );

    return( 1 );
}

*/

void piArray::SetPtr(  unsigned int id, const void *val )
{
    ((void**)mBuffer)[id] = (void*)val;
}



//------

bool piArray::AppendUInt8(   uint8  v, bool doExpand ) { return piArray::Append( &v, doExpand )!=NULL; }
bool piArray::AppendSInt8(   sint8  v, bool doExpand ) { return piArray::Append( &v, doExpand )!=NULL; }
bool piArray::AppendUInt16(  uint16 v, bool doExpand ) { return piArray::Append( &v, doExpand )!=NULL; }
bool piArray::AppendSInt16(  sint16 v, bool doExpand ) { return piArray::Append( &v, doExpand )!=NULL; }
bool piArray::AppendUInt32(  uint32 v, bool doExpand ) { return piArray::Append( &v, doExpand )!=NULL; }
bool piArray::AppendSInt32(  sint32 v, bool doExpand ) { return piArray::Append( &v, doExpand )!=NULL; }
bool piArray::AppendFloat(   float  v, bool doExpand ) { return piArray::Append( &v, doExpand )!=NULL; }
bool piArray::AppendDouble(  double v, bool doExpand ) { return piArray::Append( &v, doExpand )!=NULL; }
bool piArray::AppendPtr(     void * v, bool doExpand ) { return piArray::Append( &v, doExpand )!=NULL; }

uint32 piArray::GetUint32(int id) const { return ((uint32*)mBuffer)[id]; }
sint32 piArray::GetSint32(int id) const { return ((sint32*)mBuffer)[id]; }
uint16 piArray::GetUint16(int id) const { return ((uint16*)mBuffer)[id]; }
sint16 piArray::GetSInt16(int id) const { return ((sint16*)mBuffer)[id]; }
uint8  piArray::GetUint8(int id) const { return ((uint8 *)mBuffer)[id]; }
sint8  piArray::GetSint8(int id) const { return ((sint8 *)mBuffer)[id]; }
void  *piArray::GetPtr(int id) const { return ((void **)mBuffer)[id]; }

}