#pragma once

#include <string.h>
#include <malloc.h>
#include "../libSystem/piTypes.h"

namespace piLibs {

template<typename piArrayType> class piTArray
{
public:
    piTArray();
    ~piTArray();

    bool         Init( unsigned int max, bool doZero );
    void         End(void);
    bool         IsInitialized( void ) const;

    unsigned int GetLength(void) const;
    void         SetLength( unsigned int num);
    void         Reset(void);
    unsigned int GetMaxLength(void) const;

    piArrayType *GetAddress(const unsigned int n) const;
    piArrayType  Get( const unsigned int id) const;
    bool         Set( const piArrayType *obj, unsigned int pos);
    piArrayType *Alloc( unsigned int num, bool doexpand = false);
    piArrayType *Append( const piArrayType *obj, bool doexpand = false);
    bool         Append( const piArrayType v, bool doExpand = false);

    piArrayType *InsertAndShift( const piArrayType *obj, unsigned int pos, bool doexpand = false);
    void         RemoveAndShift( unsigned int pos);
    void         Swap(unsigned int ida, unsigned int idb);

    piArrayType *begin() const { return mBuffer; }
    piArrayType *end() const { return mBuffer + mNum; }

private:
    piArrayType   *mBuffer;
    unsigned int   mMax;
    unsigned int   mNum;
};

//------------------------------------------------------------------------------------

template<typename piArrayType> 
piTArray<piArrayType>::piTArray()
{
    mBuffer = nullptr;
    mMax = 0;
    mNum = 0;
}

template<typename piArrayType> 
piTArray<piArrayType>::~piTArray()
{
}

template<typename piArrayType> 
bool piTArray<piArrayType>::IsInitialized(void) const
{
    return mMax > 0;
}

template<typename piArrayType> 
bool piTArray<piArrayType>::Init(  unsigned int max, bool doZero )
{
    mMax = max;
    mNum = 0;
    mBuffer = (piArrayType*)malloc( max*sizeof(piArrayType) );
    if( mBuffer==nullptr )
        return false;

    if( doZero ) memset( mBuffer, 0, max*sizeof(piArrayType) );

    return true;
}

template<typename piArrayType> 
void piTArray<piArrayType>::End(void)
{
    if( mBuffer!=nullptr ) free( mBuffer );
    mBuffer = 0;
	mMax = 0;
	mNum = 0;
}

template<typename piArrayType> 
void piTArray<piArrayType>::Reset(void)
{
    memset( mBuffer, 0, mMax*sizeof(piArrayType) );
    mNum = 0;
}

template<typename piArrayType> 
unsigned int piTArray<piArrayType>::GetLength(void) const
{
    return mNum;
}

template<typename piArrayType> 
unsigned int piTArray<piArrayType>::GetMaxLength(void) const
{
    return mMax;
}

template<typename piArrayType> 
piArrayType *piTArray<piArrayType>::GetAddress(const  unsigned int n) const
{
	return( mBuffer + n );
}

template<typename piArrayType> 
piArrayType piTArray<piArrayType>::Get(const unsigned int n) const 
{ 
    return mBuffer[n]; 
}


template<typename piArrayType> 
piArrayType *piTArray<piArrayType>::Alloc(  unsigned int num, bool doexpand )
{
    if( (mNum+num)>mMax )
	{
		if( doexpand==false )
			return nullptr;

        long newmax = 4*mMax/3;
        if( newmax<4 ) newmax = 4;
        mBuffer = (piArrayType*)realloc( mBuffer, newmax*sizeof(piArrayType) );
        if( !mBuffer )
            return nullptr;
        mMax = newmax;
	}

    piArrayType *ptr = mBuffer + mNum;

    mNum += num;

    return ptr;
}


template<typename piArrayType> 
piArrayType *piTArray<piArrayType>::Append(  const piArrayType * obj, bool doexpand )
{
    if( mNum>=mMax )
    {
		if( !doexpand )
			return nullptr;

        long newmax = 4*mMax/3;
        if( newmax<4 ) newmax = 4;
        mBuffer = (piArrayType*)realloc( mBuffer, newmax*sizeof(piArrayType) );
        if( !mBuffer )
            return nullptr;
        mMax = newmax;
    }

    piArrayType *ptr = mBuffer + mNum;
    memcpy( ptr, obj, sizeof(piArrayType) );
    mNum ++;
    return ptr;
}

template<typename piArrayType> 
bool piTArray<piArrayType>::Append( const piArrayType v, bool doExpand )
{
    return Append( &v, doExpand ) != nullptr;
}


template<typename piArrayType> 
piArrayType *piTArray<piArrayType>::InsertAndShift(  const piArrayType *obj, unsigned int pos, bool doexpand )
{
	const unsigned int num = mNum;

	if( mNum>=mMax )
    {

		if( !doexpand )
			return nullptr;

        long newmax = 4*mMax/3;
        if( newmax<4 ) newmax = 4;
        mBuffer = (piArrayType*)realloc( mBuffer, newmax*sizeof(piArrayType) );
        if( !mBuffer )
            return nullptr;
        mMax = newmax;
    }

	for( unsigned int i=num; i>pos; i-- )
	{
		piArrayType *ori = mBuffer + (i-1);
		piArrayType *dst = mBuffer + (i+0);
		memcpy( dst, ori, sizeof(piArrayType) );

	}

    piArrayType *ptr = mBuffer + pos;
	if( obj ) memcpy( ptr, obj, sizeof(piArrayType) );
    mNum ++;
    return ptr;
}

template<typename piArrayType> 
bool piTArray<piArrayType>::Set(  const piArrayType *obj, unsigned int pos )
{
	if( pos>=mMax )
        return 0;
    mBuffer[pos] = *obj;
    return true;
}


template<typename piArrayType> 
void piTArray<piArrayType>::RemoveAndShift(  unsigned int pos )
{
	const unsigned int num = mNum;

	const unsigned int numelems = num-1-pos;
	if( numelems>0 )
	{
        piArrayType *ptr = mBuffer + pos;
        for( unsigned int j=0; j<numelems; j++ )
        {
            ptr[j] = ptr[j+1];
        }
	}

    mNum --;
}

template<typename piArrayType>
void piTArray<piArrayType>::Swap(unsigned int ida, unsigned int idb)
{
    piArrayType tmp = mBuffer[ida];
    mBuffer[ida] = mBuffer[idb];
    mBuffer[idb] = tmp;
}

template<typename piArrayType> 
void piTArray<piArrayType>::SetLength(  unsigned int num )
{
	mNum = num;
}




} // namespace piLibs
