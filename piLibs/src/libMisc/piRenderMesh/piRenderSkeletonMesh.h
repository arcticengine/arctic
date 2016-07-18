#pragma once


#include "piLibs/include/libRender/piRenderer.h"
#include "piLibs/include/libMesh/piMesh.h"
#include "piLibs/include/libMesh/piSkeleton.h"

#include "piRenderMesh.h"

typedef struct
{
    piSkeleton   mSkeleton;
    piRenderMesh mMesh;
}piRenderSkeletonMesh;

