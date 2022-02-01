#pragma once

#include "../libSystem/piTypes.h"

namespace piLibs {


typedef void * piTexture;
typedef void * piShader;
typedef void * piVertexArray;
typedef void * piFont3D;
typedef void * piRTarget;
typedef void * piSampler;
typedef void * piBuffer;

typedef enum
{
    piTEXTURE_1D   = 0,
    piTEXTURE_2D   = 1,
    piTEXTURE_3D   = 2,
    piTEXTURE_CUBE = 3,

    piTEXTURE_2D_ARRAY = 4

}piTextureType;

typedef enum
{
    piFILTER_NONE   = 0,
    piFILTER_LINEAR = 1,
    piFILTER_MIPMAP = 2,
    piFILTER_PCF = 3,
    piFILTER_NONE_MIPMAP = 4,
}piTextureFilter;

typedef enum
{
    piWRAP_CLAMP_TO_BORDER  = 0,
    piWRAP_CLAMP  = 1,
    piWRAP_REPEAT = 2,
    piWRAP_MIRROR_CLAMP = 3,
    piWRAP_MIRROR_REPEAT = 4,
    
}piTextureWrap;

typedef enum
{
    piFORMAT_C1I8 =  0,
    piFORMAT_C2I8 =  1,
    piFORMAT_C3I8 =  2,
    piFORMAT_C4I8 =  3,

    piFORMAT_C1F16 =  4,
    piFORMAT_C2F16 =  5,
    piFORMAT_C3F16 =  6,
    piFORMAT_C4F16 =  7,

    piFORMAT_C1F32 =  8,
    piFORMAT_C2F32 =  9,
    piFORMAT_C3F32 = 10,
    piFORMAT_C4F32 = 11,

    piFORMAT_C1I8I = 12,
    piFORMAT_C1I16I = 13,
    piFORMAT_C1I32I = 14,

    piFORMAT_D24 = 40,
    piFORMAT_D32F = 41,

    piFORMAT_C4I1010102 = 42,
    piFORMAT_C3I111110 = 43,

    piFORMAT_UNKOWN = 100
}piTextureFormat;

typedef struct
{
    piTextureType   mType;
    piTextureFormat mFormat;
    int             mXres;
    int             mYres;
    int             mZres;
    int             mMultisample;
	bool            mCompressed;
unsigned int mDeleteMe;
}piTextureInfo;

typedef enum
{
    piBufferType_Static  = 0,
    piBufferType_Dynamic = 1,
}piBufferType;

typedef enum
{
    piRArrayType_UByte    = 0,
    piRArrayType_Float    = 1,
    piRArrayType_Int      = 2,
    piRArrayType_Double   = 3,
    piRArrayType_Half     = 4,
}piRArrayDataType;

typedef struct
{
    int             mStride;
    int             mNumElements;
    int             mDivisor;
    struct
    {
        int              mNumComponents;
	    piRArrayDataType mType;
	    bool             mNormalize;
    }mEntry[12];
}piRArrayLayout;

typedef enum
{
    piBLEND_ONE  = 0,
    piBLEND_SRC_ALPHA = 1,
    piBLEND_SRC_COLOR = 2,
    piBLEND_ONE_MINUS_SRC_COLOR = 3,
    piBLEND_ONE_MINUS_SRC_ALPHA = 4,
    piBLEND_DST_ALPHA = 5,
    piBLEND_ONE_MINUS_DST_ALPHA = 6,
    piBLEND_DST_COLOR = 7,
    piBLEND_ONE_MINUS_DST_COLOR = 8,
    piBLEND_SRC_ALPHA_SATURATE = 9,
    piBLEND_ZERO = 10
}piBlendOperations;

typedef enum
{
    piBLEND_ADD  = 0,
    piBLEND_SUB = 1,
    piBLEND_RSUB = 2,
    piBLEND_MIN = 3,
    piBLEND_MAX = 4,
}piBlendEquation;

typedef enum
{
    piSTATE_CULL_FACE  = 0,
    piSTATE_DEPTH_TEST = 1,
	piSTATE_WIREFRAME = 2,
    piSTATE_FRONT_FACE = 3,
    piSTATE_BLEND = 4,
    piSTATE_ALPHA_TO_COVERAGE = 5,
    piSTATE_DEPTH_CLAMP = 6,
    piSTATE_VIEWPORT_FLIPY = 7
}piState;

typedef enum
{
    piPT_Triangle = 0,
    piPT_Point = 1,
    piPT_TriangleStrip = 2,
    piPT_LineStrip = 3,
    piPT_TriPatch = 4,
    piPT_QuadPatch = 5,
    piPT_LinesAdj = 6,
    piPT_LineStripAdj = 7,
    piPT_16Patch = 8,
    piPT_32Patch = 9,
    piPT_Lines = 10
}piPrimitiveType;

typedef enum
{
    piBARRIER_SHADER_STORAGE = 1,
    piBARRIER_UNIFORM = 2,
    piBARRIER_ATOMICS = 4,
    piBARRIER_IMAGE = 8,
    piBARRIER_COMMAND = 16,
    piBARRIER_TEXTURE = 32,
    piBARRIER_ALL = 0
}piBarrierType;

typedef struct
{
    int mNum;
    struct
    {
        char mName[64];
        int  mValue;
    }mOption[64];
}piShaderOptions;

typedef  struct 
{
    uint32  count;
    uint32  instanceCount;
    uint32  first;
    uint32  baseInstance;
}piDrawArraysIndirectCommand;

typedef  struct 
{
    uint32  count;
    uint32  instanceCount;
    uint32  firstIndex;
    uint32  baseVertex;
    uint32  baseInstance;
}piDrawElementsIndirectCommand;


class piRenderReporter
{
public:
    piRenderReporter() {}
    ~piRenderReporter() {}

    virtual void Info( const char *str ) = 0;
	//-----------
    virtual void Error( const char *str, int level ) = 0;
	//-----------
    virtual void Begin( uint64 memLeaks, uint64 memPeak, int texLeaks, int texPeak ) = 0;
    virtual void Texture( const wchar_t *key, uint64 kb, piTextureFormat format, bool compressed, int xres, int yres, int zres ) = 0;
    virtual void End( void ) = 0;

};

typedef enum
{
    // hardware shading
	GL = 0, 
    DX  = 1
}piRendererType;

class piRenderer
{
public:
	static piRenderer *Create( const piRendererType type );

protected:
    piRenderer();
public:
	~piRenderer();

    virtual bool      Initialize(int id, const void **hwnd, int num, bool disableVSync, piRenderReporter *reporter) = 0;
    virtual void      Deinitialize( void ) = 0;
    virtual void      Report( void ) = 0;
    virtual void      SetActiveWindow( int id ) = 0;
    virtual void      Enable(void) = 0;
    virtual void      Disable(void) = 0;
    virtual void	  SwapBuffers(void) = 0;

    //--- render targets ---
    virtual piRTarget CreateRenderTarget(piTexture vtex0, piTexture vtex1, piTexture vtex2, piTexture vtex3, piTexture zbuf) = 0;
    virtual void      DestroyRenderTarget( piRTarget obj ) = 0;
    virtual bool      SetRenderTarget( piRTarget obj ) = 0;
    virtual void      RenderTargetSampleLocations(piRTarget vdst, const float *locations) = 0;
    virtual void      BlitRenderTarget( piRTarget dst, piRTarget src, bool color, bool depth ) = 0;
    virtual void      SetWriteMask( bool c0, bool c1, bool c2, bool c3, bool z ) = 0;
    virtual void      SetShadingSamples( int shadingSamples ) = 0;
    virtual void      RenderTargetGetDefaultSampleLocation(piRTarget vdst, const int id, float *location) = 0;

    //--- general ---
    virtual void      Clear( const float *color0, const float *color1, const float *color2, const float *color3, const bool depth0 ) = 0;
    virtual void      SetState( piState state, bool value ) = 0;
    virtual void      SetBlending(int buf, piBlendEquation equRGB, piBlendOperations srcRGB, piBlendOperations dstRGB,
                                           piBlendEquation equALP, piBlendOperations srcALP, piBlendOperations dstALP) = 0;
    virtual void      SetViewport(int id, const int *vp) = 0;

    //--- textures ---
    virtual piTexture CreateTexture( const wchar_t *key, const piTextureInfo *info, piTextureFilter filter, piTextureWrap wrap, float aniso, void *buffer ) = 0;
    virtual void      DestroyTexture( piTexture obj ) = 0;
    virtual void      ClearTexture( piTexture vme, int level, const void *data ) = 0;
    virtual void      UpdateTexture( piTexture me, int x0, int y0, int z0, int xres, int yres, int zres, const void *buffer ) = 0;
	virtual void	  GetTextureRes( piTexture me, int *res ) = 0;
    virtual void      GetTextureFormat( piTexture me, piTextureFormat *format ) = 0;
    virtual void      GetTextureContent( piTexture me, void *data, const piTextureFormat fmt ) = 0;
    virtual void      GetTextureContent(piTexture vme, void *data, int x, int y, int z, int xres, int yres, int zres) = 0;
    virtual void      GetTextureInfo( piTexture me, piTextureInfo *info ) = 0;
    virtual void      GetTextureSampling(piTexture vme, piTextureFilter *rfilter, piTextureWrap *rwrap) = 0;
    virtual void      ComputeMipmaps( piTexture me ) = 0;
    virtual void      AttachTextures( int num, piTexture vt0, piTexture vt1=0, piTexture vt2=0, piTexture vt3=0, piTexture vt4=0, piTexture vt5=0, piTexture vt6=0, piTexture vt7=0, piTexture vt8=0, piTexture vt9=0, piTexture vt10=0, piTexture vt11=0, piTexture vt12=0, piTexture vt13=0, piTexture vt14=0, piTexture vt15=0 ) = 0;
    virtual void      DettachTextures( void ) = 0;
    virtual piTexture CreateTextureFromID(unsigned int id, piTextureFilter filter) = 0;
    virtual void      MakeResident( piTexture vme ) = 0;
    virtual void      MakeNonResident( piTexture vme ) = 0;
    virtual uint64    GetTextureHandle( piTexture vme ) = 0;

    //--- samplers ---
    virtual piSampler CreateSampler(piTextureFilter filter, piTextureWrap wrap, float anisotropy) = 0;
    virtual void      DestroySampler( piSampler obj ) = 0;
    virtual void      AttachSamplers( int num, piSampler vt0, piSampler vt1=0, piSampler vt2=0, piSampler vt3=0, piSampler vt4=0, piSampler vt5=0, piSampler vt6=0, piSampler vt7=0 ) = 0;
    virtual void      DettachSamplers( void ) = 0;

    // --- images ---
    virtual void      AttachImage(int unit, piTexture texture, int level, bool layered, int layer, piTextureFormat format ) = 0;

    //--- shaders ---
    virtual piShader  CreateShader( const piShaderOptions *options, const char *vs, const char *cs, const char *es, const char *gs, const char *fs, char *error) = 0;
    virtual void      DestroyShader( piShader obj ) = 0;
    virtual void      AttachShader( piShader obj ) = 0;
    virtual void      DettachShader( void ) = 0;

    virtual piShader  CreateCompute(const piShaderOptions *options, const char *cs, char *error) = 0;

	virtual void      SetShaderConstant4F(const unsigned int pos, const float *value, int num) = 0;
	virtual void      SetShaderConstant3F(const unsigned int pos, const float *value, int num) = 0;
	virtual void      SetShaderConstant2F(const unsigned int pos, const float *value, int num) = 0;
	virtual void      SetShaderConstant1F(const unsigned int pos, const float *value, int num) = 0;
	virtual void      SetShaderConstant1I(const unsigned int pos, const int *value, int num) = 0;
	virtual void      SetShaderConstant1UI(const unsigned int pos, const unsigned int *value, int num) = 0;
	virtual void      SetShaderConstantMat4F(const unsigned int pos, const float *value, int num, bool transpose) = 0;
	virtual void      SetShaderConstantSampler(const unsigned int pos, int unit) = 0;
    virtual void      AttachShaderConstants(piBuffer obj, int unit) = 0;
    virtual void      AttachShaderBuffer(piBuffer obj, int unit) = 0;
    virtual void      DettachShaderBuffer(int unit) = 0;
    virtual void      AttachAtomicsBuffer(piBuffer obj, int unit) = 0;
    virtual void      DettachAtomicsBuffer(int unit) = 0;

    //--- buffers (vertex, index, shader constants, shader buffer, atomics, etc) ---
    virtual piBuffer  CreateBuffer(const void *data, unsigned int amount, piBufferType mode) = 0;
    virtual void      DestroyBuffer(piBuffer) = 0;
    virtual void      UpdateBuffer(piBuffer obj, const void *data, int offset, int len) = 0;

    //--- arrays ---
    //virtual piVertexArray CreateVertexArray(piBuffer vb, piBuffer eb, const piRArrayLayout *streamLayout ) = 0;
    virtual piVertexArray CreateVertexArray(int numStreams, piBuffer vb0, const piRArrayLayout *streamLayout0, piBuffer vb1, const piRArrayLayout *streamLayout1, piBuffer eb) = 0;
    virtual void      DestroyVertexArray(piVertexArray obj) = 0;
    virtual void      AttachVertexArray(piVertexArray obj) = 0;
    virtual void      DettachVertexArray(void) = 0;

    //--- misc ---

    virtual void      DrawPrimitiveIndexed(piPrimitiveType pt, int num, int numInstances, int baseVertex, int baseInstance) = 0;
    virtual void      DrawPrimitiveIndirect(piPrimitiveType pt, piBuffer cmds, int num) = 0;
    virtual void      DrawPrimitiveNotIndexed(piPrimitiveType pt, int first, int num, int numInstances) = 0;
    virtual void      DrawPrimitiveNotIndexedMultiple(piPrimitiveType pt, const int *firsts, const int *counts, int num) = 0;
    virtual void      DrawPrimitiveNotIndexedIndirect(piPrimitiveType pt, piBuffer cmds, int num) = 0;

/*
    virtual void      SetAttribute1F( int pos, const float data ) = 0;
    virtual void      SetAttribute2F( int pos, const float *data ) = 0;
    virtual void      SetAttribute3F( int pos, const float *data ) = 0;
    virtual void      SetAttribute4F( int pos, const float *data ) = 0;
*/
    virtual void      DrawUnitCube_XYZ_NOR(int numInstanced) = 0;
    virtual void      DrawUnitCube_XYZ(int numInstanced) = 0;
    virtual void      DrawUnitQuad_XY(int numInstanced) = 0;

    virtual void      ExecuteCompute( int ngx, int ngy, int ngz, int gsx, int gsy, int gsz ) = 0;


    //--- misc ---

    virtual void      SetPointSize( bool mode, float size ) = 0; // if false, the v/g shader decides
    virtual void      SetLineWidth( float size ) = 0;
    virtual void      PolygonOffset( bool mode, bool wireframe, float a, float b ) = 0;

    virtual void      RenderMemoryBarrier( piBarrierType type ) = 0;

};

} // namespace piLibs
