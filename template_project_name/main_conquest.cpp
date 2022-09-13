#include "engine/easy.h"

using namespace arctic;  // NOLINT

//////////////////////////////////////////////////////////////////////////////////////
// ТИПЫ И СТРУКТУРЫ ДАННЫХ

//  Тип юнита
enum UnitType {
  NONE = 0, // Юнит не используется - пустой элемент массива `units`
  CITY = 1, // Город - создает воинов
  SWORDSMAN = 2, // Воин
};

// Тип для описания игровок
enum Owner {
  NEUTRAL = 0, // Игрок не задан или нейтральный юнит
  PLAYER1 = 1,
  PLAYER2 = 2,
};

// Структура описывающая юнита
struct Unit {
  UnitType type; // Тип юнита
  Owner owner; // Владелец юнита

  // Положение юнита на игровом поле
  int x;
  int y;

  // Количество оставшихся очков перемещений (MP) в этом ходу.
  // В начале хода игрока все его юниты возобновляют MP и могут походить расходуя их.
  // Город не может ходить, но может построить юнита в соседней клетке.
  int moves;

  // Уничтожить юнита (удалить из игры)
  void destroy() {
    type = NONE;
    owner = NEUTRAL;
  }
};

//////////////////////////////////////////////////////////////////////////////////////
// ГЛОБАЛЬНЫЕ КОНСТАНТЫ

constexpr int map_x = 23, map_y = 11; // Размеры игрового поля в тайлах
constexpr int max_units = map_x * map_y; // Максимальное число юнитов в игре равно числу тайлов

// Число очков перемещений (MP) юнитов в зависимости от типа
const int unit_moves[] = {
  0, // NONE
  1, // CITY
  3, // SWORDSMAN
};

// Клавиши управления
const KeyCode keyQuitGame = kKeyEscape; // Выход из игры
const KeyCode keyNextTurn = kKeyEnter; // Передача хода

//////////////////////////////////////////////////////////////////////////////////////
// ГРАФИКА

constexpr int tile_x = 10, tile_y = 10; // Размер одного игрового тайла (клеточки)
constexpr int menu_y = 5; // Толщина полоски меню

// Цвета
const Rgba color_move(255, 255, 255);
const Rgba color_owner[3] = {
  Rgba(),          // NONE
  Rgba(255, 0, 0), // PLAYER1
  Rgba(0, 0, 255), // PLAYER2
};

Sprite sprites[3][3]; // Спрайты юнитов в зависимости от владельца (первый индекс) и типа (второй индекс)
Sprite sprite_grass; // Тайл с травой
Sprite sprite_frame; // Рамка вокруг выбранного юнита

Font g_font;

//////////////////////////////////////////////////////////////////////////////////////
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ

Unit units[max_units]; // Общий массив всех юнитов в игре
Owner turn; // Игрок, который в данный момент ходит
Unit* selected; // Текущий выбранный юнит
int orderx, ordery; // Координаты приказа текущему юниту
bool turn_is_over; // Ход окончен
bool game_is_over; // Игровая сессия завершена
Owner winner; // Кто победил

//////////////////////////////////////////////////////////////////////////////////////
// ФУНКЦИИ ДЛЯ РАБОТЫ С ЮНИТАМИ

// Удалить всех юнитов
void ClearUnits() {
  for (int i = 0; i < max_units; i++)
    units[i].destroy();
}

// Сбросить очки перемещений юнитам с владельцем `owner`
void AddUnitMoves(Owner owner) {
  for (int i = 0; i < max_units; i++) {
    if (units[i].owner == owner) {
      UnitType type = units[i].type; // Получаем тип юнита
      int moves = unit_moves[type]; // Узнаем сколько MP положено юнитам этого типа
      units[i].moves = moves; // Обновляем число MP этому юниту
    }
  }
}

// Создать новый юнит
Unit* CreateUnit(UnitType type, Owner owner, int x, int y, int moves = 0) {
  for (Unit* unit = units; unit != units + max_units; unit++) {
    if (unit->type == NONE) {
      // Мы нашли неиспользуемый юнит и заполняем его нужными нам данными
      unit->type = type;
      unit->owner = owner;
      unit->x = x;
      unit->y = y;
      unit->moves = moves;
      return unit; // Возвращаем указатель на только что созданный новый юнит
    }
  }
  Fatal("Out of units"); // ОШИБКА: Все юниты уже заняты
  return nullptr;
}

// Найти юнит, который находится в заданных координатах или вернуть `nullptr` если там пусто
Unit* FindUnit(int x, int y) {
  for (Unit* unit = units; unit != units + max_units; unit++)
    if (unit->type != NONE && unit->x == x && unit->y == y)
      return unit;
  return nullptr; // Юнит не найден
}

//////////////////////////////////////////////////////////////////////////////////////
// ФУНКЦИИ УПРАВЛЕНИЯ И РИСОВАНИЯ

// Нарисовать карту и юнитов
void DrawGame() {
  int moves_left = 0;

  // Рисуем карту
  for (int y = 0; y < map_y; y++)
    for (int x = 0; x < map_x; x++)
      sprite_grass.Draw(x * tile_x, y * tile_y);

  // Рисуем юнитов
  for (Unit* unit = units; unit != units + max_units; unit++) {
    if (unit->type == NONE) continue; // Пропускаем неиспользуемых юнитов

    if (unit->owner == turn)
      moves_left += unit->moves; // Подсчитываем оставшиеся очки передвижения

    // Рисуем спрайт юнита
    sprites[unit->owner][unit->type].Draw(unit->x * tile_x, unit->y * tile_y);

    // Рисуем число оставшихся ходов юнита
    for (int i = 0; i < unit->moves; i++)
       SetPixel(unit->x * tile_x + 2 + 2 * i, unit->y * tile_y + 2, color_move);

    // Рисуем рамку вокруг выбранного юнита
    if (selected == unit)
      sprite_frame.Draw(unit->x * tile_x, unit->y * tile_y);
  }

  // Рисуем панельку сверху
  Rgba color = color_owner[turn];
  if (moves_left == 0 || winner != NEUTRAL)
    color = Scale(color, 200 + 55 * sin(5 * Time())); // Мигаем кнопкой завершения хода
  DrawRectangle(Vec2Si32(0, tile_y * map_y), Vec2Si32(tile_x * map_x - 1, tile_y * map_y + menu_y - 1), color);

  ShowFrame();
  Sleep(0.03); // Ограничитель FPS
}

// Возвращает `true` если ход нужно завершить
bool StopTurn() {
  if (IsKeyUpward(keyQuitGame)) {
    turn_is_over = true; // Ход нужно завершить, прежде чем завершить игру
    game_is_over = true; // Выйти из игры после завершения хода
  }
  if (IsKeyUpward(keyNextTurn) || (IsKeyUpward(kKeyMouseLeft) && MousePos().y > tile_y * map_y))
    turn_is_over = true;
  return turn_is_over;
}

// Выбрать юнита если он не выбран - заполняет `selected` или завершает ход/игру
void SelectUnit() {
  while (selected == nullptr) {
    DrawGame();
    if (StopTurn()) return;
    if (IsKeyUpward(kKeyMouseLeft)) {
      int x = MousePos().x / tile_x;
      int y = MousePos().y / tile_y;
      Unit* unit = FindUnit(x, y);
      if (unit != nullptr) // Если клетка не пуста
        if (unit->owner == turn) // Если юнит принадлежит текущему игроку (нельзя выбрать врага)
          if (unit->moves > 0) // Если у данного юнита остались MP в этом ходу
            selected = unit; // Выбор удался
    }
  }
}

// Выбрать юнита - заполняет `orderx` и `ordery` или завершает ход/игру
void SelectOrder() {
  orderx = -1;
  ordery = -1;
  while (orderx == -1) {
    DrawGame();
    if (StopTurn()) return;
    if (IsKeyUpward(kKeyMouseLeft)) {
      int x = MousePos().x / tile_x;
      int y = MousePos().y / tile_y;
      int dx = x - selected->x; // Вычисляем смещение по x
      int dy = y - selected->y; // Вычисляум смещение по y
      if (x >= 0 && x < map_x && y >= 0 && y < map_y) // Убеждаемся что клик попадает на игровое поле
        if (dx <= 1 && dx >= -1) // За одно действие можно сдвинуться только на одну клетку по x и y
          if (dy <= 1 && dy >= -1) {
            // Приказ отдан
            orderx = x;
            ordery = y;
          }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// ИГРОВАЯ ЛОГИКА

// Исполняет приказ города
void MoveCity() {
  Unit* target = FindUnit(orderx, ordery); // Находим юнита в клетке куда отдан приказ
  if (target == nullptr) { // Если клетка пуста - создаем воина в ней
    CreateUnit(SWORDSMAN, turn, orderx, ordery);
    selected->moves--;
  }
}

// Исполняет приказ воина
void MoveSwordsman() {
  Unit* target = FindUnit(orderx, ordery); // Находим юнита в клетке куда отдан приказ

  // Если клетка пуста - передвигаемся в неё
  if (target == nullptr) {
    selected->x = orderx;
    selected->y = ordery;
    selected->moves--;
    return;
  }

  // Если клетка занята дружественным юнитом - приказ отменяется
  if (target->owner == turn)
    return;

  // Если клетка занята вражеским городом - город захвачен
  if (target->type == CITY) {
    target->owner = turn;
    target->moves = 0;
    selected->moves = 0;
    return;
  }

  // Если клетка занята вражеским воином - воин погибает и мы занимаем его место
  if (target->type == SWORDSMAN) {
    target->destroy();
    selected->x = orderx;
    selected->y = ordery;
    selected->moves = 0;
    return;
  }
}

// Ход текущего игрока. Действия повторяются в цикле:
//  (1) Игрок выбирает юнита `selected`;
//  (2) Затем выбирает клетку куда походить `orderx` и `ordery` (eсли был выбран город, то выбирается где построить юнита);
//  (3) Приказ исполняется.
// Пока не будет нажата кнопка "Следующий ход" - `turn_is_over`.
void OneTurn() {
  AddUnitMoves(turn); // Добавляем очки перемещений всем юнитам текущего игрока
  turn_is_over = false;
  selected = nullptr;
  while (true) {
    SelectUnit(); // (1) Выбираем юнита
    SelectOrder(); // (2) Выбираем приказ
    if (turn_is_over) break;

    // (3) Исполняем приказ
    int moves = selected->moves;
    switch (selected->type) {
      case CITY:      MoveCity();      break;
      case SWORDSMAN: MoveSwordsman(); break;
      default: Fatal("unexpected unit type");
    }
    // Выбранный юнит не может больше передвигаться или не смог исполнить приказ
    if (selected->moves == 0 || selected->moves == moves)
      selected = nullptr;
  }

  // В конце хода проверяем условие победы
  int enemy_cities = 0;
  for (Unit* unit = units; unit != units + max_units; unit++)
    if (unit->type == CITY && unit->owner != turn)
      enemy_cities++;
  if (enemy_cities == 0) { // Если все города захвачены
    winner = turn;
    game_is_over = true;
    // Показываем экран победы пока не нажата кнопка выхода из игры
    while (!IsKeyUpward(keyQuitGame))
      DrawGame();
  }
}

// Игровая сессия
void GameSession() {
  // Перед началом игры нужно очистить все глобальные переменные от мусора или от предыдущей игры
  ClearUnits();
  game_is_over = false;
  winner = NEUTRAL;
  turn = PLAYER1; // Первым ходит первый игрок

  // Создаем начальные города игрокам
  CreateUnit(CITY, PLAYER1, 1, 1);
  CreateUnit(CITY, PLAYER2, map_x - 2, map_y - 2);

  // Создаем нейтральные города
  for (int y = 1; y < map_y; y += 4)
    for (int x = 1; x < map_x; x += 4)
      if (FindUnit(x, y) == nullptr) // Если в клетке пока нет города
        CreateUnit(CITY, NEUTRAL, x, y);

  // Основной цикл игры: ходы игроков по очереди до победы
  while (true) {
    // Ходит игрок, номер которого сохранен в переменной `turn`
    OneTurn();

    // Игра завершается если нажали на выход или один из игроков победил
    if (game_is_over) break;

    // Передача хода
    if (turn == PLAYER1)
      turn = PLAYER2;
    else
      turn = PLAYER1;
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// MAIN

void EasyMain() {
  ResizeScreen(map_x * tile_x, map_y * tile_y + menu_y);

  // Загружаем файлы с диска в память
  g_font.Load("data/arctic_one_bmf.fnt");
  sprites[NEUTRAL][CITY     ].Load("data/grey_city.tga");
  sprites[PLAYER1][CITY     ].Load("data/red_city.tga");
  sprites[PLAYER1][SWORDSMAN].Load("data/red_sword.tga");
  sprites[PLAYER2][CITY     ].Load("data/blue_city.tga");
  sprites[PLAYER2][SWORDSMAN].Load("data/blue_sword.tga");
  sprite_grass.Load("data/grass.tga");
  sprite_frame.Load("data/frame.tga");

  do {
    GameSession(); // Запускаем сессию
  } while (winner != NEUTRAL); // Если сессия был доиграна до конца - запускаем новую
}
