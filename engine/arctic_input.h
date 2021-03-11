// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2016 - 2019 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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

#ifndef ENGINE_ARCTIC_INPUT_H_
#define ENGINE_ARCTIC_INPUT_H_

#include "engine/arctic_types.h"
#include "engine/vec2f.h"
#include "engine/vec2si32.h"

namespace arctic {

/// @addtogroup global_input
/// @{
enum KeyCode {
  kKeyNone = 0,   ///< Indicates absence of any key, like SQL null
  kKeyUnknown = 1,   ///< Indicates an unidentified key
  // 2
  // 3
  kKeyLeft = 4,
  kKeyRight = 5,
  kKeyUp = 6,
  kKeyDown = 7,
  kKeyBackspace = 8,   ///< ASCII Backspace
  kKeyTab = 9,   ///< ASCII Horizontal Tab

  kKeyEnter = 13,  // ASCII Carriage Return
  kKeyHome = 14,
  kKeyEnd = 15,
  kKeyPageUp = 16,
  kKeyPageDown = 17,
  kKeyShift = 18,
  kKeyLeftShift = 19,
  kKeyRightShift = 20,
  kKeyControl = 21,
  kKeyLeftControl = 22,
  kKeyRightControl = 23,
  kKeyAlt = 24,
  kKeyLeftAlt = 25,
  kKeyRightAlt = 26,
  kKeyEscape = 27,  // ASCII Escape
  // 28
  // 29
  // 30
  // 31
  kKeySpace = 32,  // ' ' ASCII Space  // SAFE to map BEGIN ----
  // 33
  // 34
  // 35
  // 36
  // 37
  // 38
  kKeyApostrophe = 39,  // '/'' ASCII Apostrophe
  // 40
  // 41
  // 42
  // 43
  kKeyComma = 44,  // ','
  kKeyMinus = 45,  // '-'
  kKeyPeriod = 46,  // '.'
  kKeySlash = 47,  // '//' ASCII Slash
  kKey0 = 48,  // '0'
  kKey1 = 49,  // '1'
  kKey2 = 50,  // '2'
  kKey3 = 51,  // '3'
  kKey4 = 52,  // '4'
  kKey5 = 53,  // '5'
  kKey6 = 54,  // '6'
  kKey7 = 55,  // '7'
  kKey8 = 56,  // '8'
  kKey9 = 57,  // '9'
  // 58
  kKeySemicolon = 59,  // ';'
  kKeyPause = 60,
  kKeyEquals = 61,  // '='
  kKeyNumLock = 62,
  kKeyScrollLock = 63,
  kKeyCapsLock = 64,
  kKeyA = 65,  // 'A'
  kKeyB = 66,  // 'B'
  kKeyC = 67,  // 'C'
  kKeyD = 68,  // 'D'
  kKeyE = 69,  // 'E'
  kKeyF = 70,  // 'F'
  kKeyG = 71,  // 'G'
  kKeyH = 72,  // 'H'
  kKeyI = 73,  // 'I'
  kKeyJ = 74,  // 'J'
  kKeyK = 75,  // 'K'
  kKeyL = 76,  // 'L'
  kKeyM = 77,  // 'M'
  kKeyN = 78,  // 'N'
  kKeyO = 79,  // 'O'
  kKeyP = 80,  // 'P'
  kKeyQ = 81,  // 'Q'
  kKeyR = 82,  // 'R'
  kKeyS = 83,  // 'S'
  kKeyT = 84,  // 'T'
  kKeyU = 85,  // 'U'
  kKeyV = 86,  // 'V'
  kKeyW = 87,  // 'W'
  kKeyX = 88,  // 'X'
  kKeyY = 89,  // 'Y'
  kKeyZ = 90,  // 'Z'
  kKeyLeftSquareBracket = 91,   // '['
  kKeyBackslash = 92,   // '\'
  kKeyRightSquareBracket = 93,   // ']'
  // 94
  // 95
  kKeyGraveAccent = 96,   // '`'  // SAFE to map END ----
  kKeyF1 = 97,
  kKeyF2 = 98,
  kKeyF3 = 99,
  kKeyF4 = 100,
  kKeyF5 = 101,
  kKeyF6 = 102,
  kKeyF7 = 103,
  kKeyF8 = 104,
  kKeyF9 = 105,
  kKeyF10 = 106,
  kKeyF11 = 107,
  kKeyF12 = 108,
  kKeyNumpad0 = 109,
  kKeyNumpad1 = 110,
  kKeyNumpad2 = 111,
  kKeyNumpad3 = 112,
  kKeyNumpad4 = 113,
  kKeyNumpad5 = 114,
  kKeyNumpad6 = 115,
  kKeyNumpad7 = 116,
  kKeyNumpad8 = 117,
  kKeyNumpad9 = 118,
  kKeyNumpadSlash = 119,
  kKeyNumpadAsterisk = 120,
  kKeyNumpadMinus = 121,
  kKeyNumpadPlus = 122,
  kKeyNumpadPeriod = 123,
  // kKeyNumpadEnter = 124,  // Missing in windows (?)
  kKeyPrintScreen = 125,
  kKeyInsert = 126,
  kKeyDelete = 127,  // ASCII Delete
  // [128, 244]
  kKeySectionSign = 245,  // ASCII Section sign
  // [246, 255]
  kKeyMouseUnknown = 256,
  kKeyMouseLeft = 257,
  kKeyMouseRight = 258,
  kKeyMouseWheel = 259,
  //
  kKeyController0Button0 = 260,
  kKeyController0Button1 = 261,
  kKeyController0Button2 = 262,
  kKeyController0Button3 = 263,
  kKeyController0Button4 = 264,
  kKeyController0Button5 = 265,
  kKeyController0Button6 = 266,
  kKeyController0Button7 = 267,
  kKeyController0Button8 = 268,
  kKeyController0Button9 = 269,
  kKeyController0Button10 = 270,
  kKeyController0Button11 = 271,
  kKeyController0Button12 = 272,
  kKeyController0Button13 = 273,
  kKeyController0Button14 = 274,
  kKeyController0Button15 = 275,
  kKeyController0Button16 = 276,
  kKeyController0Button17 = 277,
  kKeyController0Button18 = 278,
  kKeyController0Button19 = 279,
  kKeyController0Button20 = 280,
  kKeyController0Button21 = 281,
  kKeyController0Button22 = 282,
  kKeyController0Button23 = 283,
  kKeyController0Button24 = 284,
  kKeyController0Button25 = 285,
  kKeyController0Button26 = 286,
  kKeyController0Button27 = 287,
  kKeyController0Button28 = 288,
  kKeyController0Button29 = 289,
  kKeyController0Button30 = 290,
  kKeyController0Button31 = 291,
  //
  kKeyController1Button0 = 292,
  kKeyController1Button1 = 293,
  kKeyController1Button2 = 294,
  kKeyController1Button3 = 295,
  kKeyController1Button4 = 296,
  kKeyController1Button5 = 297,
  kKeyController1Button6 = 298,
  kKeyController1Button7 = 299,
  kKeyController1Button8 = 300,
  kKeyController1Button9 = 301,
  kKeyController1Button10 = 302,
  kKeyController1Button11 = 303,
  kKeyController1Button12 = 304,
  kKeyController1Button13 = 305,
  kKeyController1Button14 = 306,
  kKeyController1Button15 = 307,
  kKeyController1Button16 = 308,
  kKeyController1Button17 = 309,
  kKeyController1Button18 = 310,
  kKeyController1Button19 = 311,
  kKeyController1Button20 = 312,
  kKeyController1Button21 = 313,
  kKeyController1Button22 = 314,
  kKeyController1Button23 = 315,
  kKeyController1Button24 = 316,
  kKeyController1Button25 = 317,
  kKeyController1Button26 = 318,
  kKeyController1Button27 = 319,
  kKeyController1Button28 = 320,
  kKeyController1Button29 = 321,
  kKeyController1Button30 = 322,
  kKeyController1Button31 = 323,
  //
  kKeyController2Button0 = 324,
  kKeyController2Button31 = 355,
  //
  kKeyController3Button0 = 356,
  kKeyController3Button31 = 387,
  //
  kKeyCount = 388  // Key count, not a code
};

enum ControllerAxis {
  kAxis0 = 0,
  kAxis1,
  kAxis2,
  kAxis3,
  kAxis4,
  kAxis5,
  kAxisCount
};

struct InputMessage {
  constexpr static Si32 kControllerCount = 4;
  enum Kind {
    kKeyboard = 0,
    kMouse = 1,
    kController = 2
  };
  struct Keyboard {
    Ui32 state[kKeyCount] = {0};
    Ui32 key = 0;
    Ui32 key_state = 0;
    char characters[16]  = {0};
    Ui32 queue[1024] = {0};
    Ui32 queueLen = 0;
  };
  struct Mouse {
    Vec2F pos;
    Vec2Si32 backbuffer_pos;  // Not set by the engine
    Si32 wheel_delta = 0;
  };
  struct Controller {
    Si32 controller_idx = 0;
    float axis[kAxisCount] = {0};
  };
  Kind kind = kKeyboard;
  Keyboard keyboard;
  Mouse mouse;
  Controller controller;
};
/// @}

/// @addtogroup global_advanced
/// @{

/// @brief Obtains the next available input message from the queue
/// @param [out] out_message Address to write the message to
/// @return true if the message is successfuly poped, false otherwise
bool PopInputMessage(InputMessage *out_message);
/// @brief Pushes an input message to the queue as if it came from the user
/// @param [in] message Message to push
void PushInputMessage(const InputMessage &message);

/// @}

}  // namespace arctic



#endif  // ENGINE_ARCTIC_INPUT_H_
