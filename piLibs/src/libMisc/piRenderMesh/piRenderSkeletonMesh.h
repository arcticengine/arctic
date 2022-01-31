#pragma once


#include "piLibs/include/libRender/piRenderer.h"
#include "engine/mesh.h"
#include "engine/skeleton.h"

#include "piRenderMesh.h"

typedef struct
{
    piSkeleton   mSkeleton;
    piRenderMesh mMesh;
}piRenderSkeletonMesh;

