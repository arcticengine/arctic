#include "piVR.h"
#include "oculus/piOculus.h"
#include "vive/piVive.h"

namespace piLibs {

piVR * piVR::Create(HwType hw)
{
    piVR *me = nullptr;

    if( hw==Oculus_Rift )
    {
        piVROculus * ome = new piVROculus();
        if( !ome->Init() )
            return nullptr;
        me = ome;
    }

    if( hw==HTC_Vive)
    {
        piVRVive *ome = new piVRVive();
        if (!ome->Init())
            return nullptr;
        me = ome;
    }

    return me;
}

void piVR::Destroy(piVR * me)
{
    delete me;
}


}