#include "../piSound.h"
#include "piSoundEngineDS.h"

namespace piLibs {

piSoundEngine::piSoundEngine()
{
}

piSoundEngine::~piSoundEngine()
{
}

piSoundEngine * piSoundEngine::Create(void)
{
    return new piSoundEngineDS();
}

}