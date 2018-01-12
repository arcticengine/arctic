// The MIT License(MIT)
//
// Copyright 2016-2017 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include <time.h>
#include <algorithm>
#include <cstring>
#include <deque>
#include <utility>
#include <vector>
#include "engine/easy.h"
#include "engine/font.h"

using namespace arctic;  // NOLINT
using namespace arctic::easy;  // NOLINT

Font g_font;

Sprite g_blood[7];
Sprite g_floor;
Sprite g_floor_dark;
Sprite g_hero[2];
Sprite g_intro_airplane;
Sprite g_intro_pyramids;

Sprite g_monster[3];

Sprite g_stairs_down_left;
Sprite g_stairs_down_left_dark;
Sprite g_stairs_down_right;
Sprite g_stairs_down_right_dark;
Sprite g_stairs_up_left;
Sprite g_stairs_up_left_dark;
Sprite g_stairs_up_right;
Sprite g_stairs_up_right_dark;
Sprite g_stick;
Sprite g_stone;
Sprite g_wall;
Sprite g_wall_dark;

Sprite g_empty;

enum CellKind {
  kCellWall = 0,
  kCellFloor,
  kCellStairsDownLeft,
  kCellStairsDownRight,
  kCellStairsUpLeft,
  kCellStairsUpRight,
};

bool IsHigh(CellKind kind) {
  if (kind == kCellWall) {
    return true;
  } else {
    return false;
  }
}

Sprite& CellSprite(CellKind kind, bool is_visible) {
  switch (kind) {
  case kCellFloor:
    if (is_visible) {
      return g_floor;
    } else {
      return g_floor_dark;
    }
  case kCellStairsDownLeft:
    if (is_visible) {
      return g_stairs_down_left;
    } else {
      return g_stairs_down_left_dark;
    }
  case kCellStairsDownRight:
    if (is_visible) {
      return g_stairs_down_right;
    } else {
      return g_stairs_down_right_dark;
    }
  case kCellStairsUpLeft:
    if (is_visible) {
      return g_stairs_up_left;
    } else {
      return g_stairs_up_left_dark;
    }
  case kCellStairsUpRight:
    if (is_visible) {
      return g_stairs_up_right;
    } else {
      return g_stairs_up_right_dark;
    }
  case kCellWall:
    if (is_visible) {
      return g_wall;
    } else {
      return g_wall_dark;
    }
  default:
    Fatal("Unknown cell kind in CellSprite");
    return g_empty;
  }
}

enum ItemKind {
  kItemNone = 0,
  kItemStone,
  kItemStick,
  kItemKindCount
};

Sprite& ItemSprite(ItemKind kind) {
  switch (kind) {
  case kItemNone:
    return g_empty;
  case kItemStone:
    return g_stone;
  case kItemStick:
    return g_stick;
  default:
    Fatal("Unknown item kind in ItemSprite");
    return g_empty;
  }
}

enum CreatureKind {
  kCreatureMonsterBegin = 0,
  kCreatureWinged = 0,
  kCreatureTall = 1,
  kCreatureFat = 2,
  kCreatureMonsterEnd = 3,
  kCreatureHeroBegin = 4,
  kCreatureMale = 4,
  kCreatureFemale = 5,
  kCreatureHeroEnd = 6,
  kCreatureCount = 6
};

Sprite& CreatureSprite(CreatureKind kind) {
  if (kind >= kCreatureMonsterBegin && kind < kCreatureMonsterEnd) {
    return g_monster[kind - kCreatureMonsterBegin];
  } else if (kind >= kCreatureHeroBegin && kind < kCreatureHeroEnd) {
    return g_hero[kind - kCreatureHeroBegin];
  } else {
    Fatal("Unknown creature kind in CreatureSprite.");
    return g_empty;
  }
}

enum DecalKind {
  kDecalBlood0 = 0,
  kDecalBlood1,
  kDecalBlood2,
  kDecalBlood3,
  kDecalBlood4,
  kDecalBlood5,
  kDecalBlood6,
  kDecalCount
};

Sprite& DecalSprite(DecalKind kind) {
  if (kind < kDecalCount) {
    return g_blood[kind];
  } else {
    Fatal("Unknown decal kind in DecalSprite.");
    return g_empty;
  }
}

struct Creature {
  CreatureKind kind = kCreatureMale;
  Vec2Si32 pos = Vec2Si32(1, 1);
  Vec2Si32 next_pos = Vec2Si32(1, 1);
  double move_start_at = 0.0;
  double move_part = 0.0;
  bool is_moving = false;
  std::vector<Si32> items;
};

struct Item {
  ItemKind kind = kItemNone;
};

struct Cell {
  CellKind kind = kCellWall;
  bool is_known = false;
  bool is_visible = false;
  std::vector<Item> items;
  std::vector<DecalKind> decals;
};

bool g_is_first_level = true;
CellKind g_upper_cell_kind = kCellStairsDownLeft;
Vec2Si32 g_maze_size(32, 20);
Cell g_maze[32][20];

std::vector<Creature> g_creatures;
Si32 g_hero_idx = 0;

void InitCreatures() {
  g_creatures.clear();
  Creature hero;
  g_hero_idx = static_cast<Si32>(g_creatures.size());
  g_creatures.push_back(hero);
}

Creature& Hero() {
  return g_creatures[g_hero_idx];
}

Cell& Maze(Vec2Si32 pos) {
  Check(pos.x >= 0 && pos.y >= 0 &&
    pos.x < g_maze_size.x && pos.y < g_maze_size.y,
    "pos out of bounds in Maze");
  return g_maze[pos.x][pos.y];
}

void StepMazeGeneration(Vec2Si32 from, Vec2Si32 size) {
  Maze(from).kind = kCellFloor;
  Vec2Si32 direction[4] = {Vec2Si32(-1, 0)
    , Vec2Si32(1, 0)
      , Vec2Si32(0, -1)
      , Vec2Si32(0, 1)};
  for (Si32 variants = 4; variants > 0; --variants) {
    Si32 idx = Random32(0, variants - 1);
    Vec2Si32 dir = direction[idx];
    Vec2Si32 path = from + dir;
    Vec2Si32 to = path + dir;
    if (to.x > 0 && to.x < size.x - 1 && to.y > 0 && to.y < size.y - 1) {
      if (Maze(to).kind == kCellWall) {
        Maze(path).kind = kCellFloor;
        StepMazeGeneration(to, size);
      }
    }
    direction[idx] = direction[variants - 1];
  }
}

bool IsDeadEnd(Si32 x, Si32 y) {
  Vec2Si32 pos(x, y);
  Vec2Si32 dir[4] = {Vec2Si32(-1, 0)
    , Vec2Si32(1, 0)
      , Vec2Si32(0, -1)
      , Vec2Si32(0, 1)};
  if (Maze(pos).kind == kCellFloor) {
    Si32 exits = 0;
    for (Si32 idx = 0; idx < 4; ++idx) {
      if (Maze(pos + dir[idx]).kind != kCellWall) {
        exits++;
      }
    }
    if (exits == 1) {
      return true;
    }
  }
  return false;
}

std::deque<Vec2Si32> FindDeadEnds(Vec2Si32 size) {
  std::deque<Vec2Si32> res;
  Vec2Si32 max(size.x - 1, size.y - 1);
  for (Si32 x = 1; x < max.x; ++x) {
    for (Si32 y = 1; y < max.y; ++y) {
      if (IsDeadEnd(x, y)) {
        res.emplace_back(x, y);
      }
    }
  }
  return res;
}

void EliminateDeadEnd(Vec2Si32 pos) {
  Vec2Si32 dir[4] = {Vec2Si32(-1, 0)
    , Vec2Si32(1, 0)
      , Vec2Si32(0, -1)
      , Vec2Si32(0, 1)};
  while (true) {
    for (Creature &c : g_creatures) {
      if (pos == c.pos) {
        return;
      }
    }
    if (Maze(pos).kind != kCellFloor) {
      return;
    }
    Si32 exits = 0;
    Vec2Si32 exit;
    for (Si32 idx = 0; idx < 4; ++idx) {
      if (Maze(pos + dir[idx]).kind != kCellWall) {
        exits++;
        exit = pos + dir[idx];
      }
    }
    if (exits != 1) {
      return;
    }
    Maze(pos).kind = kCellWall;
    pos = exit;
  }
}

void CycleDeadEnd(Vec2Si32 pos) {
  Vec2Si32 dir[4] = {Vec2Si32(-1, 0)
    , Vec2Si32(1, 0)
      , Vec2Si32(0, -1)
      , Vec2Si32(0, 1)};

  if (Maze(pos).kind != kCellFloor) {
    return;
  }
  Si64 rnd_dir = Random(0, 3);
  for (Si32 i = 0; i < 4; ++i) {
    Si32 idx = (i + rnd_dir) % 4;
    Vec2Si32 p = pos + dir[idx];
    if (p.x > 0 && p.x < g_maze_size.x - 1 &&
      p.y > 0 && p.y < g_maze_size.y) {
      Cell &cell = Maze(p);
      if (cell.kind == kCellWall) {
        cell.kind = kCellFloor;
        return;
      }
    }
  }
}


Sound g_music;

void PlayIntro() {
  ResizeScreen(10, 10);
  Sprite sp0;
  sp0.Create(ScreenSize().x, ScreenSize().y);
  while (!IsAnyKeyDownward()) {
    Si32 x = MousePos().x;
    Si32 y = MousePos().y;
    x = Clamp(x, 0, sp0.Width() - 1);
    y = Clamp(y, 0, sp0.Height() - 1);
    if (IsKeyDown(kKeyMouseLeft)) {
      sp0.RgbaData()[sp0.StridePixels() * y + x].rgba =
        Rgba(255, 0, 0, 255).rgba;
    } else {
      sp0.RgbaData()[sp0.StridePixels() * y + x].rgba =
        Rgba(255, 255, 255, 255).rgba;
    }
    sp0.Draw(0, 0);

    ShowFrame();
  }
  ResizeScreen(320, 200);
  sp0.Create(320, 200);

  char data[10] = "123456789";
  WriteFile("data/test.data", reinterpret_cast<Ui8*>(data), 10);
  auto data2 = ReadFile("data/test.data");
  Check(data2.size() == 10, "string size mismatch");
  Check(strncmp(data, reinterpret_cast<char*>(data2.data()), 10) == 0,
    "strings do not match");

  g_music.Load("data/snowflake_-_Living_Nightmare.ogg", false);
  g_music.Play();

  Rgba *rgba = sp0.RgbaData();
  for (Si32 y = 0; y < 200; ++y) {
    for (Si32 x = 0; x < 320; x++) {
      Si32 a = Clamp(x - 35, 0, 255);
      rgba[x + y * 320] = Rgba(255, 0, 255, a);
    }
  }
  Sprite sp;
  sp.Clone(sp0);
  sp0.Clear(Rgba(128, 128, 128, 128));

  Ui8 snow[2][320 * 200];
  for (Si32 i = 0; i < 320 * 200; ++i) {
    if (Random(0, 15) == 0) {
      snow[0][i] = static_cast<Ui8>(Random(0, 255));
    } else {
      snow[0][i] = 0;
    }
  }
  Ui8 *cur_snow = snow[0];
  Ui8 *next_snow = snow[1];

  Vec2Si32 pyramids_pos(0, 10);

  Vec2Si32 airplane_pos_begin(ScreenSize().x,
    ScreenSize().y - g_intro_airplane.Height() / 2);
  Vec2Si32 airplane_pos_end(-g_intro_airplane.Width(), 0);
  Si32 duration1 = 380;
  Si32 duration2 = 500;
  Si32 duration3 = 560;
  double start_time = Time();
  Si32 frame = 0;
  while (true) {
    double time = Time();
    frame = static_cast<Si32>((time - start_time) * 60.f);
    Clear();
    if (IsKeyDown(kKeyEscape) || IsKeyDown(kKeySpace)
      || IsKeyDown(kKeyEnter)) {
      return;
    }
    if (frame > duration3) {
      break;
    }
    if (frame > duration1 && frame < duration2) {
      pyramids_pos = Vec2Si32(Random32(-1, 1), 10 + Random32(-1, 1));
    }
    g_intro_pyramids.Draw(pyramids_pos);
    if (frame < duration1) {
      Vec2Si32 airplane_pos = airplane_pos_begin +
        (airplane_pos_end - airplane_pos_begin) * frame / duration1;
      g_intro_airplane.Draw(airplane_pos);
    }

    Rgba *back_buffer = GetEngine()->GetBackbuffer().RgbaData();
    memset(next_snow, 0, 320 * 200);
    for (Si32 y = 10; y < 190; ++y) {
      for (Si32 x = 0; x < 320; ++x) {
        Si32 z = cur_snow[x + y * 320];
        if (z) {
          Si32 next_x = x + 8 - z / 42;
          Si32 next_y = y - 1;
          if (next_x >= 320) {
            next_x -= 320;
          }
          if (next_y < 10) {
            next_y = 189;
          }
          if (next_snow[next_x + next_y * 320] == 0
            || next_snow[next_x + next_y * 320] > z) {
            next_snow[next_x + next_y * 320] = z;
          }
          if (Random(0, 15) == 0) {
            Si32 z2 = Random32(0, 255);
            if (z2 > z) {
              next_snow[x + y * 320] = z2;
            }
          }
          back_buffer[x + y * 320] =
            Rgba(255 - z / 8, 255 - z / 8, 255 - z / 8);
        }
      }
    }
    std::swap(cur_snow, next_snow);
    sp.Draw(0, 0);
    ShowFrame();
  }
}

void GenerateMaze() {
  Vec2Si32 pos;
  for (pos.x = 0; pos.x < g_maze_size.x; ++pos.x) {
    for (pos.y = 0; pos.y < g_maze_size.y; ++pos.y) {
      Maze(pos).kind = kCellWall;
      Maze(pos).is_visible = false;
      Maze(pos).is_known = false;
    }
  }

  StepMazeGeneration(g_creatures[g_hero_idx].pos, g_maze_size);

  if (!g_is_first_level) {
    if (g_upper_cell_kind == kCellStairsDownRight) {
      Maze(Hero().pos).kind = kCellStairsUpRight;
    } else {
      Maze(Hero().pos).kind = kCellStairsUpLeft;
    }
  }

  std::deque<Vec2Si32> dead_ends = FindDeadEnds(g_maze_size);
  int attempt = 0;
  while (true) {
    attempt++;
    Si32 rnd = Random32(0, static_cast<Si32>(dead_ends.size() - 1));
    Vec2Si32 pos = dead_ends[rnd];
    bool is_ok = false;
    if (Maze(pos + Vec2Si32(-1, 0)).kind == kCellFloor) {
      is_ok = true;
      Maze(pos).kind = kCellStairsDownLeft;
    } else if (Maze(pos + Vec2Si32(1, 0)).kind == kCellFloor
      || attempt > 10) {
      is_ok = true;
      Maze(pos).kind = kCellStairsDownRight;
    }
    if (is_ok) {
      dead_ends[rnd] = dead_ends.back();
      dead_ends.pop_back();
      break;
    }
  }

  Si32 to_eliminate = static_cast<Si32>(dead_ends.size() / 2);
  for (Si32 idx = 0; idx < to_eliminate; ++idx) {
    Si32 rnd = Random32(0, static_cast<Si32>(dead_ends.size() - 1));
    EliminateDeadEnd(dead_ends[rnd]);
    dead_ends[rnd] = dead_ends.back();
    dead_ends.pop_back();
  }
  Si32 to_cycle = static_cast<Si32>(dead_ends.size() / 2);
  for (Si32 idx = 0; idx < to_cycle; ++idx) {
    Si32 rnd = Random32(0, static_cast<Si32>(dead_ends.size() - 1));
    CycleDeadEnd(dead_ends[rnd]);
    dead_ends[rnd] = dead_ends.back();
    dead_ends.pop_back();
  }
  for (Ui32 idx = 0; idx < dead_ends.size(); ++idx) {
    if (Hero().pos == dead_ends[idx]) {
      continue;
    }
    Cell &cell = Maze(dead_ends[idx]);
    Item item;
    Ui64 rnd = Random(0, 255);
    if (rnd < 32) {
      item.kind = kItemStick;
    } else if (rnd < 128) {
      item.kind = kItemStone;
    } else {
      item.kind = kItemNone;
    }
    if (item.kind != kItemNone) {
      cell.items.push_back(item);
    }
  }
}

void Init() {
  g_font.Load("data/arctic_one_bmf.fnt");

  g_blood[0].Load("data/blood_0.tga");
  g_blood[1].Load("data/blood_1.tga");
  g_blood[2].Load("data/blood_2.tga");
  g_blood[3].Load("data/blood_3.tga");
  g_blood[4].Load("data/blood_4.tga");
  g_blood[5].Load("data/blood_5.tga");
  g_blood[6].Load("data/blood_6.tga");

  g_floor.Load("data/floor_1.tga");
  g_floor.UpdateOpaqueSpans();
  g_floor_dark.Load("data/floor_1_dark.tga");
  g_floor_dark.UpdateOpaqueSpans();
  g_hero[0].Load("data/hero_1.tga");
  g_hero[0].UpdateOpaqueSpans();
  g_hero[1].Load("data/hero_2.tga");
  g_hero[1].UpdateOpaqueSpans();
  g_intro_airplane.Load("data/intro_airplane_1.tga");
  g_intro_airplane.UpdateOpaqueSpans();
  g_intro_pyramids.Load("data/intro_pyramids_1.tga");

  g_monster[0].Load("data/monster_0.tga");
  g_monster[1].Load("data/monster_1.tga");
  g_monster[2].Load("data/monster_2.tga");

  g_stairs_down_left.Load("data/stairs_down_left_1.tga");
  g_stairs_down_left_dark.Load("data/stairs_down_left_1_dark.tga");
  g_stairs_down_right.Load("data/stairs_down_right_1.tga");
  g_stairs_down_right_dark.Load("data/stairs_down_right_1_dark.tga");
  g_stairs_up_left.Load("data/stairs_up_left_1.tga");
  g_stairs_up_left_dark.Load("data/stairs_up_left_1_dark.tga");
  g_stairs_up_right.Load("data/stairs_up_right_1.tga");
  g_stairs_up_right_dark.Load("data/stairs_up_right_1_dark.tga");
  g_stick.Load("data/stick_1.tga");
  g_stick.SetPivot(Vec2Si32(12, 12));
  g_stone.Load("data/stone_1.tga");
  g_stone.SetPivot(Vec2Si32(12, 12));
  g_wall.Load("data/wall_1.tga");
  g_wall.UpdateOpaqueSpans();
  g_wall_dark.Load("data/wall_1_dark.tga");
  g_wall_dark.UpdateOpaqueSpans();

  PlayIntro();

  ResizeScreen(800, 500);

  InitCreatures();
  Hero().kind = static_cast<CreatureKind>(
      Random(kCreatureHeroBegin, kCreatureHeroEnd - 1));
  Hero().pos = Vec2Si32(1, 1);
  Hero().items.resize(kItemKindCount);
  for (Ui32 idx = 0; idx < Hero().items.size(); ++idx) {
    Hero().items[idx] = 0;
  }
  GenerateMaze();
}

void LookAround() {
  Vec2Si32 pos;
  for (pos.x = 0; pos.x < g_maze_size.x; ++pos.x) {
    for (pos.y = 0; pos.y < g_maze_size.y; ++pos.y) {
      Maze(pos).is_visible = false;
    }
  }

  const Si32 radius = 10;
  Vec2Si32 min(std::max(0, Hero().pos.x - radius),
    std::max(0, Hero().pos.y - radius));
  Vec2Si32 max(std::min(g_maze_size.x - 1, Hero().pos.x + radius),
    std::min(g_maze_size.y - 1, Hero().pos.y + radius));
  for (pos.x = min.x; pos.x <= max.x; ++pos.x) {
    for (pos.y = min.y; pos.y <= max.y; ++pos.y) {
      Vec2Si32 hero_to_pos = pos - Hero().pos;
      Si32 maxStep = radius * 2;
      for (Si32 step = 0; step <= maxStep; ++step) {
        Vec2Si32 s = Hero().pos + ((hero_to_pos * step) / maxStep);
        Maze(s).is_known = true;
        Maze(s).is_visible = true;
        if (Maze(s).kind == kCellWall) {
          break;
        }
      }
    }
  }
}

void Update() {
  double time = Time();

  if (!g_music.IsPlaying()) {
    //    g_music.Play();
  }

  Vec2Si32 step(0, 0);
  if (IsKeyDown(kKeyUp) || IsKeyDown("w")) {
    step.y = 1;
  }
  if (IsKeyDown(kKeyDown) || IsKeyDown("s")) {
    step.y = -1;
  }
  if (IsKeyDown(kKeyLeft) || IsKeyDown("a")) {
    step.x = -1;
    step.y = 0;
  }
  if (IsKeyDown(kKeyRight) || IsKeyDown("d")) {
    step.x = 1;
    step.y = 0;
  }

  static bool g_musicDisabled = false;

  if (!g_musicDisabled) {
    // switch background music tracks
    if (!g_music.IsPlaying()) {
      g_music.Play();
    }
  } else {
    if (g_music.IsPlaying()) {
      g_music.Stop();
    }
  }

  if (IsKeyDownward("5")) {
    g_musicDisabled = !g_musicDisabled;
  }
  if (IsKeyDownward("6")) {
    SetInverseY(true);
  }
  if (IsKeyDownward("7")) {
    SetInverseY(false);
  }

  // Cheats
  if (IsKeyDownward("v")) {
    Vec2Si32 pos;
    for (pos.x = 0; pos.x < g_maze_size.x; ++pos.x) {
      for (pos.y = 0; pos.y < g_maze_size.y; ++pos.y) {
        Maze(pos).is_known = true;
      }
    }
  }
  if (IsKeyDownward("n")) {
    GenerateMaze();
    easy::GetEngine()->GetBackbuffer().Clear();
  }
  if (IsKeyDownward("=+")) {
    SetMasterVolume(Clamp(GetMasterVolume() + 0.01f, 0.f, 1.f));
  }
  if (IsKeyDownward("-_")) {
    SetMasterVolume(Clamp(GetMasterVolume() - 0.01f, 0.f, 1.f));
  }
  // End of cheats

  const double kMoveDuration = 0.1;
  Hero().move_part = 0.0;
  if (Hero().is_moving) {
    double duration = time - Hero().move_start_at;
    if (duration > kMoveDuration) {
      Hero().pos = Hero().next_pos;
      Hero().is_moving = false;
    } else {
      Hero().move_part = duration / kMoveDuration;
    }
  }

  if (!Hero().is_moving) {
    Cell &cur = Maze(Hero().pos);
    if (!cur.items.empty()) {
      for (Ui32 idx = 0; idx < cur.items.size(); ++idx) {
        Hero().items[cur.items[idx].kind]++;
      }
      cur.items.clear();
    }
    if (!(step == Vec2Si32(0, 0))) {
      Cell &cell = Maze(Hero().pos + step);
      if (cell.kind != kCellWall) {
        if (cell.kind == kCellFloor) {
          Hero().is_moving = true;
          Hero().next_pos = Hero().pos + step;
          Hero().move_start_at = time;
        } else if (cell.kind == kCellStairsDownLeft
          || cell.kind == kCellStairsDownRight) {
          Hero().pos += step;
          g_is_first_level = false;
          g_upper_cell_kind = cell.kind;
          GenerateMaze();
          easy::GetEngine()->GetBackbuffer().Clear();
        }
      }
    }
  }

  LookAround();
}

void Render() {
  Clear();

  Vec2Si32 pos;
  for (pos.y = 19; pos.y >= 0; --pos.y) {
    for (pos.x = 0; pos.x < 32; ++pos.x) {
      if (Maze(pos).is_known) {
        Vec2Si32 scr_pos = pos * 25;
        bool is_visible = Maze(pos).is_visible;
        CellKind kind = Maze(pos).kind;
        Sprite &sprite = CellSprite(kind, is_visible);
        sprite.Draw(scr_pos);
      }
    }
  }
  for (pos.y = 19; pos.y >= 0; --pos.y) {
    for (pos.x = 0; pos.x < 32; ++pos.x) {
      Cell &cell = Maze(pos);
      if (cell.is_known) {
        Vec2Si32 scr_pos = pos * 25;
        if (IsHigh(cell.kind)) {
          Sprite& sprite = CellSprite(cell.kind, cell.is_visible);
          sprite.Draw(scr_pos);
        }
        for (size_t idx = 0; idx < cell.items.size(); idx++) {
          if (cell.is_visible) {
            Item &item = cell.items[idx];
            Sprite &sprite = ItemSprite(item.kind);
            sprite.Draw(scr_pos + sprite.Pivot(),
              static_cast<float>(Time()));
          }
        }
      }
    }
    if (pos.y == Hero().pos.y) {
      Vec2Si32 scr_pos = Hero().pos * 25 +
        static_cast<Si32>(Hero().move_part * 25.0) *
        (Hero().next_pos - Hero().pos);
      CreatureSprite(Hero().kind).Draw(scr_pos);
    }
  }

  Si32 item_x = 25;
  for (Si32 kind = kItemNone; kind < kItemKindCount; ++kind) {
    Sprite &sprite = ItemSprite(static_cast<ItemKind>(kind));
    for (Si32 idx = 0; idx < Hero().items[kind]; ++idx) {
      sprite.Draw(Vec2Si32(item_x, 0) + sprite.Pivot(),
        static_cast<float>(Time()) + static_cast<float>(idx));
      item_x += 5;
    }
    if (Hero().items[kind]) {
      item_x += 30;
    }
  }
  /*
     static std::deque<Vec2Si32> history;
     history.push_back(MousePos() * 2 - ScreenSize() / 2);
     if (history.size() >= 2) {
     Vec2Si32 a = history[history.size() - 2];
     Vec2Si32 b = history[history.size() - 1];
     Vec2Si32 ab = b - a;
     if (ab.x * ab.x + ab.y * ab.y < 1) {
     history.pop_back();
     }
     }
     if (history.size() > 1000) {
     history.pop_front();
     }
     for (size_t idx = 1; idx < history.size(); ++idx) {
     DrawTriangle(history[idx - 1], history[idx], ScreenSize() / 2,
     Rgba(0, 0, 255, 255), Rgba(255, 0, 0, 255), Rgba(0, 255, 0, 255));
     }*/

  static double prev_time = Time();
  static double smooth_fps = 0.0;
  double time = Time();
  double fps = 1.0 / std::max(time - prev_time, 0.001);
  smooth_fps = smooth_fps * 0.95 + 0.05 * fps;
  prev_time = time;
  char fps_text[128];
  snprintf(fps_text, sizeof(fps_text), u8"FPS: %.1F", smooth_fps);
  g_font.Draw(fps_text, 0, ScreenSize().y - 1, kTextOriginTop);
  
  const char *long_text = u8"Длинный текст на русском языке по центру!\n"
  u8"Second line !@#$%^&*()_+ with \\r at the end\r"
  u8"Third line ,./<>?;'\\:\"| with \\n at the end\n"
  u8"4th line []{}-=§±`~ with \\n\\r\\n\\r at the end\n\r\n\r"
  u8"6 йцукенгшщзхъфывапролджэёячсмитьбю\n"
  u8"7 ЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЁЯЧСМИТЬБЮ\r"
  u8"8th line ДDОOАA - – — with \\n\\r\\n\\r at the end\n\r\n\r";
  Si64 ltw = g_font.EvaluateSize(long_text, false).x;
  Si64 ltoffset = (ScreenSize().x - ltw) / 2;
  g_font.Draw(long_text, static_cast<Si32>(ltoffset),
      ScreenSize().y, kTextOriginTop);
  
  g_font.Draw("first base at 0\ninvisible line 2",
      0, 0, kTextOriginFirstBase);
  g_font.Draw("line 1\nlast base at 0",
      ScreenSize().x / 3, 0, kTextOriginLastBase);
  g_font.Draw("line 1\nbottom at 0",
      ScreenSize().x * 2 / 3, 0, kTextOriginBottom);
  static Si32 frame = 0;
  frame++;
  if (frame % 20 == 0)
  ShowFrame();
}

void EasyMain() {
  Init();
  while (!IsKeyDownward(kKeyEscape)) {
    Update();
    Render();
  }
}
