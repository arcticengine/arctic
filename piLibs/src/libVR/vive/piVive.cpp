#define DISABLEI

#include "../../libMath/piVecTypes.h"

#ifdef DISABLEI
#else
#include <openvr.h>
#endif

#include "piVive.h"

namespace piLibs {


bool piVRVive::Init(void)
{
    return true;
}

void piVRVive::DeInit(void)
{
}

//====================================================================

class piVRHMDVive : public piVRHMD
{
private:
    piVRHMD::HmdInfo    mData;
#ifdef DISABLEI
#else
    vr::IVRSystem      *mHMD;
#endif

public:

    bool Init(int deviceID, float pixelDensity);
    void DeInit(void);
    bool AttachToWindow(HmdInfo *info, bool createMirrorTexture, int mirrorTextureSizeX, int mirrorTextureSizeY);
    void BeginFrame(int *texIndexLeft, int *texIndexRight, bool *outNeedsMipMapping);
    void EndFrame(void);
};



static int imax( int a, int b ) { return (a>b)?a:b; }


piVRHMD *piVRVive::CreateHmd(int deviceID, float pixelDensity)
{
#ifdef DISABLEI
    return nullptr;
#else
    return new piVRHMDVive();
#endif
}

void piVRVive::DestroyHmd(piVRHMD * me)
{
    delete me;
}


//------------------------------------------------------------

bool piVRHMDVive::Init(int deviceID, float pixelDensity)
{
#ifdef DISABLEI
    return false;
#else
    vr::EVRInitError eError = vr::VRInitError_None;

    mHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
    if (eError != vr::VRInitError_None)
    {
        //char buf[1024]; sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        return false;
    }

    vr::IVRRenderModels *m_pRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
    if (!m_pRenderModels)
    {
        //char buf[1024]; sprintf_s(buf, sizeof(buf), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        return false;
    }

    return true;
#endif
}


void piVRHMDVive::DeInit( void )
{
#ifdef DISABLEI
#else
    vr::VR_Shutdown();
#endif
}


bool piVRHMDVive::AttachToWindow(HmdInfo *info, bool createMirrorTexture, int mirrorTextureSizeX, int mirrorTextureSizeY)
{
#ifdef DISABLEI
    return false;
#else
    //float m_fNearClip = 1.0f;
    //float m_fFarClip = 1000.0f;
    //vr::HmdMatrix44_t mat4ProjectionLeft  = mHMD->GetProjectionMatrix(vr::Eye_Left, m_fNearClip, m_fFarClip, vr::API_OpenGL);
    //vr::HmdMatrix44_t mat4ProjectionRight = mHMD->GetProjectionMatrix(vr::Eye_Right, m_fNearClip, m_fFarClip, vr::API_OpenGL);
    float fLeft, fRight, fTop, fBottom;
    mHMD->GetProjectionRaw( vr::Eye_Left,  &fLeft, &fRight, &fTop, &fBottom );
    mHMD->GetProjectionRaw( vr::Eye_Right, &fLeft, &fRight, &fTop, &fBottom );


    vr::HmdMatrix34_t mat4eyePosLeft = mHMD->GetEyeToHeadTransform(vr::Eye_Left);
    vr::HmdMatrix34_t mat4eyePosRight = mHMD->GetEyeToHeadTransform(vr::Eye_Right);

    uint32_t nRenderWidth, nRenderHeight;
    mHMD->GetRecommendedRenderTargetSize(&nRenderWidth, &nRenderHeight);

    return true;
#endif
}


void piVRHMDVive::BeginFrame( int *texIndexLeft, int *texIndexRight, bool *outNeedsMipMapping)
{
}

void piVRHMDVive::EndFrame( void )
{
#ifdef DISABLEI
#else
    unsigned int m_nResolveTextureId = 0;

    vr::Texture_t leftEyeTexture  = { (void*)m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
    vr::Texture_t rightEyeTexture = { (void*)m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };

    vr::VRCompositor()->Submit( vr::Eye_Left, &leftEyeTexture );
    vr::VRCompositor()->Submit( vr::Eye_Right, &rightEyeTexture );
#endif
}

}