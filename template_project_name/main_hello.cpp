// Copyright (c) <year> Your name

#include "engine/easy.h"

using namespace arctic;  // NOLINT

Font g_font;

void EasyMain() {
  g_font.Load("data/arctic_one_bmf.fnt");
  while (!IsKeyDownward(kKeyEscape)) {
    Clear();
    char text[128];
    snprintf(text, sizeof(text), u8"Hello world!");
    g_font.Draw(text, 0, ScreenSize().y, kTextOriginTop);
    ShowFrame();
  }
}
