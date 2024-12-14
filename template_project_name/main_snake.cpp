// Copyright (c) <year> Your name

#include "engine/easy.h"
#include <vector>
#include <utility>
#include <chrono>
#include <random>

using namespace arctic;  // NOLINT

Font g_font;

/// @brief Moves the snake tail
/// @param previous_x Previous x coordinate
/// @param previous_y Previous y coordinate
/// @param snake Snake body coordinates in a deque
void Move_snake_tail(int previous_x, int previous_y, std::deque<std::pair<int, int>>& snake) {
    for (auto snake_iter = ++snake.begin(); snake_iter < snake.end(); ++snake_iter) {
        std::swap(previous_x, (*snake_iter).first);
        std::swap(previous_y, (*snake_iter).second);
    }
}

/// @brief Shows the failure message
void ShowFailureMessage() {
    g_font.Draw("Game over. Press esc to quit", ScreenSize().x / 2, ScreenSize().y / 2, kTextOriginBottom);
    ShowFrame();
    while (!IsKeyDownward(kKeyEscape)) {
        ShowFrame();
    }
}

/// @brief Draws the scene
/// @param snake Snake body coordinates in a deque
/// @param food Food coordinates in a list
void Draw_scene(const std::deque<std::pair<int, int>>& snake, const std::list<std::pair<int, int>>& food) {
    Clear();
    for (auto snake_part : snake){
        g_font.Draw(u8"o", snake_part.first, snake_part.second, kTextOriginBottom, kTextAlignmentLeft,
            kDrawBlendingModeColorize, kFilterNearest, Rgba(128, 255, 128));
    }
    for (auto food_part : food){
        g_font.Draw(u8"o", food_part.first, food_part.second, kTextOriginBottom, kTextAlignmentLeft,
            kDrawBlendingModeColorize, kFilterNearest, Rgba(255, 128, 128));
    }
    ShowFrame();
}

/// @brief Checks if the snake intersects with itself
/// @param snake Snake body coordinates in a deque
/// @return True if the snake intersects with itself, false otherwise
bool Snake_intersect(const std::deque<std::pair<int, int>>& snake) {
    int begin_x = (*snake.begin()).first;
    int begin_y = (*snake.begin()).second;
    if (begin_x <= 0 || begin_y <= 0 || begin_x >= ScreenSize().x || begin_y >= ScreenSize().y) {
        return true;
    }
    for (auto snake_iter = ++snake.begin(); snake_iter < --snake.end(); ++snake_iter) {
        if (begin_x == (*snake_iter).first && begin_y == (*snake_iter).second) {
            return true;
        }
    }
    return false;
}

/// @brief Generates food
/// @param food A reference to the list of food coordinates
void Generate_food(std::list<std::pair<int, int>> &food) {
    int food_quantity = 100;
    static std::mt19937 rand;
    auto start = std::chrono::high_resolution_clock::now();
    rand.seed((uint32_t)(std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count()));
    static std::uniform_int_distribution<unsigned> random_x (1, ScreenSize().x - 1);
    static std::uniform_int_distribution<unsigned> random_y (1, ScreenSize().y - 1);
    
    for (int i = 0; i <= food_quantity; ++i ) {
        int x = random_x(rand) / 10 * 10;
        int y = random_y(rand) / 10 * 10;
        food.push_back({x , y});
    }
}

/// @brief Checks if the snake intersects with food
/// @param snake Snake body coordinates in a deque
/// @param food Food coordinates in a list
/// @return True if the snake intersects with food, false otherwise
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

/// @brief Main function for the snake game
void EasyMain() {
    g_font.Load("data/arctic_one_bmf.fnt"); // Load the font from the file
    int xoffset = 10;
    int yoffset = 10;
    std::deque<std::pair<int, int>> snake = {{40, 10}, {30, 10}, {20, 10}, {10, 10}};
    double snake_time = Time();
    enum Direction { up, down, right, left};
    Direction direction = up;
    std::list<std::pair<int, int>> food = {{0,0}};
    Generate_food(food);
    
    Draw_scene(snake, food);
    
    while (!IsKeyDownward(kKeyEscape)) {
        ShowFrame();

        int xmul = 0;
        int ymul = 0;
        int previous_x = (*snake.begin()).first;
        int previous_y = (*snake.begin()).second;

        if (IsKeyDownward(kKeyUp) || direction == up && Time() > snake_time + 1) {
            ymul = 1;
            direction = up;
        }
        if (IsKeyDownward(kKeyDown) || direction == down && Time() > snake_time + 1) {
            ymul = -1;
            direction = down;
        }
        if (IsKeyDownward(kKeyRight) || direction == right && Time() > snake_time + 1) {
            xmul = 1;
            direction = right;      
        }
        if (IsKeyDownward(kKeyLeft) || direction == left && Time() > snake_time + 1) {
            xmul = -1;
            direction = left;
        }

        if (xmul != 0 || ymul != 0) {
            (*snake.begin()).first += xoffset * xmul;
            (*snake.begin()).second += yoffset * ymul;
            
            if (!Snake_intersect(snake)) {
                Move_snake_tail(previous_x, previous_y, snake);
                Food_intersect(snake, food);
                Draw_scene(snake, food);
                snake_time = Time();
            } else {
                ShowFailureMessage();
                return;
            }
        }
    }
}
