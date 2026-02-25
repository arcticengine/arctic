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

#ifndef __AVAILABILITY_INTERNAL_DEPRECATED
#define __AVAILABILITY_INTERNAL_DEPRECATED
#endif

namespace arctic {

/// @addtogroup global_input
/// @{

/// @name Key State Transition Functions
/// @{

/// Checks if a key was pressed down during the last frame
/// @param key_code The key code to check
/// @return True if the key transitioned from up to down state during the last frame
/// @note This function returns true ONLY on the first frame when a key is pressed.
///       It does NOT return true while the key is being held down.
///       Use IsKeyDown() for continuous key press detection (e.g., character movement).
///       Use IsKeyDownward() for one-time actions (e.g., menu selection, jumping).
bool IsKeyDownward(const KeyCode key_code);

/// Checks if any of the specified keys was pressed down during the last frame
/// @param keys A C-string containing one or more key character codes to check
/// @return True if any of the specified keys transitioned from up to down state during the last frame
/// @note This function returns true ONLY on the first frame when any of the keys are pressed.
///       It does NOT return true while the keys are being held down.
///       Use IsKeyDown() for continuous key press detection (e.g., character movement).
///       Use IsKeyDownward() for one-time actions (e.g., menu selection, jumping).
bool IsKeyDownward(const char *keys);

/// Checks if a specific key was pressed down during the last frame
/// @param key The key character code to check
/// @return True if the key transitioned from up to down state during the last frame
/// @note This function returns true ONLY on the first frame when the key is pressed.
///       It does NOT return true while the key is being held down.
///       Use IsKeyDown() for continuous key press detection (e.g., character movement).
///       Use IsKeyDownward() for one-time actions (e.g., menu selection, jumping).
bool IsKeyDownward(const char key);

/// Checks if a key was pressed down during the last frame
/// @param key_code The integer key code to check
/// @return True if the key transitioned from up to down state during the last frame
/// @note This function returns true ONLY on the first frame when the key is pressed.
///       It does NOT return true while the key is being held down.
///       Use IsKeyDown() for continuous key press detection (e.g., character movement).
///       Use IsKeyDownward() for one-time actions (e.g., menu selection, jumping).
bool IsKeyDownward(const Si32 key_code);

/// Checks if any of the specified keys was pressed down during the last frame
/// @param keys A string containing one or more key character codes to check
/// @return True if any of the specified keys transitioned from up to down state during the last frame
/// @note This function returns true ONLY on the first frame when any of the keys are pressed.
///       It does NOT return true while the keys are being held down.
///       Use IsKeyDown() for continuous key press detection (e.g., character movement).
///       Use IsKeyDownward() for one-time actions (e.g., menu selection, jumping).
bool IsKeyDownward(const std::string &keys);

/// Checks if a key is currently being held down
/// @param key_code The key code to check
/// @return True if the key is currently in the down state
/// @note This function returns true as long as the key is being held down.
///       Use this for continuous actions like character movement or camera control.
///       For one-time actions (e.g., menu selection, jumping), use IsKeyDownward() instead.
bool IsKeyDown(const KeyCode key_code);

/// Checks if any of the specified keys is currently being held down
/// @param keys A C-string containing one or more key character codes to check
/// @return True if any of the specified keys is currently in the down state
/// @note This function returns true as long as any of the keys are being held down.
///       Use this for continuous actions like character movement or camera control.
///       For one-time actions (e.g., menu selection, jumping), use IsKeyDownward() instead.
bool IsKeyDown(const char *keys);

/// Checks if a specific key is currently being held down
/// @param key The key character code to check
/// @return True if the key is currently in the down state
/// @note This function returns true as long as the key is being held down.
///       Use this for continuous actions like character movement or camera control.
///       For one-time actions (e.g., menu selection, jumping), use IsKeyDownward() instead.
bool IsKeyDown(const char key);

/// Checks if a key is currently being held down
/// @param key_code The integer key code to check
/// @return True if the key is currently in the down state
/// @note This function returns true as long as the key is being held down.
///       Use this for continuous actions like character movement or camera control.
///       For one-time actions (e.g., menu selection, jumping), use IsKeyDownward() instead.
bool IsKeyDown(const Si32 key_code);

/// Checks if any of the specified keys is currently being held down
/// @param keys A string containing one or more key character codes to check
/// @return True if any of the specified keys is currently in the down state
/// @note This function returns true as long as any of the keys are being held down.
///       Use this for continuous actions like character movement or camera control.
///       For one-time actions (e.g., menu selection, jumping), use IsKeyDownward() instead.
bool IsKeyDown(const std::string &keys);

/// Checks if a key was released during the last frame
/// @param key_code The key code to check
/// @return True if the key transitioned from down to up state during the last frame
bool IsKeyUpward(const KeyCode key_code);

/// Checks if any of the specified keys was released during the last frame
/// @param keys A C-string containing one or more key character codes to check
/// @return True if any of the specified keys transitioned from down to up state during the last frame
bool IsKeyUpward(const char *keys);

/// Checks if a specific key was released during the last frame
/// @param key The key character code to check
/// @return True if the key transitioned from down to up state during the last frame
bool IsKeyUpward(const char key);

/// Checks if a key was released during the last frame
/// @param key_code The integer key code to check
/// @return True if the key transitioned from down to up state during the last frame
bool IsKeyUpward(const Si32 key_code);

/// Checks if any of the specified keys was released during the last frame
/// @param keys A string containing one or more key character codes to check
/// @return True if any of the specified keys transitioned from down to up state during the last frame
bool IsKeyUpward(const std::string &keys);

/// @}

/// @name Global Key State Functions
/// @{

/// Checks if any key was pressed down during the last frame
/// @return True if any key transitioned from up to down state during the last frame
bool IsAnyKeyDownward();

/// Checks if any key is currently being held down
/// @return True if any key is currently in the down state
bool IsAnyKeyDown();

/// Checks if any key was released during the last frame
/// @return True if any key transitioned from down to up state during the last frame
bool IsAnyKeyUpward();

/// @}

/// @name Key State Manipulation Functions
/// @{

/// Sets the state of a key as if it was pressed or released at the end of the last frame
/// @param key_code The key code to modify
/// @param is_set_down True to set the key as pressed, false to set it as released
/// @note The state is set just like it would be if the key actually transitioned
///       up or down at the end of the last frame.
///       This affects the results of IsKeyDownward(), IsKeyDown(), IsKeyUpward(),
///       IsAnyKeyDownward(), IsAnyKeyDown(), and IsAnyKeyUpward() calls
void SetKey(const KeyCode key_code, bool is_set_down);

/// Sets the state of a key as if it was pressed or released at the end of the last frame
/// @param key The key character code to modify
/// @param is_set_down True to set the key as pressed, false to set it as released
/// @note The state is set just like it would be if the key actually
///       transitioned up or down at the end of the last frame.
///   This affects the results of IsKeyDownward(), IsKeyDown(), IsKeyUpward(),
///       IsAnyKeyDownward(), IsAnyKeyDown(), and IsAnyKeyUpward() calls
void SetKey(const char key, bool is_set_down);

/// Clears all key state transition information
/// @note After calling this function, IsKeyDownward(), IsKeyUpward(), IsAnyKeyDownward(),
///       and IsAnyKeyUpward() will return false until the next ShowFrame() call updates
///       the transition information. IsKeyDown() and IsAnyKeyDown() are not affected.
void ClearKeyStateTransitions();

/// @}

/// @name Controller and Mouse Input Functions
/// @{

/// Gets the position of a controller axis
/// @param controller_idx Index of the controller (0 for the first controller)
/// @param axis_idx Index of the axis on the controller
/// @return The position of the axis as a float value between -1.0 and 1.0
float ControllerAxis(Si32 controller_idx, Si32 axis_idx);

/// Gets the current mouse cursor position
/// @return The mouse cursor position as a 2D vector (x, y)
Vec2Si32 MousePos();

/// Gets the current mouse cursor X coordinate
/// @return The mouse cursor X coordinate
Si32 MouseX();

/// Gets the current mouse cursor Y coordinate
/// @return The mouse cursor Y coordinate
Si32 MouseY();

/// Gets the mouse movement vector since the last frame
/// @return The mouse movement as a 2D vector (delta_x, delta_y)
Vec2Si32 MouseMove();

/// Gets the mouse wheel rotation since the last frame
/// @return The mouse wheel delta (positive for scrolling up, negative for scrolling down)
Si32 MouseWheelDelta();

/// @}

/// @name Input Message Functions
/// @{

/// Gets the number of input messages received during the last frame
/// @return The number of input messages
Si32 InputMessageCount();

/// Gets a specific input message from the last frame
/// @param idx Index of the message to retrieve (0 to InputMessageCount()-1)
/// @return The input message at the specified index
const InputMessage& GetInputMessage(Si32 idx);

/// @}

/// @} // End of global_input group

/// @name Legacy Key State Functions
/// @{

/// Legacy function to check if a key is currently being held down
/// @param key_code The key code to check
/// @return True if the key is currently in the down state
/// @deprecated Use IsKeyDown() instead
__AVAILABILITY_INTERNAL_DEPRECATED bool IsKey(const KeyCode key_code);

/// Legacy function to check if any of the specified keys is currently being held down
/// @param keys A C-string containing one or more key character codes to check
/// @return True if any of the specified keys is currently in the down state
/// @deprecated Use IsKeyDown() instead
__AVAILABILITY_INTERNAL_DEPRECATED bool IsKey(const char *keys);

/// Legacy function to check if a specific key is currently being held down
/// @param key The key character code to check
/// @return True if the key is currently in the down state
/// @deprecated Use IsKeyDown() instead
__AVAILABILITY_INTERNAL_DEPRECATED bool IsKey(const char key);

/// Legacy function to check if a key is currently being held down
/// @param key_code The integer key code to check
/// @return True if the key is currently in the down state
/// @deprecated Use IsKeyDown() instead
__AVAILABILITY_INTERNAL_DEPRECATED bool IsKey(const Si32 key_code);

/// Legacy function to check if any of the specified keys is currently being held down
/// @param keys A string containing one or more key character codes to check
/// @return True if any of the specified keys is currently in the down state
/// @deprecated Use IsKeyDown() instead
__AVAILABILITY_INTERNAL_DEPRECATED bool IsKey(const std::string &keys);

/// @}

}  // namespace arctic

#endif  // ENGINE_EASY_INPUT_H_
