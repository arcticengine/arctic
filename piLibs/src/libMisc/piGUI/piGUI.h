#pragma once

#include "../../libDataUtils/piTArray.h"
#include "../../libRender/piRenderer.h"
#include "../../libMath/piVecTypes.h"

#include "piRenderFont2D.h"
#include "piRenderFont3D.h"
#include "piRenderIcons.h"

namespace piLibs {

class piGUI
{
public:
    piGUI();
    ~piGUI();

    bool  Initialize(piRenderer *renderer);
    void  Deinitialize(piRenderer *renderer);

    int  LoadFont2D(piRenderer *renderer, const int fontTytpe, const wchar_t *path);
    int  LoadFont3D(piRenderer *renderer, const int fontTytpe, const wchar_t *path);
    int  LoadIconSet2D(piRenderer *renderer, const wchar_t *path);

    void  DrawRectColor(piRenderer *renderer, float x1, float y1, float x2, float y2, const float *color);
    
    piRenderFont2D  *GetFont2D(const int id);
    piRenderFont3D  *GetFont3D(const int id);
    piRenderIcons   *GetIconSet2D(const int id);

private:
    piShader mShader[2];
    piTArray<piRenderFont2D>  mFonts2D;
    piTArray<piRenderFont3D>  mFonts3D;
    piTArray<piRenderIcons>  mIcons2D;
};

} // namespace piLibs