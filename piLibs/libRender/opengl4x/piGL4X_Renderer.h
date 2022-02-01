#pragma once

#include "../piRenderer.h"
#include "piGL4X_RenderContext.h"
#include "piGL4X_Ext.h"

namespace piLibs {

class piRendererGL4X : public piRenderer
{
public:
    piRendererGL4X();
    virtual ~piRendererGL4X();

    bool      Initialize(int id, const void **hwnd, int num, bool disableVSync, piRenderReporter *reporter);
    void      Deinitialize( void );
    void      Report( void );
    void      SetActiveWindow( int id );
    void      Enable(void);
    void      Disable(void);
	void	  SwapBuffers( void );

    //--- render targets ---
    piRTarget CreateRenderTarget(piTexture vtex0, piTexture vtex1, piTexture vtex2, piTexture vtex3, piTexture zbuf);
    void      DestroyRenderTarget( piRTarget obj );
    bool      SetRenderTarget( piRTarget obj );
    void      RenderTargetSampleLocations(piRTarget vdst, const float *locations);
    void      BlitRenderTarget( piRTarget dst, piRTarget src, bool color, bool depth );
    void      SetWriteMask( bool c0, bool c1, bool c2, bool c3, bool z );
    void      SetShadingSamples( int shadingSamples );
    void      RenderTargetGetDefaultSampleLocation(piRTarget vdst, const int id, float *location);

    //--- general ---
    void      Clear( const float *color0, const float *color1, const float *color2, const float *color3, const bool depth0 );
    void      SetState( piState state, bool value );
    void      SetBlending(int buf, piBlendEquation equRGB, piBlendOperations srcRGB, piBlendOperations dstRGB,
                                   piBlendEquation equALP, piBlendOperations srcALP, piBlendOperations dstALP);
    void      SetViewport(int id, const int *vp);

    //--- textures ---
    piTexture CreateTexture( const wchar_t *key, const piTextureInfo *info, piTextureFilter filter, piTextureWrap wrap, float aniso, void *buffer );
    void      DestroyTexture( piTexture obj );
    void      AttachTextures( int num, piTexture vt0, piTexture vt1, piTexture vt2, piTexture vt3, piTexture vt4, piTexture vt5, piTexture vt6, piTexture vt7, piTexture vt8, piTexture vt9, piTexture vt10, piTexture vt11, piTexture vt12, piTexture vt13, piTexture vt14, piTexture vt15 );
    void      DettachTextures( void );
    void      ClearTexture( piTexture vme, int level, const void *data );
    void      UpdateTexture( piTexture me, int x0, int y0, int z0, int xres, int yres, int zres, const void *buffer );
	void	  GetTextureRes( piTexture me, int *res );
    void      GetTextureFormat( piTexture me, piTextureFormat *format );
    void      GetTextureContent( piTexture me, void *data, const piTextureFormat fmt );
    void      GetTextureContent(piTexture vme, void *data, int x, int y, int z, int xres, int yres, int zres);
    void      GetTextureInfo( piTexture me, piTextureInfo *info );
    void      GetTextureSampling(piTexture vme, piTextureFilter *rfilter, piTextureWrap *rwrap);
    piTexture CreateTextureFromID(unsigned int id, piTextureFilter filter);
    void      ComputeMipmaps( piTexture me );
    void      MakeResident( piTexture vme );
    void      MakeNonResident( piTexture vme );
    uint64    GetTextureHandle( piTexture vme );


    piSampler CreateSampler(piTextureFilter filter, piTextureWrap wrap, float anisotropy);
    void      DestroySampler( piSampler obj );
    void      AttachSamplers(int num, piSampler vt0, piSampler vt1, piSampler vt2, piSampler vt3, piSampler vt4, piSampler vt5, piSampler vt6, piSampler vt7);
    void      DettachSamplers( void );

    void      AttachImage(int unit, piTexture texture, int level, bool layered, int layer, piTextureFormat format);

    //--- shaders ---
    piShader  CreateShader( const piShaderOptions *options, const char *vs, const char *cs, const char *es, const char *gs, const char *fs, char *error);
    void      DestroyShader( piShader obj );
    void      AttachShader( piShader obj );
    void      DettachShader( void );

    piShader  CreateCompute(const piShaderOptions *options, const char *cs, char *error);

    void      AttachShaderConstants(piBuffer obj, int unit);
	void      SetShaderConstant4F(const unsigned int pos, const float *value, int num);
	void      SetShaderConstant3F(const unsigned int pos, const float *value, int num);
	void      SetShaderConstant2F(const unsigned int pos, const float *value, int num);
	void      SetShaderConstant1F(const unsigned int pos, const float *value, int num);
	void      SetShaderConstant1I(const unsigned int pos, const int *value, int num);
	void      SetShaderConstant1UI(const unsigned int pos, const unsigned int *value, int num);
	void      SetShaderConstantMat4F(const unsigned int pos, const float *value, int num, bool transpose);
    void      SetShaderConstantSampler(const unsigned int pos, int unit);
    void      AttachShaderBuffer(piBuffer obj, int unit);
    void      DettachShaderBuffer(int unit);
    void      AttachAtomicsBuffer(piBuffer obj, int unit);
    void      DettachAtomicsBuffer(int unit);

    //--- buffers (vertex, index, shader constants) ---
    piBuffer  CreateBuffer(const void *data, unsigned int amount, piBufferType mode);
    void      DestroyBuffer(piBuffer);
    void      UpdateBuffer(piBuffer obj, const void *data, int offset, int len);

    //--- arrays ---
    piVertexArray CreateVertexArray(int numStreams, piBuffer vb0, const piRArrayLayout *streamLayout0, piBuffer vb1, const piRArrayLayout *streamLayout1, piBuffer eb);


    void      DestroyVertexArray(piVertexArray obj);
    void      AttachVertexArray(piVertexArray obj);
    void      DettachVertexArray(void);

    //--- misc ---

    void      DrawPrimitiveIndexed(piPrimitiveType pt, int num, int numInstances, int baseVertex, int baseInstance);
    void      DrawPrimitiveIndirect(piPrimitiveType pt, piBuffer cmds, int num);
    void      DrawPrimitiveNotIndexed(piPrimitiveType pt, int first, int num, int numInstanced);
    void      DrawPrimitiveNotIndexedMultiple(piPrimitiveType pt, const int *firsts, const int *counts, int num);
    void      DrawPrimitiveNotIndexedIndirect(piPrimitiveType pt, piBuffer cmds, int num);

/*
    void      SetAttribute1F( int pos, const float data );
    void      SetAttribute2F( int pos, const float *data );
    void      SetAttribute3F( int pos, const float *data );
    void      SetAttribute4F( int pos, const float *data );
*/
    void      DrawUnitCube_XYZ_NOR(int numInstanced);
    void      DrawUnitCube_XYZ(int numInstanced);
    void      DrawUnitQuad_XY(int numInstanced);

    void      ExecuteCompute(int ngx, int ngy, int ngz, int gsx, int gsy, int gsz);

    //--- misc ---

    void      SetPointSize( bool mode, float size ); // if false, the v/g shader decides
    void      SetLineWidth( float size );
    void      PolygonOffset( bool mode, bool wireframe, float a, float b );

    void      RenderMemoryBarrier(piBarrierType type);


private:
	void      PrintInfo( void );
	static void CALLBACK DebugLog( unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char *message , const void *userParam );

protected:
	piRenderReporter *mReporter;
    int               mID;
    NGLEXTINFO       *mExt;
    piRTarget         mBindedTarget;
	piGL4X_RenderContext *mRC;

    int      mMngTexMax;
    void    *mMngTexSlots;
    uint64      mMngTexMemCurrent;
    uint64      mMngTexMemPeak;
    int      mMngTexNumCurrent;
    int      mMngTexNumPeak;

	// aux geometry... (cubes, quads, etc)
    piVertexArray mVA[3];
    piBuffer      mVBO[3];
};

} // namespace piLibs