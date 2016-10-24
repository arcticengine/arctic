// The MIT License(MIT)
//
// Copyright 2016 Huldra
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

#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include "engine/arctic_types.h"
#include "demo/targetver.h"

#define MAX_LOADSTRING 255

// Forward declarations of functions included in this code module:
bool CreateMainWindow(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE instance_handle,
    _In_opt_ HINSTANCE prev_instance_handle,
    _In_ LPWSTR    command_line,
    _In_ int       cmd_show) {
  UNREFERENCED_PARAMETER(prev_instance_handle);
  UNREFERENCED_PARAMETER(command_line);

  BOOL is_ok = SetProcessDPIAware();
  // TODO(Huldra): Check is_ok

  if (!CreateMainWindow(instance_handle, cmd_show)) {
    return FALSE;
  }

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return static_cast<int>(msg.wParam);
}

//
// Creates main window.
//
bool CreateMainWindow(HINSTANCE instance_handle, int cmd_show) {
  WCHAR title_bar_text[MAX_LOADSTRING];
  WCHAR window_class_name[MAX_LOADSTRING];

  LoadStringW(instance_handle, IDS_APP_TITLE, title_bar_text, MAX_LOADSTRING);
  LoadStringW(instance_handle, IDC_DEMO, window_class_name, MAX_LOADSTRING);

  WNDCLASSEXW wcex;
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = instance_handle;
  wcex.hIcon = LoadIcon(instance_handle, MAKEINTRESOURCE(IDI_APP_ICON));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DEMO);
  wcex.lpszClassName = window_class_name;
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL_APP_ICON));

  ATOM register_class_result = RegisterClassExW(&wcex);

  HWND window_handle = CreateWindowW(window_class_name, title_bar_text,
    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr,
    instance_handle, nullptr);
  if (!window_handle) {
    return false;
  }

  ShowWindow(window_handle, cmd_show);
  UpdateWindow(window_handle);
  return true;
}

//
//  Processes messages for the main window.
//
LRESULT CALLBACK WndProc(HWND window_handle, UINT message,
  WPARAM word_param, LPARAM long_param) {
  switch (message) {
  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(window_handle, &ps);
    // TODO(Huldra): Add any drawing code that uses hdc here...
    EndPaint(window_handle, &ps);
  }
  break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(window_handle, message, word_param, long_param);
  }
  return 0;
}
