#include <engine/easy_sprite.h>
#include <engine/arctic_types.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>

jmp_buf arctic_jmp_env;
using namespace arctic;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  int val = setjmp(arctic_jmp_env);
  if(val == 1337) {
    return 0;
  }
  Sprite s;
  s.LoadFromData(data, size, "a.tga");
  return 0;
}


