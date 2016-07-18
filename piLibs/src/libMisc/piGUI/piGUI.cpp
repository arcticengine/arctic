#include <math.h>
#include "../../libRender/piRenderer.h"
#include "piGUI.h"

namespace piLibs {

static const char vs0[] = \

"layout (location=1) uniform vec4 uXYUV[2];"

"layout (location=0) in vec2 inVertex;"
"layout (location=1) in vec2 inUV;"

"out vec2 vfUV;"

"void main(void)"
"{"
    "vec4 xyuv = uXYUV[0] + (uXYUV[1]-uXYUV[0])*(0.5+0.5*inVertex.xyxy);"
	"gl_Position = vec4(xyuv.xy,0.0,1.0);"
	"vfUV = xyuv.zw;"
"}";

static const char fs0[] = \

"layout (location=0) uniform vec4 uColor;"
"layout (binding=0) uniform sampler2D t0;"

"in vec2 vfUV;"
"out vec4 outColor;"

"void main(void)"
"{"
	"float f = texture(t0,vfUV).x;"
    "outColor = vec4( uColor.xyz, f*uColor.w );"
"}";


static const char vs1[] = \

"layout (location=1) uniform vec4 uXYXY;"

"layout (location=0) in vec2 inVertex;"
"layout (location=1) in vec2 inUV;"

"void main(void)"
"{"
"vec2 xyuv = uXYXY.xy + (uXYXY.zw-uXYXY.xy)*(0.5+0.5*inVertex);"
"gl_Position = vec4(xyuv.xy,0.0,1.0);"
"}";

static const char fs1[] = \

"layout (location=0) uniform vec4 uColor;"
"layout (binding=0) uniform sampler2D t0;"

"in vec2 vfUV;"
"out vec4 outColor;"

"void main(void)"
"{"
"outColor = uColor;"
"}";

piGUI::piGUI()
{
}

piGUI::~piGUI()
{
}

bool piGUI::Initialize(piRenderer *renderer)
{
    char error[2048];

    mShader[0] = renderer->CreateShader(nullptr, vs0, nullptr, nullptr, nullptr, fs0, error);
    if (mShader[0] == nullptr )
        return false;

    mShader[1] = renderer->CreateShader(nullptr, vs1, nullptr, nullptr, nullptr, fs1, error);
    if (mShader[1] == nullptr )
        return false;

    if (!mFonts2D.Init(8, true))
        return false;

    if (!mFonts3D.Init(8, true))
        return false;

    if (!mIcons2D.Init(8, true))
        return false;
    
    return true;
}


void piGUI::Deinitialize(piRenderer *renderer)
{
    {
        const int num = mIcons2D.GetLength();
        for (int i = 0; i<num; i++)
        {
            piRenderIcons *font = (piRenderIcons*)mIcons2D.GetAddress(i);
            font->Deinit(renderer);
        }
    }
    {
        const int num = mFonts2D.GetLength();
        for( int i=0; i<num; i++ )
        {
            piRenderFont2D *font = (piRenderFont2D*)mFonts2D.GetAddress(i);
            font->Deinit(renderer);
        }
    }
    {
    const int num = mFonts3D.GetLength();
    for (int i = 0; i<num; i++)
    {
        piRenderFont3D *font = (piRenderFont3D*)mFonts3D.GetAddress(i);
        font->Deinit(renderer);
    }
}
    mFonts2D.End();
    mFonts3D.End();
    mIcons2D.End();

    renderer->DestroyShader(mShader[0]);
    renderer->DestroyShader(mShader[1]);
}

int piGUI::LoadFont2D(piRenderer *renderer, const int fontTytpe, const wchar_t *path)
{
    const int id = mFonts2D.GetLength();
    piRenderFont2D *font = (piRenderFont2D*)mFonts2D.Alloc(1, true);
    if (!font) return -1;

    if( !font->Init(renderer, fontTytpe, path) )
        return -1;

    return id;
}

int piGUI::LoadFont3D(piRenderer *renderer, const int fontTytpe, const wchar_t *path)
{
    const int id = mFonts2D.GetLength();
    piRenderFont3D *font = (piRenderFont3D*)mFonts3D.Alloc(1, true);
    if (!font) return -1;

    if (!font->Init(renderer, fontTytpe, path))
        return -1;

    return id;
}

int piGUI::LoadIconSet2D(piRenderer *renderer, const wchar_t *path)
{
    const int id = mIcons2D.GetLength();
    piRenderIcons *icons = (piRenderIcons*)mIcons2D.Alloc(1, true);
    if (!icons) return -1;

    if (!icons->Init(renderer, path))
        return -1;

    return id;
}

piRenderFont2D *piGUI::GetFont2D(const int fontTytpe)
{
    return (piRenderFont2D*)mFonts2D.GetAddress(fontTytpe);
}

piRenderFont3D *piGUI::GetFont3D(const int fontTytpe)
{
    return (piRenderFont3D*)mFonts3D.GetAddress(fontTytpe);
}

piRenderIcons *piGUI::GetIconSet2D(const int fontTytpe)
{
       return (piRenderIcons*)mIcons2D.GetAddress(fontTytpe);
}

void piGUI::DrawRectColor(piRenderer *renderer, float x1, float y1, float x2, float y2, const float *color)
{
    const float xy1xy2[4] = { x1, y1, x2, y2 };

    renderer->AttachShader(mShader[1]);
    renderer->SetShaderConstant4F(0, color, 1);
    renderer->SetShaderConstant4F(1, xy1xy2, 1);
    renderer->DrawUnitQuad_XY(1);
    renderer->DettachShader();
}

}