// Copyright (c) <year> Your name

#include "engine/easy.h"
#include <vector>
#include <utility>
#include <chrono>
#include <random>

using namespace arctic;  // NOLINT

Font g_font;

void Move_snake_tail( int previous_x, int previous_y, std::deque<std::pair<int, int>>& snake)
{
    for (auto snake_iter = ++snake.begin(); snake_iter < snake.end(); ++snake_iter)
    {
        std::swap(previous_x, (*snake_iter).first);
        std::swap(previous_y, (*snake_iter).second);
    }
}

void ShowFailureMessage() {
    g_font.Draw("Game over. Press esc to quit", ScreenSize().x / 2, ScreenSize().y / 2, kTextOriginBottom);
    ShowFrame();
    while (!IsKeyDownward(kKeyEscape)) {
        ShowFrame();
    }
}

void Draw_scene(const std::deque<std::pair<int, int>>& snake, const std::list<std::pair<int, int>>& food) {
    Clear();
    for (auto snake_part : snake){
        g_font.Draw(u8"o", snake_part.first, snake_part.second, kTextOriginBottom,
            kDrawBlendingModeColorize, kFilterNearest, Rgba(128, 255, 128));
    }
    for (auto food_part : food){
        g_font.Draw(u8"o", food_part.first, food_part.second, kTextOriginBottom,
            kDrawBlendingModeColorize, kFilterNearest, Rgba(255, 128, 128));
    }
    ShowFrame();
}

bool Snake_intersect(const std::deque<std::pair<int, int>>& snake) {
    int begin_x = (*snake.begin()).first;
    int begin_y = (*snake.begin()).second;
    if ( begin_x <= 0 || begin_y <= 0 || begin_x >= ScreenSize().x || begin_y >= ScreenSize().y ) {
        return true;
    }
    for (auto snake_iter = ++snake.begin(); snake_iter < --snake.end(); ++snake_iter){
        if (begin_x == (*snake_iter).first && begin_y == (*snake_iter).second) {
            return true;
        }
    }
    return false;
}

void Generate_food(std::list<std::pair<int, int>> &food) {
    int food_quantity = 100;
    static std::mt19937 rand;
    auto start = std::chrono::high_resolution_clock::now();
    rand.seed((uint32_t)(std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count()));
    static std::uniform_int_distribution<unsigned> random_x (1, ScreenSize().x - 1);
    static std::uniform_int_distribution<unsigned> random_y (1, ScreenSize().y - 1);
    
    for (int i = 0; i <= food_quantity; ++i )
    {
        int x = random_x(rand) / 10 * 10;
        int y = random_y(rand) / 10 * 10;
        food.push_back({x , y});
    }
}

bool Food_intersect( std::deque<std::pair<int, int>>& snake, std::list<std::pair<int, int>>& food) {
    for (auto food_part= food.begin(); food_part != food.end(); ++food_part) {
        if ((*food_part).first == (*snake.begin()).first && (*food_part).second == (*snake.begin()).second) {
            snake.push_front({(*food_part).first,(*food_part).second});
            food.erase(food_part);
            return true;
        }
    }
    return false;
}

void EasyMain() {
  g_font.Load("data/arctic_one_bmf.fnt");
    int xoffset = 10;
    int yoffset = 10;
    std::deque<std::pair<int, int>> snake = {{40, 10}, {30, 10}, {20, 10}, {10, 10}};
    int previous_x = (*snake.begin()).first;
    int previous_y = (*snake.begin()).second;
    double snake_time = Time();
    bool game_over = false;
    enum Direction { up, down, right, left};
    Direction direction = up;
    std::list<std::pair<int, int>> food = {{0,0}};
    Generate_food(food);
    
    Draw_scene(snake, food);
    
    while (!IsKeyDownward(kKeyEscape) && !game_over) {
        ShowFrame();
        if (IsKeyDownward(kKeyUp) || direction == up && Time() > snake_time + 1) {
            previous_x = (*snake.begin()).first;
            previous_y = (*snake.begin()).second;
            direction = up;
            (*snake.begin()).second += yoffset;
            if(!Snake_intersect(snake)) {
                Move_snake_tail(previous_x, previous_y, snake);
                Food_intersect(snake, food);
                Draw_scene(snake, food);
                snake_time = Time();
            }
            else {
                ShowFailureMessage();
                game_over = true;
            }
        }
        if (IsKeyDownward(kKeyDown) || direction == down && Time() > snake_time + 1) {
            previous_x = (*snake.begin()).first;
            previous_y = (*snake.begin()).second;
            direction = down;
            (*snake.begin()).second -= yoffset;
            if(!Snake_intersect(snake)) {
                Move_snake_tail(previous_x, previous_y, snake);
                Food_intersect(snake, food);
                Draw_scene(snake, food);
                snake_time = Time();
            }
            else {
                ShowFailureMessage();
                game_over = true;
            }
        }
        
        if (IsKeyDownward(kKeyRight) || direction == right && Time() > snake_time + 1) {
            previous_x = (*snake.begin()).first;
            previous_y = (*snake.begin()).second;
            direction = right;
            (*snake.begin()).first += xoffset;
            if (!Snake_intersect(snake)) {
                Move_snake_tail(previous_x, previous_y, snake);
                Food_intersect(snake, food);
                Draw_scene(snake, food);
                snake_time = Time();
            }
            else {
                ShowFailureMessage();
                game_over = true;
            }
                
        }
        
        if (IsKeyDownward(kKeyLeft) || direction == left && Time() > snake_time + 1) {
            previous_x = (*snake.begin()).first;
            previous_y = (*snake.begin()).second;
            direction = left;
            (*snake.begin()).first -= xoffset;
            if (!Snake_intersect(snake)) {
                Move_snake_tail(previous_x, previous_y, snake);
                Food_intersect(snake, food);
                Draw_scene(snake, food);
                snake_time = Time();
            }
            else {
                ShowFailureMessage();
                game_over = true;
            }
        }
    }
}
