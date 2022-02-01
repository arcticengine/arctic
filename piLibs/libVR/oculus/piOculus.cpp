#include "../../libSystem/piTimer.h"
#include "../../libSystem/piTypes.h"
#include "../../libMath/piVecTypes.h"

#if defined(OVR_OS_WIN32) || defined(OVR_OS_WIN64)
#include <windows.h>
#endif
#include "OVR_CAPI_GL.h"
#include <string.h>

#include "piOculus.h"

namespace piLibs {


bool piVROculus::Init(void)
{
    if (ovr_Initialize(nullptr) < 0)
        return false;

    return true;
}

void piVROculus::DeInit(void)
{
    ovr_Shutdown();
}

//====================================================================

class piVRHMDOculus : public piVRHMD
{
private:
    ovrLayerEyeFov      mLayer;
    bool                mIsVisible;
    ovrHmdDesc          mHmdDesc;
    ovrSession          mSession;
    uint64              mFrame;
    ovrTextureSwapChain mTextureChain[2];
    ovrSizei            mRecommendedTexSize[2];
    ovrMirrorTexture    mMirrorTexture;
    bool                mEnableMipmapping;
    struct HapticState
    {
        int    mState;
        float  mDuration;
        double mTime;
    }mHapticState[2];
    piTimer mTimer;

public:
    bool AttachToWindow( bool createMirrorTexture, int mirrorTextureSizeX, int mirrorTextureSizeY);
    void BeginFrame(int *texIndexLeft, int *texIndexRight, bool *outNeedsMipMapping);
    void EndFrame(void);
    //void Haptic(int id, float frequency, float amplitude, float duration);

private:
    void iHaptic(int id, float frequency, float amplitude, float duration);

friend class piVROculus;

private:
    bool Init(int deviceID, float pixelDensity);
    void DeInit(void);

};



static int imax( int a, int b ) { return (a>b)?a:b; }


piVRHMD *piVROculus::CreateHmd(int deviceID, float pixelDensity)
{
    piVRHMDOculus *me = new piVRHMDOculus();
    if( !me ) 
        return nullptr;

    if( !me->Init(deviceID, pixelDensity ) )
        return nullptr;

    return me;
}

void piVROculus::DestroyHmd(piVRHMD * vme)
{
    piVRHMDOculus *me = (piVRHMDOculus*)vme;
    me->DeInit();
    delete me;
}


//------------------------------------------------------------

void piVRHMDOculus::iHaptic(int id, float frequency, float amplitude, float duration)
{
    HapticState *hs = mHapticState + id;

    if( hs->mState==1 ) return;

    hs->mState = 1;
    hs->mDuration = duration;
    hs->mTime = mTimer.GetTime();

    const ovrControllerType ct = (id == 0) ? ovrControllerType_LTouch : ovrControllerType_RTouch;
    ovr_SetControllerVibration(mSession, ct, frequency, amplitude);
}

bool piVRHMDOculus::Init(int deviceID, float pixelDensity)
{
    if( !mTimer.Init() )
        return false;

    ovrGraphicsLuid luid;
    ovrResult res = ovr_Create(&mSession, &luid);
    if (!OVR_SUCCESS(res))
        return false;

    mFrame = 0;
    mHmdDesc = ovr_GetHmdDesc(mSession);
    mInfo.mXres = mHmdDesc.Resolution.w;
    mInfo.mYres = mHmdDesc.Resolution.h;
    mEnableMipmapping = (pixelDensity>1.0f);

    for (int i = 0; i<2; i++)
    {
        mRecommendedTexSize[i] = ovr_GetFovTextureSize(mSession, ovrEyeType(i), mHmdDesc.DefaultEyeFov[i], pixelDensity);
    }

    mInfo.mVRXres = mRecommendedTexSize[0].w;
    mInfo.mVRYres = mRecommendedTexSize[0].h;

    for (int i = 0; i<2; i++)
    {
        mInfo.mHead.mController[i].mInternal = 0;
        mInfo.mHead.mController[i].mUpButtonPressed = false;
        mInfo.mHead.mController[i].mUpButton_Down = false;
        mInfo.mHead.mController[i].mUpButton_Up = false;
        mInfo.mHead.mController[i].mDownButtonPressed = false;
        mInfo.mHead.mController[i].mDownButton_Down = false;
        mInfo.mHead.mController[i].mDownButton_Up = false;
        mInfo.mHead.mController[i].Vibrate = [&]( int id, float frequency, float amplitude, float duration) 
        {
            iHaptic(id, frequency, amplitude, duration );
        };
        mHapticState[i].mState = 0;
    }

    return true;
}


void piVRHMDOculus::DeInit( void )
{
}


bool piVRHMDOculus::AttachToWindow( bool createMirrorTexture, int mirrorTextureSizeX, int mirrorTextureSizeY)
{
    if( createMirrorTexture )
    {
    ovrMirrorTextureDesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = mirrorTextureSizeX;
    desc.Height = mirrorTextureSizeY;
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    ovrResult result = ovr_CreateMirrorTextureGL(mSession, &desc, &mMirrorTexture);
    if (!OVR_SUCCESS(result))
        return false;
    ovr_GetMirrorTextureBufferGL(mSession, mMirrorTexture, &mInfo.mMirrorTexID);
    }

    for( int i=0; i<2; i++ )
    {
        ovrTextureSwapChainDesc desc = {};
        desc.Type = ovrTexture_2D;
        desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc.ArraySize = 1;
        desc.Width = mRecommendedTexSize[i].w;
        desc.Height = mRecommendedTexSize[i].h;
        desc.MipLevels = (mEnableMipmapping==true) ? 3 : 1;
        desc.SampleCount = 1;
        desc.StaticImage = ovrFalse;

        ovrResult result = ovr_CreateTextureSwapChainGL(mSession, &desc, &mTextureChain[i]);
        if( !OVR_SUCCESS(result) )
            return false;

        int length = 0;
        ovr_GetTextureSwapChainLength(mSession, mTextureChain[i], &length);

        mInfo.mTexture[i].mNum = length;

        for( int j=0; j<length; j++ )
        {
            unsigned int chainTexId;
            ovr_GetTextureSwapChainBufferGL(mSession, mTextureChain[i], j, &chainTexId);
            mInfo.mTexture[i].mTexID[j] = chainTexId;
        }

        mLayer.ColorTexture[i] = mTextureChain[i];
        mLayer.Viewport[i] = { { 0, 0 }, { mRecommendedTexSize[i].w, mRecommendedTexSize[i].h } };
    }


    // FloorLevel will give tracking poses where the floor height is 0
    ovr_SetTrackingOriginType( mSession, ovrTrackingOrigin_FloorLevel );


    // Initialize our single full screen Fov layer.
    mLayer.Header.Type = ovrLayerType_EyeFov;
    mLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft | ovrLayerFlag_HighQuality;
    return true;
}


void piVRHMDOculus::BeginFrame( int *texIndexLeft, int *texIndexRight, bool *outNeedsMipMapping)
{
    // take care of haptics first
    for( int i=0; i<2; i++ )
    {
        HapticState *hs = mHapticState + i;
        if( hs->mState==1 )
        {
            const float dt = float(mTimer.GetTime() - hs->mTime );
            if( dt > hs->mDuration )
            {
                const ovrControllerType ct = (i==0) ? ovrControllerType_LTouch : ovrControllerType_RTouch;
                ovr_SetControllerVibration( mSession, ct, 0.0f, 0.0f );
                hs->mState = 0;
            }
        }
    }

    // go on with rendering

    ovrEyeRenderDesc    eyeRenderDesc[2];

    eyeRenderDesc[0] = ovr_GetRenderDesc( mSession, ovrEye_Left,  mHmdDesc.DefaultEyeFov[0] );
    eyeRenderDesc[1] = ovr_GetRenderDesc( mSession, ovrEye_Right, mHmdDesc.DefaultEyeFov[1] );

    ovrVector3f HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset, eyeRenderDesc[1].HmdToEyeOffset };

    double   sensorSampleTime;
    ovrPosef EyeRenderPose[2];
    ovr_GetEyePoses(mSession, mFrame, ovrTrue, HmdToEyeOffset, EyeRenderPose, &sensorSampleTime);

    mLayer.Fov[0] = eyeRenderDesc[0].Fov;
    mLayer.Fov[1] = eyeRenderDesc[1].Fov;
    mLayer.RenderPose[0] = EyeRenderPose[0];
    mLayer.RenderPose[1] = EyeRenderPose[1];
    mLayer.SensorSampleTime = sensorSampleTime;


    // Get both eye poses simultaneously, with IPD offset already included.
    double ftiming = ovr_GetPredictedDisplayTime(mSession, 0);
    double displayMidpointSeconds = ovr_GetTimeInSeconds();//GetPredictedDisplayTime(mHMD, 0);
    ovrTrackingState hmdState = ovr_GetTrackingState(mSession, displayMidpointSeconds, ovrTrue);
    ovr_CalcEyePoses(hmdState.HeadPose.ThePose, HmdToEyeOffset, mLayer.RenderPose);
   

    mat4x4 rot = setRotationQuaternion(vec4(&hmdState.HeadPose.ThePose.Orientation.x));
    mat4x4 tra = setTranslation(-hmdState.HeadPose.ThePose.Position.x, -hmdState.HeadPose.ThePose.Position.y, -hmdState.HeadPose.ThePose.Position.z);
    mat4x4 tmp = transpose(rot) * tra;
    memcpy(mInfo.mHead.mCamera, &tmp, 16 * sizeof(float));

    ovrInputState inputState;
    if (ovr_GetInputState(mSession, ovrControllerType_Touch, &inputState) >= 0)
    {
        mInfo.mHead.mController[0].mEnabled = ((inputState.ControllerType & ovrControllerType_LTouch) != 0);
        if (mInfo.mHead.mController[0].mEnabled )
        {
            mInfo.mHead.mController[0].mThumbstick[0] = inputState.Thumbstick[ovrHand_Left].x;
            mInfo.mHead.mController[0].mThumbstick[1] = inputState.Thumbstick[ovrHand_Left].y;
            mInfo.mHead.mController[0].mIndexTrigger = inputState.IndexTrigger[ovrHand_Left];
            mInfo.mHead.mController[0].mHandTrigger = inputState.HandTrigger[ovrHand_Left];
            bool oldUpButton = mInfo.mHead.mController[0].mUpButtonPressed;
            mInfo.mHead.mController[0].mUpButtonPressed = (inputState.Buttons & ovrButton_Y) != 0;
            mInfo.mHead.mController[0].mUpButton_Down = (!oldUpButton &&  mInfo.mHead.mController[0].mUpButtonPressed);
            mInfo.mHead.mController[0].mUpButton_Up   = ( oldUpButton && !mInfo.mHead.mController[0].mUpButtonPressed);
            bool oldDownButton = mInfo.mHead.mController[0].mDownButtonPressed;
            mInfo.mHead.mController[0].mDownButtonPressed = (inputState.Buttons & ovrButton_X) != 0;
            mInfo.mHead.mController[0].mDownButton_Down = (!oldDownButton &&  mInfo.mHead.mController[0].mDownButtonPressed);
            mInfo.mHead.mController[0].mDownButton_Up = (oldDownButton && !mInfo.mHead.mController[0].mDownButtonPressed);
        }

        mInfo.mHead.mController[1].mEnabled = ((inputState.ControllerType & ovrControllerType_RTouch) != 0);
        if (mInfo.mHead.mController[1].mEnabled)
        {
            mInfo.mHead.mController[1].mEnabled = true;
            mInfo.mHead.mController[1].mThumbstick[0] = inputState.Thumbstick[ovrHand_Right].x;
            mInfo.mHead.mController[1].mThumbstick[1] = inputState.Thumbstick[ovrHand_Right].y;
            mInfo.mHead.mController[1].mIndexTrigger = inputState.IndexTrigger[ovrHand_Right];
            mInfo.mHead.mController[1].mHandTrigger = inputState.HandTrigger[ovrHand_Right];
            bool oldUpButton = mInfo.mHead.mController[1].mUpButtonPressed;
            mInfo.mHead.mController[1].mUpButtonPressed = (inputState.Buttons & ovrButton_B) != 0;
            mInfo.mHead.mController[1].mUpButton_Down = (!oldUpButton &&  mInfo.mHead.mController[1].mUpButtonPressed);
            mInfo.mHead.mController[1].mUpButton_Up = (oldUpButton && !mInfo.mHead.mController[1].mUpButtonPressed);
            bool oldDownButton = mInfo.mHead.mController[1].mDownButtonPressed;
            mInfo.mHead.mController[1].mDownButtonPressed = (inputState.Buttons & ovrButton_A) != 0;
            mInfo.mHead.mController[1].mDownButton_Down = (!oldDownButton &&  mInfo.mHead.mController[1].mDownButtonPressed);
            mInfo.mHead.mController[1].mDownButton_Up = (oldDownButton && !mInfo.mHead.mController[1].mDownButtonPressed);
        }
    }
    else
    {
        mInfo.mHead.mController[0].mEnabled = false;
        mInfo.mHead.mController[1].mEnabled = false;
    }

    for( int i=0; i<2; i++ )
    {
        ovrPosef ph0 = hmdState.HandPoses[i].ThePose;

        ovrVector3f vel = hmdState.HandPoses[i].LinearVelocity;

        mat4x4 rot = setRotationQuaternion(vec4(&ph0.Orientation.x));
        mat4x4 tra = setTranslation(ph0.Position.x, ph0.Position.y, ph0.Position.z);
        mat4x4 tmp = tra * rot;
        memcpy(mInfo.mHead.mController[i].mLocation, &tmp, 16 * sizeof(float));
        mInfo.mHead.mController[i].mVelocity[0] = vel.x;
        mInfo.mHead.mController[i].mVelocity[1] = vel.y;
        mInfo.mHead.mController[i].mVelocity[2] = vel.z;
    }

    //ovrSwapTextureSet *ts = mTextureSet;

    //if (isVisible)
    {
        //ts->CurrentIndex = (ts->CurrentIndex + 1) % ts->TextureCount;

        for( int eyeID = 0; eyeID<2; eyeID++ )
        {
            mat4x4 rot = setRotationQuaternion(vec4(&mLayer.RenderPose[eyeID].Orientation.x));
            mat4x4 tra = setTranslation(-mLayer.RenderPose[eyeID].Position.x, -mLayer.RenderPose[eyeID].Position.y, -mLayer.RenderPose[eyeID].Position.z);
            mat4x4 tmp = transpose(rot) * tra;
            memcpy(mInfo.mEye[eyeID].mCamera, &tmp, 16 * sizeof(float));

            mInfo.mEye[eyeID].mProjection[0] = mLayer.Fov[eyeID].UpTan;
            mInfo.mEye[eyeID].mProjection[1] = mLayer.Fov[eyeID].DownTan;
            mInfo.mEye[eyeID].mProjection[2] = mLayer.Fov[eyeID].LeftTan;
            mInfo.mEye[eyeID].mProjection[3] = mLayer.Fov[eyeID].RightTan;

            mInfo.mEye[eyeID].mVP[0] = mLayer.Viewport[eyeID].Pos.x;
            mInfo.mEye[eyeID].mVP[1] = mLayer.Viewport[eyeID].Pos.y;
            mInfo.mEye[eyeID].mVP[2] = mLayer.Viewport[eyeID].Size.w;
            mInfo.mEye[eyeID].mVP[3] = mLayer.Viewport[eyeID].Size.h;
        }
    }


    mInfo.mHead.mProjection[0] = mLayer.Fov[0].UpTan;
    mInfo.mHead.mProjection[1] = mLayer.Fov[0].DownTan;
    mInfo.mHead.mProjection[2] = mLayer.Fov[0].LeftTan;
    mInfo.mHead.mProjection[3] = mLayer.Fov[1].RightTan;

    *outNeedsMipMapping = mEnableMipmapping;
    ovr_GetTextureSwapChainCurrentIndex(mSession, mTextureChain[0], texIndexLeft);
    ovr_GetTextureSwapChainCurrentIndex(mSession, mTextureChain[1], texIndexRight);
}

void piVRHMDOculus::EndFrame( void )
{
    for( int i=0; i<2; i++ )
    {
        ovr_CommitTextureSwapChain( mSession, mTextureChain[i] );
    }

    // Submit frame with one layer we have.
    ovrLayerHeader* layers = &mLayer.Header;
    ovrResult       result = ovr_SubmitFrame(mSession, mFrame, nullptr, &layers, 1);

    if( !OVR_SUCCESS(result) )
        return;

    mIsVisible = (result == ovrSuccess);

    ovrSessionStatus sessionStatus;
    ovr_GetSessionStatus( mSession, &sessionStatus);

    if( sessionStatus.ShouldQuit ) return;
    if( sessionStatus.ShouldRecenter ) ovr_RecenterTrackingOrigin(mSession);
    //sessionStatus.HmdPresent
    //sessionStatus.DisplayLost
    //sessionStatus.HmdMounted
    //sessionStatus.IsVisible


    mFrame++;

}

/*
void piVRHMDOculus::Haptic( int id, float frequency, float amplitude, float duration )
{
    mHapticState[id].mState = 1;
    mHapticState[id].mDuration = duration;
    mHapticState[id].mTime = piTimer_GetTime();
    const ovrControllerType ct = (id==0) ? ovrControllerType_LTouch : ovrControllerType_RTouch;
    ovr_SetControllerVibration(mSession, ct, frequency, amplitude);
}*/


}