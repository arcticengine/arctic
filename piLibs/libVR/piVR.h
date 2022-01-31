#pragma once

#include <functional>

namespace piLibs {

class piVRHMD
{
public:
    typedef struct
    {
        bool  mEnabled;
        float mLocation[16];
        float mVelocity[3];
        bool  mUpButtonPressed;
        bool  mUpButton_Down;
        bool  mUpButton_Up;
        bool  mDownButtonPressed;
        bool  mDownButton_Down;
        bool  mDownButton_Up;

        bool  mIndexTriggerPressed;
        float mIndexTrigger; // Left and right finger trigger values, in the range 0.0 to 1.0f.
        float mHandTrigger; // Left and right hand trigger values , in the range 0.0 to 1.0f.
        float mThumbstick[2]; // Horizontal and vertical thumbstick axis values , in the range -1.0f to 1.0f.

        int   mInternal;

        //void (*Vibrate)(int id, float frequency, float amplitude, float duration);
        std::function<void(int id, float frequency, float amplitude, float duration)> Vibrate;

    }Controller;

    typedef struct
    {
        float mCamera[16];
        float mProjection[4];
        Controller mController[2];
    }HeadInfo;

    typedef struct
    {
        int   mVP[4];
        float mProjection[4];
        float mCamera[16];
    }EyeInfo;

    typedef struct
    {
        int                 mNum;
        unsigned int        mTexID[64];
    }TextureChain;

    typedef struct
    {
        // init
        int          mXres;
        int          mYres;
        int          mVRXres;
        int          mVRYres;
        TextureChain mTexture[2];
        unsigned int mMirrorTexID;

        // per frame
        HeadInfo     mHead;
        EyeInfo      mEye[2];
    }HmdInfo;
    
    HmdInfo mInfo;

    virtual bool AttachToWindow( bool createMirrorTexture, int mirrorTextureSizeX, int mirrorTextureSizeY) = 0;
    virtual void BeginFrame( int *texIndexLeft, int *texIndexRight, bool *outNeedsMipMapping) = 0;
    virtual void EndFrame( void ) = 0;
    //virtual void Haptic(int id, float frequency, float amplitude, float duration) = 0;
	virtual ~piVRHMD() {};
};

class piVR
{
public:
    typedef enum
    {
        Oculus_Rift = 0,
        HTC_Vive = 1
    }HwType;

    static piVR *Create(HwType hw);
    static void  Destroy(piVR * me);

    virtual piVRHMD  *CreateHmd(int deviceID, float pixelDensity) = 0;
    virtual void      DestroyHmd(piVRHMD * me) = 0;
	virtual ~piVR() {};
};



} // namespace piLibs
