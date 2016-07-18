#pragma once

#include "../libMesh/piMesh.h"
#include "../libMath/piVecTypes.h"
#include "../libDataUtils/piTArray.h"

namespace piLibs {

class piSkeleton
{
public:
    piSkeleton();
    ~piSkeleton();

    bool Init( int maxBones );
    void End( void );
    int  AddBone( int parentID );


    void UpdateBone( int id, const mat4x4 & m );
    void Update( void );
    void GetData( void *data );

    typedef struct _piBone
    {
        mat4x4 mLocalMatrix;
        mat4x4 mGlobalMatrix;

        int    mNumChildren;
        struct _piBone *mChild[8];
    }piBone;

private:
    void iUpdateBoneGlobalMatrix( piBone *me, piBone *parent );

private:

    piTArray<piBone> mBones;
    void   *mRoot;
};


} // namespace piLibs
