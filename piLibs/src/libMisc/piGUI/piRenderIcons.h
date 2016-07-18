#pragma once

#include "../../libRender/piRenderer.h"
#include "../../libMath/piVecTypes.h"

namespace piLibs {

class piRenderIcons
{
public:
    piRenderIcons();
    ~piRenderIcons();

    bool Init(piRenderer *renderer, const wchar_t *path);
    void Deinit(piRenderer *renderer);

    typedef struct
    {
        piTexture texture;
        bound2    uv;
    }IconInfo;

    void Render(  piRenderer *renderer, float x, float y, float sizex, float sizey, const float *color, int id );
    void Render(  piRenderer *renderer, float x, float y, float z, const mat4x4& mMVP, float size, const float *color, int id );
    void GetIcon( IconInfo *info, int id);

private:
    piTexture mTexture;
    piShader  mShader;
    piShader  mShader3d;
    int       mNumIcons;
    int       mIconSize;
    int       mTilesX;
    int       mTilesY;
};

} // namespace piLibs