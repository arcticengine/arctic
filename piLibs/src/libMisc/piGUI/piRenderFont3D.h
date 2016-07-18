#pragma once

#include "piRenderFont.h"
#include "../../libDataUtils/piTArray.h"
#include "../../libRender/piRenderer.h"
#include "../../libMath/piVecTypes.h"
#include "../../libMisc/piRenderMesh/piRenderMesh.h"

namespace piLibs {

class piRenderFont3D
{
public:
    piRenderFont3D();
    ~piRenderFont3D();

    bool Init(piRenderer *renderer, const int fontTytpe, const wchar_t *path);
    void Deinit(piRenderer *renderer);

    void Prints( piRenderer *renderer, float x, float y, float z, float sizex, float sizey, bool doItalic, 
				 bool drawCursor, int cursorPos, float cursorTime,
				 const wchar_t *tmp, const float *color, int offsetUniformPos, bool stereoInstances);

private:
    typedef struct
    {
        float        mWidth;
        piRenderMesh mRenderMesh;
        bool         mLoaded;
    }CharInfo;

    piTArray<CharInfo>   mChars;
    int       mFontType;
    bound3    mBBox;
};

} // namespace piLibs