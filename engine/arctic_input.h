// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2016 - 2017 Huldra
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

#include "engine/arctic_math.h"
#include "engine/arctic_types.h"

namespace arctic {

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
  kKeyCount = 260  // Key count, not a code
};

struct InputMessage {
  enum Kind {
    kKeyboard = 0,
    kMouse = 1
  };
  struct Keyboard {
    Ui32 state[kKeyCount];
    Ui32 key;
    Ui32 key_state;
    Ui32 queue[1024];
    Ui32 queueLen;
  };
  struct Mouse {
    Vec2F pos;
    Vec2Si32 backbuffer_pos;  // Not set by the engine
    Si32 wheel_delta;
  };
  Kind kind;
  Keyboard keyboard;
  Mouse mouse;
};

bool PopInputMessage(InputMessage *out_message);
void PushInputMessage(const InputMessage &message);

}  // namespace arctic

#endif  // ENGINE_ARCTIC_INPUT_H_
