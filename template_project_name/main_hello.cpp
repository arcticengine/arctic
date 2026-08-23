// Copyright (c) <year> Your name

#include "engine/easy.h"

using namespace arctic;  // NOLINT

Font g_font; ///< Font used to draw text

/// @brief Main function for the hello world example
void EasyMain() {
  // Load font
  g_font.Load("data/arctic_one_bmf.fnt"); // Load the font from the file

  // Loop until the window is closed or escape is pressed. Escape is this
  // program's own convention; the engine ends nothing on a key of its own.
  while (!IsMainWindowCloseRequested() && !IsKeyDownward(kKeyEscape)) {
    // Clear screen
    Clear();

    // Draw text
    char text[128]; // define a buffer for the text
    snprintf(text, sizeof(text), u8"Hello world!");  // write the text to the buffer
    g_font.Draw(text, 0, ScreenSize().y, kTextOriginTop);  // Draw content of the buffer at the top of the screen

    // Show frame
    ShowFrame(); // Displays the content of the frame buffer and updates the input state
  }
}
