#pragma once

namespace piLibs {

#define IQRFONT_TAHOMA  0
#define IQRFONT_COURIER 1

typedef struct
{
    void (*mFunc)( void *data, float *x, float *y, float *sizex, float *sizey, int id, int lid, int cid );
    void *mData;
}drawCharacterCB;



typedef struct
{
    float     mColor[4];
}piRenderFontSyntax_Style;

typedef struct
{
    int                         mNumTokens;
    piRenderFontSyntax_Style    mStyle;
    const wchar_t             **mTokens;
}piRenderFontSyntax_Tokens;

typedef struct
{
    piRenderFontSyntax_Style    mDefaultStyle;
    int                         mNumGroups;
    piRenderFontSyntax_Tokens   mGroups[16];
}piRenderFontSyntax;



} // namespace piLibs