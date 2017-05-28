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
#include "engine/easy.h"

using namespace arctic;
using namespace arctic::easy;

Sprite g_wall;
Sprite g_hero;
Sprite g_floor;
Sound g_step;

Si32 g_maze[32][20];
Vec2Si32 g_hero_pos(1, 1);

void StepMazeGeneration(Vec2Si32 from, Vec2Si32 size) {
    g_maze[from.x][from.y] = 1;
    Vec2Si32 direction[4] = {Vec2Si32(-1, 0)
        , Vec2Si32(1, 0)
        , Vec2Si32(0, -1)
        , Vec2Si32(0, 1)};
    for (Si32 variants = 4; variants > 0; --variants) {
        Si32 idx = rand() % variants;
        Vec2Si32 dir = direction[idx];
        Vec2Si32 path = from + dir;
        Vec2Si32 to = path + dir;
        if (to.x > 0 && to.x < size.x - 1 && to.y > 0 && to.y < size.y - 1) {
            if (g_maze[to.x][to.y] == 0) {
                g_maze[path.x][path.y] = 1;
                StepMazeGeneration(to, size);
            }
        }
        direction[idx] = direction[variants - 1];
    }
}

void Init() {
    ResizeScreen(800, 500);

    g_wall.Load("data/wall1.tga");
    g_hero.Load("data/hero1.tga");
    g_floor.Load("data/floor1.tga");
    g_step.Load("data/step.wav");

    Vec2Si32 maze_size(32, 20);
    for (Si32 x = 0; x < maze_size.x; ++x) {
        for (Si32 y = 0; y < maze_size.y; ++y) {
            g_maze[x][y] = 0;
        }
    }

    g_hero_pos = Vec2Si32(1, 1);
    srand(static_cast<unsigned int>(time(nullptr)));
    StepMazeGeneration(g_hero_pos, maze_size);
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

    if (g_maze[g_hero_pos.x + step.x][g_hero_pos.y + step.y] == 1) {
        g_step.Play();
        g_hero_pos += step;
    }
}

void Render() {
    for (Si32 x = 0; x < 32; ++x) {
        for (Si32 y = 19; y >= 0; --y) {
            if (g_maze[x][y] == 0) {
                g_wall.Draw(x * 25, y * 25);
            } else {
                g_floor.Draw(x * 25, y * 25);
            }
            if (x == g_hero_pos.x && y == g_hero_pos.y) {
                g_hero.Draw(x * 25 + 5, y * 25 + 10);
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
