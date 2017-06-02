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
Sprite g_stpne;
Sprite g_wall;
Sprite g_wall_dark;

Sound g_step;

enum CellKind {
    kCellWall = 0,
    kCellFloor,
    kCellStairsDownLeft,
    kCellStairsDownRight,
    kCellStairsUpLeft,
    kCellStairsUpRight,
};

struct Cell {
    CellKind kind = kCellWall;
    bool is_in_fog_of_war = true;
    bool is_visible = false;
};

Vec2Si32 g_maze_size(32, 20);
Cell g_maze[32][20];
Vec2Si32 g_hero_pos(1, 1);
Si32 g_hero_idx = 0;
std::independent_bits_engine<std::mt19937_64, 8, Ui64> g_rnd;

void StepMazeGeneration(Vec2Si32 from, Vec2Si32 size) {
    g_maze[from.x][from.y].kind = kCellFloor;
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
            if (g_maze[to.x][to.y].kind == kCellWall) {
                g_maze[path.x][path.y].kind = kCellFloor;
                StepMazeGeneration(to, size);
            }
        }
        direction[idx] = direction[variants - 1];
    }
}

bool IsDeadEnd(Si32 x, Si32 y) {
    Vec2Si32 dir[4] = {Vec2Si32(-1, 0)
        , Vec2Si32(1, 0)
        , Vec2Si32(0, -1)
        , Vec2Si32(0, 1)};
    if (g_maze[x][y].kind == kCellFloor) {
        Si32 exits = 0;
        for (Si32 idx = 0; idx < 4; ++idx) {
            if (g_maze[x + dir[idx].x][y + dir[idx].y].kind == kCellFloor) {
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
        if (pos == g_hero_pos) {
            return;
        }
        if (g_maze[pos.x][pos.y].kind != kCellFloor) {
            return;
        }
        Si32 exits = 0;
        Vec2Si32 exit;
        for (Si32 idx = 0; idx < 4; ++idx) {
            if (g_maze[pos.x + dir[idx].x][pos.y + dir[idx].y].kind ==
                    kCellFloor) {
                exits++;
                exit = Vec2Si32(pos.x + dir[idx].x, pos.y + dir[idx].y);
            }
        }
        if (exits != 1) {
            return;
        }
        g_maze[pos.x][pos.y].kind = kCellWall;
        pos = exit;
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
        for (Si32 y = 0; y < 200; ++y) {
            for (Si32 x = 0; x < 320; ++x) {
                Si32 z = cur_snow[x + y * 320];
                if (z) {
                    Si32 next_x = x + 8 - z/42;
                    Si32 next_y = y - 1;
                    if (next_x >= 320) {
                        next_x -= 320;
                    }
                    if (next_y < 0) {
                        next_y = 199;
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

    for (Si32 x = 0; x < g_maze_size.x; ++x) {
        for (Si32 y = 0; y < g_maze_size.y; ++y) {
            g_maze[x][y].kind = kCellWall;
            g_maze[x][y].is_visible = false;
            g_maze[x][y].is_in_fog_of_war = true;
        }
    }

    StepMazeGeneration(g_hero_pos, g_maze_size);
    std::deque<Vec2Si32> dead_ends = FindDeadEnds(g_maze_size);

    int attempt = 0;
    while (true) {
        attempt++;
        Si32 rnd = g_rnd() % dead_ends.size();
        Vec2Si32 pos = dead_ends[rnd];
        bool is_ok = false;
        if (g_maze[pos.x - 1][pos.y].kind == kCellFloor) {
            is_ok = true;
            g_maze[pos.x][pos.y].kind = kCellStairsDownLeft;
        } else if (g_maze[pos.x + 1][pos.y].kind == kCellFloor
                || attempt > 10) {
            is_ok = true;
            g_maze[pos.x][pos.y].kind = kCellStairsDownRight;
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
    g_stpne.Load("data/stone_1.tga");
    g_wall.Load("data/wall_1.tga");
    g_wall_dark.Load("data/wall_1_dark.tga");

    g_step.Load("data/step.wav");

    PlayIntro();

    ResizeScreen(800, 500);

    g_hero_idx = g_rnd() % 2;
    g_hero_pos = Vec2Si32(1, 1);
    GenerateMaze();
}

void Update() {
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
        for (Si32 x = 0; x < g_maze_size.x; ++x) {
            for (Si32 y = 0; y < g_maze_size.y; ++y) {
                g_maze[x][y].is_in_fog_of_war = false;
            }
        }
    }
    if (IsKey("n")) {
        GenerateMaze();
        easy::GetEngine()->GetBackbuffer().Clear();
    }
    // End of cheats

    Cell &cell = g_maze[g_hero_pos.x + step.x][g_hero_pos.y + step.y];
    if (cell.kind != kCellWall) {
        if (cell.kind == kCellFloor) {
            g_step.Play();
            g_hero_pos += step;
        } else if (cell.kind == kCellStairsDownLeft 
                || cell.kind == kCellStairsDownRight) {
            g_hero_pos += step;
            GenerateMaze();
            easy::GetEngine()->GetBackbuffer().Clear();
        }
    }

    Vec2Si32 pos;
    for (pos.x = g_hero_pos.x - 1; pos.x <= g_hero_pos.x + 1; ++pos.x) {
        for (pos.y = g_hero_pos.y - 1; pos.y <= g_hero_pos.y + 1; ++pos.y) {
            g_maze[pos.x][pos.y].is_in_fog_of_war = false;
        }
    }
}

void Render() {
    for (Si32 x = 0; x < 32; ++x) {
        for (Si32 y = 19; y >= 0; --y) {
            if (!g_maze[x][y].is_in_fog_of_war) {
                Vec2Si32 pos(x * 25, y * 25);
                switch(g_maze[x][y].kind) {
                case kCellWall:
                    g_wall.Draw(pos);
                    break;
                case kCellFloor:
                    g_floor.Draw(pos);
                    break;
                case kCellStairsDownLeft:
                    g_stairs_down_left.Draw(pos);
                    break;
                case kCellStairsDownRight:
                    g_stairs_down_right.Draw(pos);
                    break;
                case kCellStairsUpLeft:
                    g_stairs_up_left.Draw(pos);
                    break;
                case kCellStairsUpRight:
                    g_stairs_up_right.Draw(pos);
                    break;
                }
            }
            if (x == g_hero_pos.x && y == g_hero_pos.y) {
                g_hero[g_hero_idx].Draw(x * 25, y * 25);
            }
        }
    }
}

void EasyMain() {
    Init();
    while (!IsKey(kKeyEscape)) {
        Update();
        Render();
        ShowFrame();
    }
}
