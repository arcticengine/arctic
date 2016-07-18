#include <malloc.h>
#include <math.h>
#include "../../libRender/piRenderer.h"
#include "../../libImage/piImage.h"
#include "../../libImage/processing/piImageFlip.h"
#include "piRenderIcons.h"

namespace piLibs {


static const char vs[] = \
"layout (location=0) uniform vec4 data[3];"

"layout (location=0) in vec2 inVertex;"
"layout (location=1) in vec2 inUV;"

"out vec2 vfUV;"

"void main( void )"
"{"
    "vec2 xy = data[0].xy + data[0].zw * (0.5 + 0.5*inVertex.xy);"
    "vec2 uv = data[1].xy + data[1].zq * (0.5 + 0.5*inVertex.xy*vec2(1.0,-1.0));"

    "gl_Position = vec4( xy, 0.0, 1.0 );"
    "vfUV  = uv;"
"}";

static const char fs[] = \
"layout (location=0) uniform vec4 data[3];"
"layout (binding=0) uniform sampler2D t0;"

"in vec2 vfUV;"
"out vec4 outColor;"

"void main( void )"
"{"
    "float f = texture( t0, vfUV ).x;"
    "outColor = vec4( data[2].xyz, f*data[2].w );"
"}";


static const char vs3d[] = \
"layout (location=0) uniform vec4 data[3];"
"layout (location=3) uniform float z;"
"layout (location=4) uniform mat4x4 mMatrix_CamPrj;"

"layout (location=0) in vec2 inVertex;"
"layout (location=1) in vec2 inUV;"

"out vec2 vfUV;"

"void main( void )"
"{"
    "vec2 xy = data[0].xy + data[0].zw * (0.5 + 0.5*inVertex.xy);"
    "vec2 uv = data[1].xy + data[1].zq * (0.5 + 0.5*inVertex.xy*vec2(1.0,-1.0));"

    "gl_Position = mMatrix_CamPrj * vec4( xy, z, 1.0 );"
    "vfUV  = uv;"
"}";

static const char fs3d[] = \
"layout (location=0) uniform vec4 data[3];"
"layout (binding=0) uniform sampler2D t0;"

"in vec2 vfUV;"
"out vec4 outColor;"

"void main( void )"
"{"
    "float f = texture( t0, vfUV ).x;"
    "outColor = vec4( data[2].xyz, f*data[2].w );"
"}";


piRenderIcons::piRenderIcons()
{
}

piRenderIcons::~piRenderIcons()
{
}

bool piRenderIcons::Init(piRenderer *renderer, const wchar_t *path)
{
    mIconSize = 128;

    piImage image;
    if (!image.Load(path))
        return nullptr;

    //piImage_Flip( &image );

    mTilesX = image.GetXRes() / mIconSize;
    mTilesY = image.GetYRes() / mIconSize;
    mNumIcons = mTilesX * mTilesY;

    const piTextureInfo tinfo0 = { piTEXTURE_2D, piFORMAT_C1I8, false, image.GetXRes(), image.GetYRes(), 1, 1 };
    mTexture = renderer->CreateTexture( 0, &tinfo0, piFILTER_MIPMAP, piWRAP_CLAMP, 1.0f, image.GetData() );
    if( !mTexture )
        return nullptr;

    char error[2048];
    mShader = renderer->CreateShader(nullptr, vs, 0, 0, 0, fs, error);
    if( !mShader )
        return nullptr;

    mShader3d = renderer->CreateShader(nullptr, vs3d, 0, 0, 0, fs3d, error);
    if( !mShader )
        return nullptr;

    image.Free();

    return true;
}

void piRenderIcons::Deinit( piRenderer *renderer)
{
    renderer->DestroyShader( mShader );
    renderer->DestroyShader( mShader3d );
    renderer->DestroyTexture( mTexture );
}

void piRenderIcons::GetIcon( IconInfo *info, int id)
{
    int idx = id % mTilesX;
    int idy = id / mTilesX;
    //idy = mTilesY - 1 - idy;

    float s = 1.0f / (float)mTilesX;
    float t = 1.0f / (float)mTilesY;
    float u = float(idx)*s;
    float v = float(idy)*t;


    info->texture = mTexture;
    info->uv = bound2(u, u + s, v, v + t);
}

void piRenderIcons::Render( piRenderer *renderer, float x, float y, float sizex, float sizey, const float *color, int id)
{
    int idx = id % mTilesX;
    int idy = id / mTilesX;

    float s = 1.0f/(float)mTilesX;
    float t = 1.0f/(float)mTilesY;
    float u = float(idx)*s;
    float v = float(idy)*t;

    const float xywh_uvst_rgba[12] = { x, y, sizex, sizey, 
                                        u, v, s, t,                         
                                        color[0], color[1], color[2], color[3] };

    renderer->SetState(piSTATE_BLEND, true);
    renderer->SetBlending(0, piBLEND_ADD, piBLEND_SRC_ALPHA, piBLEND_ONE_MINUS_SRC_ALPHA,
                             piBLEND_ADD, piBLEND_SRC_ALPHA, piBLEND_ONE_MINUS_SRC_ALPHA);
    renderer->AttachShader(mShader);
    renderer->SetShaderConstant4F( 0, xywh_uvst_rgba, 3 );
    renderer->AttachTextures( 1, mTexture );
    renderer->DrawUnitQuad_XY(1);
    renderer->DettachTextures();
    renderer->DettachShader();
    renderer->SetState(piSTATE_BLEND, false);
}

void piRenderIcons::Render( piRenderer *renderer, float x, float y, float z, const mat4x4& mMVP, float size, const float *color, int id)
{
    int idx = id % mTilesX;
    int idy = id / mTilesX;

    float s = 1.0f / (float)mTilesX;
    float t = 1.0f / (float)mTilesY;
    float u = float(idx)*s;
    float v = float(idy)*t;

    const float xywh_uvst_rgba[12] = { x, y, size, size, 
                                        u, v, s, t,                         
                                        color[0], color[1], color[2], color[3] };

    renderer->SetState(piSTATE_BLEND, true);
    renderer->SetBlending(0, piBLEND_ADD, piBLEND_SRC_ALPHA, piBLEND_ONE_MINUS_SRC_ALPHA,
                             piBLEND_ADD, piBLEND_SRC_ALPHA, piBLEND_ONE_MINUS_SRC_ALPHA);
    renderer->AttachShader(mShader3d);
    renderer->SetShaderConstant4F( 0, xywh_uvst_rgba, 3 );
    renderer->SetShaderConstantMat4F( 4, (float*)&mMVP, 1, true);
    renderer->SetShaderConstant1F( 3, &z, 1 );
    renderer->AttachTextures( 1, mTexture );
    renderer->DrawUnitQuad_XY(1);
    renderer->DettachTextures();
    renderer->DettachShader();
    renderer->SetState(piSTATE_BLEND, false);
}

}