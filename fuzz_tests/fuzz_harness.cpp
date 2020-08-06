#include <engine/easy.h>
#include <engine/easy_sound_instance.h>
#include <engine/arctic_types.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>

jmp_buf arctic_jmp_env;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) {

    void* fake_wav = calloc(size, 1);
    memcpy((char*)fake_wav, data, size);

    int val = setjmp(arctic_jmp_env);
    if(val == 1337)
    {
        free(fake_wav);
        return 0;
    }

    std::shared_ptr<arctic::SoundInstance> result = arctic::LoadWav((arctic::Ui8*)fake_wav, size);

    free(fake_wav);
    return 0;
}


