// The MIT License (MIT)
//
// Copyright (c) 2017 - 2020 Huldra
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

#ifndef ENGINE_EASY_INPUT_H_
#define ENGINE_EASY_INPUT_H_

#include <string>

#include "engine/arctic_input.h"
#include "engine/arctic_types.h"
#include "engine/vec2si32.h"

namespace arctic {

/// @addtogroup global_input
/// @{

// true if key transitioned from up to down state last frame
/// @brief Returns true if the key with the specified key_code travelled downwards during the last frame
/// @param key_code The key code
bool IsKeyDownward(const KeyCode key_code);
/// @brief Returns true if any of the specified keys travelled downwards during the last frame
/// @param keys A c-string specifying one or more key character codes to check
bool IsKeyDownward(const char *keys);
/// @brief Returns true if the key specified travelled downwards during the last frame
/// @param key The key character code
bool IsKeyDownward(const char key);
/// @brief Returns true if the key with the specified key_code travelled downwards during the last frame
/// @param key_code The key code
bool IsKeyDownward(const Si32 key_code);
/// @brief Returns true if any of the specified keys travelled downwards during the last frame
/// @param keys A std::string specifying one or more key character codes to check
bool IsKeyDownward(const std::string &keys);

// true is key is currently down
/// @brief Returns true if the key with the specified key_code was pressed during the last frame
/// @param key_code The key code
bool IsKeyDown(const KeyCode key_code);
/// @brief Returns true if any of the specified keys was pressed during the last frame
/// @param keys A c-string specifying one or more key character codes to check
bool IsKeyDown(const char *keys);
/// @brief Returns true if the key specified was pressed during the last frame
/// @param key The key character code
bool IsKeyDown(const char key);
/// @brief Returns true if the key with the specified key_code was pressed during the last frame
/// @param key_code The key code
bool IsKeyDown(const Si32 key_code);
/// @brief Returns true if any of the specified keys was pressed during the last frame
/// @param keys A std::string specifying one or more key character codes to check
bool IsKeyDown(const std::string &keys);

// true if key transitioned from down to up since last frame
/// @brief Returns true if the key with the specified key_code was released during the last frame
/// @param key_code The key code
bool IsKeyUpward(const KeyCode key_code);
/// @brief Returns true if any of the specified keys was released during the last frame
/// @param keys A c-string specifying one or more key character codes to check
bool IsKeyUpward(const char *keys);
/// @brief Returns true if the key specified was released during the last frame
/// @param key The key character code
bool IsKeyUpward(const char key);
/// @brief Returns true if the key with the specified key_code was released during the last frame
/// @param key_code The key code
bool IsKeyUpward(const Si32 key_code);
/// @brief Returns true if any of the specified keys was released during the last frame
/// @param keys A std::string specifying one or more key character codes to check
bool IsKeyUpward(const std::string &keys);


/// @brief Returns true if key transitioned from up to down state last frame
bool IsAnyKeyDownward();
/// @brief Returns true if key is currently down
bool IsAnyKeyDown();
/// @brief Returns true if key transitioned from down to up since last frame
bool IsAnyKeyUpward();

/// @brief Changes the stored key state
/// @param key_code The key code
/// @param is_set_down Key state to set as if it was the last seen state of the key
/// @details The state is set just like it would be if the key actually transitioned
/// up or down at the end of the last frame. It may affect resuts of
/// IsKeyDownward() IsKeyDown() IsKeyUpward() IsAnyKeyDownward() IsAnyKeyDown() and
/// IsAnyKeyUpward() calls.
void SetKey(const KeyCode key_code, bool is_set_down);

/// @brief Changes the stored key state
/// @param key The key character code
/// @param is_set_down Key state to set as if it was the last seen state of the key
/// @details The state is set just like it would be if the key actually
/// transitioned up or down at the end of the last frame. It may affect
/// resuts of IsKeyDownward() IsKeyDown() IsKeyUpward() IsAnyKeyDownward()
/// IsAnyKeyDown() and IsAnyKeyUpward() calls.
void SetKey(const char key, bool is_set_down);

/// @brief Clears key state transition information
/// @details IsKeyDownward() IsKeyUpward() IsAnyKeyDownward() and IsAnyKeyUpward()
/// calls will return false after a ClearKeyStateTransitions() call
/// untill the ShowFrame() call fills the key state transition infromation with
/// the information on new transitions.
/// IsKeyDown() IsAnyKeyDown() calls are not affected and return the actual state.
void ClearKeyStateTransitions();

/// @brief Returns controller axis position
float ControllerAxis(Si32 controller_idx, Si32 axis_idx);
/// @brief Returns mouse cursor position
Vec2Si32 MousePos();
/// @brief Returns mouse cursor position x coordinate
Si32 MouseX();
/// @brief Returns mouse cursor position y coordinate
Si32 MouseY();
/// @brief Returns mouse movement vector
Vec2Si32 MouseMove();
/// @brief Returns mouse wheel rotation delta
Si32 MouseWheelDelta();

/// @brief Returns the number of user input messages obtained by the
/// last Swap() call
Si32 InputMessageCount();

/// @brief Returns the user input message with the index specified
const InputMessage& GetInputMessage(Si32 idx);

/// @}

bool IsKey(const KeyCode key_code);
bool IsKey(const char *keys);
bool IsKey(const char key);
bool IsKey(const Si32 key_code);
bool IsKey(const std::string &keys);

}  // namespace arctic

#endif  // ENGINE_EASY_INPUT_H_
