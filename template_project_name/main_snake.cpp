// Copyright (c) <year> Your name

#include "engine/easy.h"
#include <deque>
#include <list>
#include <utility>
#include <vector>

using namespace arctic;  // NOLINT

Font g_font;

typedef std::pair<int, int> Cell;
typedef std::deque<Cell> Snake;
typedef std::list<Cell> Food;

enum Direction {
  kUp,
  kDown,
  kRight,
  kLeft
};

/// @brief Moves the snake tail
/// @param previous_x Previous x coordinate of the head
/// @param previous_y Previous y coordinate of the head
/// @param snake Snake body coordinates in a deque
void MoveSnakeTail(int previous_x, int previous_y, Snake *snake) {
  for (auto snake_iter = ++snake->begin(); snake_iter < snake->end();
      ++snake_iter) {
    std::swap(previous_x, snake_iter->first);
    std::swap(previous_y, snake_iter->second);
  }
}

/// @brief Shows the failure message
void ShowFailureMessage() {
  g_font.Draw("Game over. Press esc to quit",
      ScreenSize().x / 2, ScreenSize().y / 2, kTextOriginBottom);
  ShowFrame();
  // Closing the window ends the program by itself, Escape is this program's
  // own convention.
  while (!IsKeyDownward(kKeyEscape)) {
    ShowFrame();
  }
}

/// @brief Draws the scene
/// @param snake Snake body coordinates in a deque
/// @param food Food coordinates in a list
void DrawScene(const Snake &snake, const Food &food) {
  Clear();
  for (const Cell &snake_part : snake) {
    g_font.Draw(u8"o", snake_part.first, snake_part.second, kTextOriginBottom,
        kTextAlignmentLeft, kDrawBlendingModeColorize, kFilterNearest,
        Rgba(128, 255, 128));
  }
  for (const Cell &food_part : food) {
    g_font.Draw(u8"o", food_part.first, food_part.second, kTextOriginBottom,
        kTextAlignmentLeft, kDrawBlendingModeColorize, kFilterNearest,
        Rgba(255, 128, 128));
  }
  ShowFrame();
}

/// @brief Checks if the snake hits the screen border or itself
/// @param snake Snake body coordinates in a deque
/// @return True if the snake head is outside the screen or on its body
bool SnakeIntersect(const Snake &snake) {
  int begin_x = snake.begin()->first;
  int begin_y = snake.begin()->second;
  if (begin_x <= 0 || begin_y <= 0
      || begin_x >= ScreenSize().x || begin_y >= ScreenSize().y) {
    return true;
  }
  for (auto snake_iter = ++snake.begin(); snake_iter < --snake.end();
      ++snake_iter) {
    if (begin_x == snake_iter->first && begin_y == snake_iter->second) {
      return true;
    }
  }
  return false;
}

/// @brief Generates food
/// @param food A pointer to the list of food coordinates
void GenerateFood(Food *food) {
  const int food_quantity = 100;
  for (int i = 0; i < food_quantity; ++i) {
    // Food snaps to the 10 pixel grid the snake moves on.
    int x = Random32(1, ScreenSize().x - 1) / 10 * 10;
    int y = Random32(1, ScreenSize().y - 1) / 10 * 10;
    food->push_back(Cell(x, y));
  }
}

/// @brief Checks if the snake head is on food and eats it
/// @param snake Snake body coordinates in a deque
/// @param food Food coordinates in a list
/// @return True if food was eaten, false otherwise
bool FoodIntersect(Snake *snake, Food *food) {
  for (auto food_part = food->begin(); food_part != food->end(); ++food_part) {
    if (food_part->first == snake->begin()->first
        && food_part->second == snake->begin()->second) {
      snake->push_front(Cell(food_part->first, food_part->second));
      food->erase(food_part);
      return true;
    }
  }
  return false;
}

/// @brief Reads the arrow keys
/// @param direction The current direction, changed by a pressed arrow key
/// @return True if an arrow key was pressed this frame
bool ReadDirection(Direction *direction) {
  // A turn straight back is not allowed, the head would hit the neck.
  if (IsKeyDownward(kKeyUp) && *direction != kDown) {
    *direction = kUp;
    return true;
  }
  if (IsKeyDownward(kKeyDown) && *direction != kUp) {
    *direction = kDown;
    return true;
  }
  if (IsKeyDownward(kKeyRight) && *direction != kLeft) {
    *direction = kRight;
    return true;
  }
  if (IsKeyDownward(kKeyLeft) && *direction != kRight) {
    *direction = kLeft;
    return true;
  }
  return false;
}

/// @brief Main function for the snake game
void EasyMain() {
  g_font.Load("data/arctic_one_bmf.fnt");  // Load the font from the file
  const int step = 10;
  const double step_seconds = 1.0;
  Snake snake = {{40, 10}, {30, 10}, {20, 10}, {10, 10}};
  double snake_time = Time();
  Direction direction = kUp;
  Food food;
  GenerateFood(&food);

  DrawScene(snake, food);

  // Closing the window ends the program by itself, Escape is this program's
  // own convention.
  while (!IsKeyDownward(kKeyEscape)) {
    ShowFrame();

    // The snake moves when an arrow key is pressed or when a second has passed
    // since the previous move.
    bool is_key_move = ReadDirection(&direction);
    bool is_timer_move = Time() > snake_time + step_seconds;
    if (!is_key_move && !is_timer_move) {
      continue;
    }

    int xmul = 0;
    int ymul = 0;
    switch (direction) {
      case kUp:
        ymul = 1;
        break;
      case kDown:
        ymul = -1;
        break;
      case kRight:
        xmul = 1;
        break;
      case kLeft:
        xmul = -1;
        break;
    }

    int previous_x = snake.begin()->first;
    int previous_y = snake.begin()->second;
    snake.begin()->first += step * xmul;
    snake.begin()->second += step * ymul;

    if (SnakeIntersect(snake)) {
      ShowFailureMessage();
      return;
    }
    MoveSnakeTail(previous_x, previous_y, &snake);
    FoodIntersect(&snake, &food);
    DrawScene(snake, food);
    snake_time = Time();
  }
}
