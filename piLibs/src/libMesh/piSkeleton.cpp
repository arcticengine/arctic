#include <string.h>
#include "piSkeleton.h"


namespace piLibs {


piSkeleton::piSkeleton()
{
}

piSkeleton::~piSkeleton()
{
}


bool piSkeleton::Init( int maxBones )
{
    if( !mBones.Init( maxBones, false ) )
        return false;

    mRoot = nullptr;

    return true;
}

void piSkeleton::End( void )
{
    mBones.End();
}

int piSkeleton::AddBone( int parentID )
{
    const int id = mBones.GetLength();

    piBone *me = mBones.Alloc( 1, true );
    if( !me )
        return -1;

    me->mNumChildren = 0;

    if( parentID>0 )
    {
        piBone *parent = mBones.GetAddress( parentID );
        parent->mChild[ parent->mNumChildren++ ] = me;
    }
    else
    {
        mRoot = me;
    }

    return id;
}


void piSkeleton::UpdateBone( int id, const mat4x4 & m )
{
    piBone *me = (piBone*)mBones.GetAddress( id );
    me->mLocalMatrix = m;
}

void piSkeleton::iUpdateBoneGlobalMatrix( piBone *me, piBone *parent )
{
    mat4x4 m = me->mLocalMatrix;    

    if( parent !=nullptr ) m = parent->mGlobalMatrix * m;

    me->mGlobalMatrix = m;

    const int num = me->mNumChildren;
    for( int i=0; i<num; i++ )
    {
        iUpdateBoneGlobalMatrix( me->mChild[i], me );
    }
}

void piSkeleton::Update( void )
{
    iUpdateBoneGlobalMatrix( (piBone*)mRoot, nullptr );    
}

void piSkeleton::GetData( void *data )
{
    char *ptr = (char*)data;
    const int num = mBones.GetLength();
    for( int i=0; i<num; i++ )
    {
        const piBone *me = mBones.GetAddress( i );
        memcpy( ptr, &me->mGlobalMatrix, sizeof(mat4x4) );
        ptr += sizeof(mat4x4);
    }

}


}