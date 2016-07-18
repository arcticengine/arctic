#pragma once

#include "piRenderFont.h"
#include "../../libRender/piRenderer.h"
#include "../../libMath/piVecTypes.h"
#include "../../libImage/piImage.h"

namespace piLibs {

class piRenderFont2D
{
public:
    piRenderFont2D();
    ~piRenderFont2D();

    bool Init(piRenderer *renderer, const int fontTytpe, const wchar_t *path);
    void Deinit(piRenderer *renderer);

    void Prints( piRenderer *renderer, float x, float y, float sizex, float sizey, bool doItalic,
        bool drawCursor, int cursorPos, double cursorTime,
        const wchar_t *tmp, const float *color,
        const drawCharacterCB * dcCB, float *finalXY);

    void Prints( piRenderer *renderer, float x, float y, float z, const mat4x4& mMVP, float size, bool doItalic,
        bool drawCursor, int cursorPos, double cursorTime,
        const wchar_t *tmp, const float *color,
        const drawCharacterCB * dcCB, float *finalXY);

    void PrintsSyntax( piRenderer *renderer, float x, float y, float sizex, float sizey, bool doItalic,
        bool drawCursor, int cursorPos, double cursorTime,
        const int selectionEnabled, const int selectionStart, const int selectionEnd,
        const wchar_t *str,
        const piRenderFontSyntax *syntax);

    void Length( float sizex, float sizey, bool doItalic, const wchar_t *tmp, float *res);

private:
    void FillInfo(const int fontType);

private:

    typedef struct
    {
        int	xmin;
        int xmax;
    }CharInfo;

    piImage	  mImage;
    piTexture mTexture;
    piShader  mShader;
    piShader  mShader3d;
    CharInfo  mCharInfo[256];
    int       mFontType;
};

} // namespace piLibs