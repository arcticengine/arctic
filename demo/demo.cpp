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

// demo.cpp : Defines the entry point for the application.
#include "demo/demo.h"

#include <time.h>
#include <algorithm>
#include <deque>
#include <random>
#include <utility>
#include "engine/easy.h"

using namespace arctic;
using namespace arctic::easy;

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

Sound g_step;

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

std::independent_bits_engine<std::mt19937_64, 8, Ui64> g_rnd;

void InitCreatures() {
    g_creatures.clear();
    Creature hero;
    g_hero_idx = g_creatures.size();
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
        Si32 idx = g_rnd() % variants;
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
    Si32 rnd_dir = g_rnd() % 4;
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

void PlayIntro() {
    ResizeScreen(320, 200);
    Ui8 snow[2][320 * 200];
    for (Si32 i = 0; i < 320 * 200; ++i) {
        if (g_rnd() % 16 == 0) {
            snow[0][i] = g_rnd() % 256;
        } else {
            snow[0][i] = 0;
        }
    }
    Ui8 *cur_snow = snow[0];
    Ui8 *next_snow = snow[1];

    Vec2Si32 pyramids_pos(0, 10);

    Vec2Si32 airplane_pos_begin(ScreenSize().x,
        ScreenSize().y - g_intro_airplane.height() / 2);
    Vec2Si32 airplane_pos_end(-g_intro_airplane.width(), 0);
    Si32 duration1 = 380;
    Si32 duration2 = 500;
    Si32 duration3 = 560;
    Si32 frame = 0;
    while (true) {
        Clear();
        if (IsKey(kKeyEscape) || IsKey(kKeySpace) || IsKey(kKeyEnter)) {
            return;
        }
        frame++;
        if (frame > duration3) {
            break;
        }
        if (frame > duration1 && frame < duration2) {
            pyramids_pos = Vec2Si32(g_rnd() % 3 - 1, 10 + g_rnd() % 3 - 1);
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
                    Si32 next_x = x + 8 - z/42;
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
                    if (g_rnd() % 16 == 0) {
                        Si32 z2 = g_rnd() % 256;
                        if (z2 > z) {
                            next_snow[x + y * 320] = z2;
                        }
                    }
                    back_buffer[x + y * 320] =
                        Rgba(255 - z/8, 255 - z/8, 255 - z/8);
                }
            }
        }
        std::swap(cur_snow, next_snow);
        ShowFrame();
    }
}

void GenerateMaze() {
    g_rnd.seed(static_cast<unsigned int>(time(nullptr)));

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
        Si32 rnd = g_rnd() % dead_ends.size();
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

    Si32 to_eliminate = dead_ends.size() / 2;
    for (Si32 idx = 0; idx < to_eliminate; ++idx) {
        Si32 rnd = g_rnd() % dead_ends.size();
        EliminateDeadEnd(dead_ends[rnd]);
        dead_ends[rnd] = dead_ends.back();
        dead_ends.pop_back();
    }
    Si32 to_cycle = dead_ends.size() / 2;
    for (Si32 idx = 0; idx < to_cycle; ++idx) {
        Si32 rnd = g_rnd() % dead_ends.size();
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
        Ui64 rnd = g_rnd();
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
    g_blood[0].Load("data/blood_0.tga");
    g_blood[1].Load("data/blood_1.tga");
    g_blood[2].Load("data/blood_2.tga");
    g_blood[3].Load("data/blood_3.tga");
    g_blood[4].Load("data/blood_4.tga");
    g_blood[5].Load("data/blood_5.tga");
    g_blood[6].Load("data/blood_6.tga");

    g_floor.Load("data/floor_1.tga");
    g_floor_dark.Load("data/floor_1_dark.tga");
    g_hero[0].Load("data/hero_1.tga");
    g_hero[1].Load("data/hero_2.tga");
    g_intro_airplane.Load("data/intro_airplane_1.tga");
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
    g_stone.Load("data/stone_1.tga");
    g_wall.Load("data/wall_1.tga");
    g_wall_dark.Load("data/wall_1_dark.tga");

    g_step.Load("data/step.wav");

    PlayIntro();

    ResizeScreen(800, 500);

    InitCreatures();
    Hero().kind = static_cast<CreatureKind>(kCreatureHeroBegin +
        g_rnd() % (kCreatureHeroEnd - kCreatureHeroBegin));
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
    Vec2Si32 step(0, 0);
    if (IsKey(kKeyUp) || IsKey("w")) {
        step.y = 1;
    }
    if (IsKey(kKeyDown) || IsKey("s")) {
        step.y = -1;
    }
    if (IsKey(kKeyLeft) || IsKey("a")) {
        step.x = -1;
        step.y = 0;
    }
    if (IsKey(kKeyRight) || IsKey("d")) {
        step.x = 1;
        step.y = 0;
    }

    // Cheats
    if (IsKey("v")) {
        Vec2Si32 pos;
        for (pos.x = 0; pos.x < g_maze_size.x; ++pos.x) {
            for (pos.y = 0; pos.y < g_maze_size.y; ++pos.y) {
                Maze(pos).is_known = true;
            }
        }
    }
    if (IsKey("n")) {
        GenerateMaze();
        easy::GetEngine()->GetBackbuffer().Clear();
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
                    g_step.Play();

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
                        sprite.Draw(scr_pos);
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
            sprite.Draw(item_x, 0);
            item_x += 5;
        }
        if (Hero().items[kind]) {
            item_x += 30;
        }
    }

    ShowFrame();
}

void EasyMain() {
    Init();
    while (!IsKey(kKeyEscape)) {
        Update();
        Render();
    }
}