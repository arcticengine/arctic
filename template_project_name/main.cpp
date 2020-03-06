// Copyright (c) <year> Your name

#include "engine/easy.h"

using namespace arctic;  // NOLINT
using namespace arctic::easy;  // NOLINT

const Si32 kBlockSpriteCount = 3;
Sprite g_blocks[kBlockSpriteCount];
const Si32 kFieldWidth = 8;
const Si32 kFieldHeight = 16;
Si32 g_field[kFieldHeight][kFieldWidth] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {2, 0, 0, 0, 0, 0, 0, 2},
  {2, 0, 0, 0, 0, 0, 2, 2},
  {2, 0, 0, 0, 0, 2, 2, 2},
  {2, 0, 0, 0, 2, 2, 2, 2},
  {2, 0, 0, 0, 2, 2, 2, 2},
  {2, 0, 0, 0, 2, 2, 2, 2},
};
const Si32 kTetraminoSide = 5;
const Si32 kTetraminoCount = 7;
Si32 g_tetraminoes[kTetraminoCount * kTetraminoSide][kTetraminoSide] = {
  {0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 1, 1, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 1, 0, 0},
  {0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 1, 1, 1, 0},
  {0, 0, 1, 0, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0},
  {0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 1, 1, 0, 0},
  {0, 0, 1, 1, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},

  {0, 0, 0, 0, 0},
  {0, 0, 1, 1, 0},
  {0, 0, 1, 1, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
};
const Si32 kOrientationCount = 4;
Si32 g_current[kOrientationCount][kTetraminoSide][kTetraminoSide];
Si32 g_current_x;
Si32 g_current_y;
Si32 g_current_orientation;
double g_prev_time;
Si32 g_dx = 0;
Si32 g_dy = 0;
Si32 g_rotate = 0;
Si32 g_drop = 0;
Si32 g_score = 0;
Font g_font;
void StartNewTetramino() {
  Si32 idx = Random32(0, kTetraminoCount - 1);
  for (Si32 y = 0; y < kTetraminoSide; ++y) {
    for (Si32 x = 0; x < kTetraminoSide; ++x) {
      g_current[0][y][x] = g_tetraminoes[y + idx * kTetraminoSide][x];
      g_current[1][x][kTetraminoSide - 1 - y] = g_tetraminoes[y + idx * kTetraminoSide][x];
      g_current[2][kTetraminoSide - 1 - y][kTetraminoSide - 1 - x] = g_tetraminoes[y + idx * kTetraminoSide][x];
      g_current[3][kTetraminoSide - 1 - x][y] = g_tetraminoes[y + idx * kTetraminoSide][x];
    }
  }
  g_current_x = (kFieldWidth - kTetraminoSide + 1) / 2;
  g_current_y = 0;
  g_current_orientation = Random32(0, kOrientationCount - 1);
}
void ClearField() {
  for (Si32 y = 0; y < kFieldHeight; ++y) {
    for (Si32 x = 0; x < kFieldWidth; ++x) {
        g_field[y][x] = 0;
    }
  }
}
void Init() {
  g_blocks[1].Load("data/block_1.tga");
  g_blocks[2].Load("data/block_2.tga");
  g_blocks[0].Create(g_blocks[1].Size());
  g_blocks[1].Draw(g_blocks[0], 0, 0, kDrawBlendingModeColorize, kFilterNearest, Rgba(0, 0, 48));
  ResizeScreen(800, 500);
  StartNewTetramino();
  g_prev_time = Time();
  g_score = 0;
  g_font.Load("data/arctic_one_bmf.fnt");
}
bool IsPositionOk(Si32 test_x, Si32 test_y, Si32 test_orientation) {
  for (Si32 y = 0; y < kTetraminoSide; ++y) {
    for (Si32 x = 0; x < kTetraminoSide; ++x) {
      if (g_current[test_orientation][y][x]) {
        if (x + test_x < 0 || x + test_x >= kFieldWidth || y + test_y >= kFieldHeight
              || g_field[y + test_y][x + test_x]) {
          return false;
        }
      }
    }
  }
  return true;
}
void LockTetramino() {
  for (Si32 y = 0; y < kTetraminoSide; ++y) {
    for (Si32 x = 0; x < kTetraminoSide; ++x) {
      if (g_current[g_current_orientation][y][x]) {
        g_field[y + g_current_y][x + g_current_x] = 2;
      }
    }
  }
  bool do_continue = true;
  Si32 full_lines = 0;
  while (do_continue) {
    do_continue = false;
    for (Si32 y = kFieldHeight - 1; y >= 0; --y) {
      bool is_full_line = true;
      for (Si32 x = 0; x < kFieldWidth; ++x) {
        if (!g_field[y][x]) {
          is_full_line = false;
          break;
        }
      }
      if (is_full_line) {
        full_lines++;
        do_continue = true;
        for (Si32 y2 = y; y2 > 0; --y2) {
          for (Si32 x = 0; x < kFieldWidth; ++x) {
            g_field[y2][x] = g_field[y2 - 1][x];
          }
        }
      }
    }
  }
  g_score += full_lines * full_lines * 100;
}
void Update() {
  double time = Time();
  if (IsKeyDownward(kKeyLeft) || IsKeyDownward("a")) {
    g_dx = -1;
  } else if (IsKeyDownward(kKeyRight) || IsKeyDownward("d")) {
    g_dx = 1;
  }
  g_rotate = g_rotate || IsKeyDownward(kKeyUp)
    || IsKeyDownward("w") || IsKeyDownward(" ");
  g_drop = g_drop || IsKeyDownward(kKeyDown) || IsKeyDownward("s");
  if (IsKeyDownward("c")) {
    ClearField();
    StartNewTetramino();
    return;
  }
  if (time - g_prev_time >= 0.5) {
    g_dy = 1;
    g_prev_time = time;
  } else {
    g_dy = 0;
  }

  if (g_dx && IsPositionOk(g_current_x + g_dx,
        g_current_y, g_current_orientation)) {
    g_current_x += g_dx;
    g_dy = 0;
  } else {
    g_dx = 0;
  }
  if (g_rotate) {
    if (IsPositionOk(g_current_x, g_current_y,
          (g_current_orientation + 1) % kOrientationCount)) {
      g_current_orientation = (g_current_orientation + 1) % kOrientationCount;
      g_dy = 0;
    }
  }
  bool is_lock = false;
  if (IsPositionOk(g_current_x, g_current_y + (g_dy ? 1 : 0),
        g_current_orientation)) {
    g_current_y += (g_dy ? 1 : 0);
  } else {
    is_lock = (g_dx == 0);
  }
  if (g_drop) {
    while (IsPositionOk(g_current_x, g_current_y + 1, g_current_orientation)) {
      g_current_y++;
    }
  }
  g_dx = 0;
  g_dy = 0;
  g_drop = 0;
  g_rotate = 0;
  if (is_lock) {
    LockTetramino();
    StartNewTetramino();
  }
}
void Render() {
  Clear();
  Si32 x_offset = (ScreenSize().x - 25 * kFieldWidth) / 2;
  for (Si32 y = 0; y < kFieldHeight; ++y) {
    for (Si32 x = 0; x < kFieldWidth; ++x) {
      g_blocks[g_field[y][x]].Draw(x_offset + x * 25, (kFieldHeight - 1 - y) * 25);
    }
  }
  for (Si32 y = 0; y < kTetraminoSide; ++y) {
    for (Si32 x = 0; x < kTetraminoSide; ++x) {
      if (g_current[g_current_orientation][y][x]) {
        g_blocks[g_current[g_current_orientation][y][x]].Draw(
          x_offset + (x + g_current_x) * 25, (kFieldHeight - 1 - y - g_current_y) * 25);
      }
    }
  }
  char score[128];
  snprintf(score, sizeof(score), u8"Score: %d", g_score);
  g_font.Draw(score, 0, ScreenSize().y, kTextOriginTop);
  ShowFrame();
}
void EasyMain() {
  Init();
  while (!IsKeyDownward(kKeyEscape)) {
    Update();
    Render();
  }
}
