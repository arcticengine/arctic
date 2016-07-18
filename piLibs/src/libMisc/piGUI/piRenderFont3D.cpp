#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "../../libSystem/piStr.h"
#include "../../libDataUtils/piArray.h"
#include "../../libRender/piRenderer.h"
#include "../../libMesh/piMesh.h"
#include "../../libMath/piVecTypes.h"
#include "../../libMesh/formats/piMeshObj.h"
#include "../../libMesh/generation/piMesh_ModComplex.h"
#include "../../libMisc/piRenderMesh/piRenderMesh.h"
#include "piRenderFont3D.h"


namespace piLibs {

static const wchar_t *names[] = 
{
/*  33 ! */ L"sexclamation", 
/*  34  " */ L"sdoublequote", 
/*  35  # */ L"spound", 
/*  36  $ */ L"sdollar", 
/*  37  % */ L"spercentage", 
/*  38  & */ L"sand", 
/*  39  ' */ L"squote", 
/*  40  ( */ L"sopenbracket", 
/*  41  ) */ L"sclosebracket", 
/*  42 * */ L"sasterisk", 
/*  43 + */ L"splus", 
/*  44 , */ L"scoma", 
/*  45 - */ L"sminus", 
/*  46 . */ L"sdot", 
/*  47 / */ L"slash", 
/*  48 0 */ L"s0", 
/*  49 1 */ L"s1", 
/*  50 2 */ L"s2", 
/*  51 3 */ L"s3", 
/*  52 4 */ L"s4", 
/*  53 5 */ L"s5", 
/*  54 6 */ L"s6", 
/*  55 7 */ L"s7", 
/*  56 8 */ L"s8", 
/*  57 9 */ L"s9", 
/*  58 : */ L"scolom", 
/*  59 ; */ L"ssemicolom", 
/*  60 < */ L"smaller", 
/*  61 = */ L"sequal", 
/*  62 > */ L"sbigger", 
/*  63 ? */ L"sinterrogation", 
/*  64 @ */ L"sat", 
/*  65 A */ L"ua", 
/*  66 B */ L"ub", 
/*  67 C */ L"uc", 
/*  68 D */ L"ud", 
/*  69 E */ L"ue", 
/*  70 F */ L"uf", 
/*  71 G */ L"ug", 
/*  72 H */ L"uh", 
/*  73 I */ L"ui", 
/*  74 J */ L"uj", 
/*  75 K */ L"uk", 
/*  76 L */ L"ul", 
/*  77 M */ L"um", 
/*  78 N */ L"un", 
/*  79 O */ L"uo", 
/*  80 P */ L"up", 
/*  81 Q */ L"uq", 
/*  82 R */ L"ur", 
/*  83 S */ L"us", 
/*  84 T */ L"ut", 
/*  85 U */ L"uu", 
/*  86 V */ L"uv", 
/*  87 W */ L"uw", 
/*  88 X */ L"ux", 
/*  89 Y */ L"uy", 
/*  90 Z */ L"uz", 
/*  91 [ */ L"sopenbrace", 
/*  92 \ */ L"sbackslash", 
/*  93 ] */ L"sclosebrace", 
/*  94 ^ */ L"stick", 
/*  95 _ */ L"sunderbar", 
/*  96 ` */ L"squote", 
/*  97 a */ L"la", 
/*  98 b */ L"lb", 
/*  99 c */ L"lc", 
/* 100 d */ L"ld", 
/* 101 e */ L"le", 
/* 102 f */ L"lf", 
/* 103 g */ L"lg", 
/* 104 h */ L"lh", 
/* 105 i */ L"li", 
/* 106 j */ L"lj", 
/* 107 k */ L"lk", 
/* 108 l */ L"ll", 
/* 109 m */ L"lm", 
/* 110 n */ L"ln", 
/* 111 o */ L"lo", 
/* 112 p */ L"lp", 
/* 113 q */ L"lq", 
/* 114 r */ L"lr", 
/* 115 s */ L"ls", 
/* 116 t */ L"lt", 
/* 117 u */ L"lu", 
/* 118 v */ L"lv", 
/* 119 w */ L"lw", 
/* 120 x */ L"lx", 
/* 121 y */ L"ly", 
/* 122 z */ L"lz", 
/* 123 { */ L"sopencurly", 
/* 124 | */ L"spipe", 
/* 125 } */ L"sclosecurly", 
/* 126 ~ */ L"snot"
};


piRenderFont3D::piRenderFont3D()
{
}

piRenderFont3D::~piRenderFont3D()
{
}

#define FIRSTCHAR 33
#define LASTCHAR 126

bool piRenderFont3D::Init(piRenderer *renderer, const int fontType, const wchar_t *path)
{
    mFontType = fontType;

    const int num = 256;
    if (!mChars.Init(num, false))
        return false;

    for( int i=0; i<num; i++ )
    {
        CharInfo *info = (CharInfo*)mChars.Alloc(1, false);
        info->mWidth = -1.0f;
        info->mLoaded = false;
    }

    wchar_t fname2[256];

    mBBox = bound3( 1e20f );
	bool loadedSomething = false;
    for (int i = FIRSTCHAR; i <= LASTCHAR; i++)
    {
        CharInfo *info = (CharInfo*)mChars.GetAddress(i);

        piMesh mesh;

        wchar_t fname1[256];        
        piwsprintf( fname1, 256, L"%s/default/%s.mesh", path, names[i-FIRSTCHAR] );
        if( !mesh.Load( fname1 ) )
        {
            piwsprintf(fname2, 256, L"%s/default/src/%s.obj", path, names[i - FIRSTCHAR]);
            if( !piMeshObj_Read( &mesh, fname2, true ) )
            {
                continue;
            }
            mesh.Save( fname1 );
         }
		
        info->mLoaded = true;

        //piMesh_Subdivide( &mesh, 0, 1, 0 );         piMesh_Normalize( &mesh, 0, 1 );

        mesh.CalcBBox( 0, 0 );

        if (!info->mRenderMesh.InitFromMesh( renderer, &mesh, piPT_Triangle))
        {
            mesh.DeInit();
            return false;
        }

        info->mWidth = mesh.mBBox.mMaxX - mesh.mBBox.mMinX;

        mesh.DeInit();
        
        mBBox = expand( mBBox, info->mRenderMesh.GetBBox() );


		loadedSomething = true;
    }


	if( !loadedSomething ) return false;

	return true;
}

void piRenderFont3D::Deinit(piRenderer *renderer)
{
    for (int i = FIRSTCHAR; i <= LASTCHAR; i++)
    {
        CharInfo *info = (CharInfo*)mChars.GetAddress(i);
        if( info->mLoaded )
        {
            info->mRenderMesh.End(renderer);
        }
    }
    mChars.End();
}


void piRenderFont3D::Prints( piRenderer *renderer, float x, float y, float z, float sizex, float sizey, bool doItalic,
						     bool drawCursor, int cursorPos, float cursorTime,
                             const wchar_t *str, const float *color, int offsetUniformPos, bool stereoInstances)
{
	const int len = str?piwstrlen( str ):0;

    float xo = x;
    float cx = mBBox.mMaxX - mBBox.mMinX;
    float cy = mBBox.mMaxY - mBBox.mMinY;

	for( int i=0; i<len; i++ )
	{
		const wchar_t c = str[i];

		if( c==10 )//L'\n' )
		{
			x  = xo;
			y -= cy;
		}
		if( c==32 )
		{
            CharInfo *info = (CharInfo*)mChars.GetAddress((int)'a');
            x += info->mWidth;
		}
		else
		{
            CharInfo *info = (CharInfo*)mChars.GetAddress( (int)c);

            if( info->mLoaded )
            {
                const float offset[4] = { x, y, z, 0.0f };
                renderer->SetShaderConstant4F(offsetUniformPos, offset, 1);
                info->mRenderMesh.Render( renderer, 0, stereoInstances?2:1 );
                x += info->mWidth;
            }
        }

    }
}

}