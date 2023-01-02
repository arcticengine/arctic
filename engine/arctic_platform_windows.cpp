// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2017 - 2022 Huldra
// Copyright (c) 2021 Vlad2001_MFS
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

#include "engine/arctic_platform.h"
#include "engine/arctic_platform_def.h"

#ifdef ARCTIC_PLATFORM_WINDOWS

#define IDI_ICON1 129

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <winsock2.h>
#include <Mmsystem.h>
#include <shellapi.h>
#include <Shlwapi.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>  // NOLINT
#include <vector>

#include "engine/engine.h"
#include "engine/easy.h"
#include "engine/arctic_input.h"
#include "engine/arctic_mixer.h"
#include "engine/log.h"
#include "engine/rgb.h"
#include "engine/vec3f.h"
#include "engine/unicode.h"

#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Ws2_32.lib")


extern void EasyMain();


PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;
PFNGLUNIFORM2IPROC glUniform2i = nullptr;
PFNGLUNIFORM3IPROC glUniform3i = nullptr;
PFNGLUNIFORM4IPROC glUniform4i = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM2FPROC glUniform2f = nullptr;
PFNGLUNIFORM3FPROC glUniform3f = nullptr;
PFNGLUNIFORM4FPROC glUniform4f = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLBUFFERSUBDATAPROC glBufferSubData = nullptr;

namespace arctic {

Ui16 FromBe(Ui16 x) {
  return ntohs(x);
}
Si16 FromBe(Si16 x) {
  return ntohs(x);
}
Ui32 FromBe(Ui32 x) {
  return ntohl(x);
}
Si32 FromBe(Si32 x) {
  return ntohl(x);
}
Ui16 ToBe(Ui16 x) {
  return htons(x);
}
Si16 ToBe(Si16 x) {
  return htons(x);
}
Ui32 ToBe(Ui32 x) {
  return htonl(x);
}
Si32 ToBe(Si32 x) {
  return htonl(x);
}

inline void Check(bool condition, const char *error_message,
  const char *error_message_postfix) {
  if (condition) {
    return;
  }
  Fatal(error_message, error_message_postfix);
}

void Fatal(const char *message, const char *message_postfix) {
  size_t size = 1 +
    strlen(message) +
    (message_postfix ? strlen(message_postfix) : 0);
  char *full_message = static_cast<char *>(LocalAlloc(LMEM_ZEROINIT, size));
  if (!full_message) {
    full_message = "Not enough memroy to report a fatal error!";
  } else {
    sprintf_s(full_message, size, "%s%s", message,
      (message_postfix ? message_postfix : ""));
  }
  Log(full_message);
  MessageBox(NULL, full_message, "Arctic Engine", MB_OK | MB_ICONERROR);
  StopLogger();
  ExitProcess(1);  //-V2014
}

static void FatalWithLastError(const char* message_prefix,
  const char* message_infix = nullptr,
  const char* message_postfix = nullptr) {
  DWORD dw = GetLastError();
  char *message_info = "";
  char *message = "";
  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    dw,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR)&message_info,
    0, NULL);

  size_t size = 1 +
    strlen(message_prefix) +
    strlen(message_info) +
    (message_infix ? strlen(message_infix) : 0) +
    (message_postfix ? strlen(message_postfix) : 0);
  message = static_cast<char *>(LocalAlloc(LMEM_ZEROINIT, size));
  if (message) {
    sprintf_s(message, size, "%s%s%s%s", message_prefix, message_info,
      (message_infix ? message_infix : ""),
      (message_postfix ? message_postfix : ""));
  } else {
    message = "Not enough memory to report details on a fatal error!";
  }
  Fatal(message);
}

static void CheckWithLastError(bool condition, const char *message_prefix,
  const char *message_infix = nullptr,
  const char *message_suffix = nullptr) {
  if (condition) {
    return;
  }
  FatalWithLastError(message_prefix, message_infix, message_suffix);
}

static const PIXELFORMATDESCRIPTOR pfd = {
  sizeof(PIXELFORMATDESCRIPTOR),
  1,
  PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
  PFD_TYPE_RGBA,
  32,
  0, 0, 0, 0, 0, 0, 8, 0,
  0, 0, 0, 0, 0,  // accum
  32,             // zbuffer
  0,              // stencil!
  0,              // aux
  PFD_MAIN_PLANE,
  0, 0, 0, 0
};

struct SystemInfo {
  HINSTANCE instance_handle;
  HWND window_handle;
  HWND inner_window_handle;
  Si32 screen_width;
  Si32 screen_height;
};

SystemInfo g_system_info;
static bool g_is_full_screen = false;
static Si32 g_window_width = 0;
static Si32 g_window_height = 0;
static std::atomic<bool> g_is_cursor_desired = true;
static std::atomic<bool> g_is_cursor_visible = true;

KeyCode TranslateKeyCode(WPARAM word_param) {  //-V2008
  if (word_param >= 'A' && word_param <= 'Z') {
    return static_cast<KeyCode>(word_param - 'A' + kKeyA);
  }
  if (word_param >= '0' && word_param <= '9') {
    return static_cast<KeyCode>(word_param - '0' + kKey0);
  }
  if (word_param >= VK_F1 && word_param <= VK_F12) {
    return static_cast<KeyCode>(word_param - VK_F1 + kKeyF1);
  }
  if (word_param >= VK_NUMPAD0 && word_param <= VK_NUMPAD9) {
    return static_cast<KeyCode>(word_param - VK_NUMPAD0 + kKeyNumpad0);
  }

  switch (word_param) {
  case VK_LEFT:
    return kKeyLeft;
  case VK_RIGHT:
    return kKeyRight;
  case VK_UP:
    return kKeyUp;
  case VK_DOWN:
    return kKeyDown;
  case VK_BACK:
    return kKeyBackspace;
  case VK_TAB:
    return kKeyTab;
  case VK_RETURN:
    return kKeyEnter;
  case VK_HOME:
    return kKeyHome;
  case VK_END:
    return kKeyEnd;
  case VK_PRIOR:
    return kKeyPageUp;
  case VK_NEXT:
    return kKeyPageDown;
  case VK_SHIFT:
    return kKeyShift;
  case VK_LSHIFT:
    return kKeyLeftShift;
  case VK_RSHIFT:
    return kKeyRightShift;
  case VK_CONTROL:
    return kKeyControl;
  case VK_LCONTROL:
    return kKeyLeftControl;
  case VK_RCONTROL:
    return kKeyRightControl;
  case VK_MENU:
    return kKeyAlt;
  case VK_LMENU:
    return kKeyLeftAlt;
  case VK_RMENU:
    return kKeyRightAlt;
  case VK_ESCAPE:
    return kKeyEscape;
  case VK_SPACE:
    return kKeySpace;
  case VK_PAUSE:
    return kKeyPause;
  case VK_NUMLOCK:
    return kKeyNumLock;
  case VK_SCROLL:
    return kKeyScrollLock;
  case VK_CAPITAL:
    return kKeyCapsLock;
  case VK_SNAPSHOT:
    return kKeyPrintScreen;
  case VK_INSERT:
    return kKeyInsert;
  case VK_DELETE:
    return kKeyDelete;
  case VK_DIVIDE:
    return kKeyNumpadSlash;
  case VK_MULTIPLY:
    return kKeyNumpadAsterisk;
  case VK_SUBTRACT:
    return kKeyNumpadMinus;
  case VK_ADD:
    return kKeyNumpadPlus;
  case VK_DECIMAL:
    return kKeyNumpadPeriod;
  case VK_OEM_COMMA:
    return kKeyComma;
  case VK_OEM_PERIOD:
    return kKeyPeriod;
  case VK_OEM_MINUS:
    return kKeyMinus;
  case VK_OEM_PLUS:
    return kKeyEquals;
  case VK_OEM_1:
    return kKeySemicolon;
  case VK_OEM_2:
    return kKeySlash;
  case VK_OEM_3:
    return kKeyGraveAccent;
  case VK_OEM_4:
    return kKeyLeftSquareBracket;
  case VK_OEM_5:
    return kKeyBackslash;
  case VK_OEM_6:
    return kKeyRightSquareBracket;
  case VK_OEM_7:
    return kKeyApostrophe;
  case VK_OEM_8:
    return kKeySectionSign;
  }
  return kKeyUnknown;
}

void OnMouse(KeyCode key, WPARAM word_param, LPARAM long_param, bool is_down) {
  Check(g_window_width != 0, "Could not obtain window width in OnMouse");
  Check(g_window_height != 0, "Could not obtain window height in OnMouse");
  Si32 x = GET_X_LPARAM(long_param);
  Si32 y = g_window_height - GET_Y_LPARAM(long_param);
  Vec2F pos(static_cast<float>(x) / static_cast<float>(g_window_width - 1),
    static_cast<float>(y) / static_cast<float>(g_window_height - 1));
  InputMessage msg;
  msg.kind = InputMessage::kMouse;
  msg.keyboard.key = key;
  msg.keyboard.key_state = (is_down ? 1 : 2);
  msg.keyboard.characters[0] = '\0';
  msg.mouse.pos = pos;
  msg.mouse.wheel_delta = 0;
  PushInputMessage(msg);
  if (!g_is_cursor_visible) {
    SetCursor(NULL);
  }
}

void OnMouseWheel(WPARAM word_param, LPARAM long_param) {
  Check(g_window_width != 0, "Could not obtain window width in OnMouseWheel");
  Check(g_window_height != 0,
    "Could not obtain window height in OnMouseWheel");

  Si32 fw_keys = GET_KEYSTATE_WPARAM(word_param);
  Si32 z_delta = GET_WHEEL_DELTA_WPARAM(word_param);

  Si32 x = GET_X_LPARAM(long_param);
  Si32 y = g_window_height - GET_Y_LPARAM(long_param);

  Vec2F pos(static_cast<float>(x) / static_cast<float>(g_window_width - 1),
    static_cast<float>(y) / static_cast<float>(g_window_height - 1));
  InputMessage msg;
  msg.kind = InputMessage::kMouse;
  msg.keyboard.key = kKeyCount;
  msg.keyboard.key_state = 0;
  msg.keyboard.characters[0] = '\0';
  msg.mouse.pos = pos;
  msg.mouse.wheel_delta = z_delta;
  PushInputMessage(msg);
}

void ToggleFullscreen() {
  SetFullScreen(!g_is_full_screen);
}

bool IsFullScreen() {
  return g_is_full_screen;
}

void SetFullScreen(bool is_enable) {
  if (is_enable == g_is_full_screen) {
    return;
  }
  g_is_full_screen = is_enable;
  if (g_is_full_screen) {
    SetWindowLongPtr(g_system_info.window_handle, GWL_STYLE,
      WS_POPUP | WS_VISIBLE);
    ShowWindow(g_system_info.window_handle, SW_RESTORE);
    SetWindowPos(g_system_info.window_handle, HWND_TOP, 0, 0, 0, 0,
      SWP_FRAMECHANGED | SWP_NOSIZE);
    ShowWindow(g_system_info.window_handle, SW_SHOWMAXIMIZED);
  } else {
    SetWindowLongPtr(g_system_info.window_handle, GWL_STYLE,
      WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    SetWindowPos(g_system_info.window_handle, 0, 0, 0, 0, 0,
      SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOZORDER);
  }
  RECT client_rect;
  GetClientRect(g_system_info.window_handle, &client_rect);
  SetWindowPos(g_system_info.inner_window_handle, 0, 0, 0,
    client_rect.right - client_rect.left,
    client_rect.bottom - client_rect.top,
    SWP_NOZORDER);
}

bool IsCursorVisible() {
  return g_is_cursor_desired;
}

void SetCursorVisible(bool is_enable) {
  g_is_cursor_desired = is_enable;
  if (g_is_cursor_visible != is_enable) {
    ShowCursor(is_enable);
    g_is_cursor_visible = is_enable;
  }
  if (!g_is_cursor_visible) {
    SetCursor(NULL);
  }
}

void OnKey(WPARAM word_param, LPARAM long_param, bool is_down) {
  KeyCode key = TranslateKeyCode(word_param);
  InputMessage msg;
  msg.kind = InputMessage::kKeyboard;
  msg.keyboard.key = key;
  msg.keyboard.key_state = (is_down ? 1 : 2);
  msg.keyboard.characters[0] = '\0';
  PushInputMessage(msg);
}

void OnChar(WPARAM word_param, LPARAM long_param) {
  switch (word_param) {
  case 0x08:  // backspace
    return;
  case 0x0A:  // linefeed
    return;
  case 0x1B:  // escape
    return;
  case 0x09:  // tab
    break;
  case 0x0D:  // carriage return
    return;
  default:  // isplayable characters
    break;
  }
  InputMessage msg;
  msg.kind = InputMessage::kKeyboard;
  msg.keyboard.key = kKeyUnknown;
  msg.keyboard.key_state = 1;
  char utf16[16];
  memset(utf16, 0, sizeof(utf16));
  memcpy(utf16, &word_param, sizeof(word_param));
  memset(msg.keyboard.characters, 0, sizeof(msg.keyboard.characters));
  strncpy(msg.keyboard.characters, Utf16ToUtf8(utf16).c_str(),
      sizeof(msg.keyboard.characters));
  msg.keyboard.characters[sizeof(msg.keyboard.characters) - 1] = '\0';
  PushInputMessage(msg);
}

LRESULT CALLBACK WndProc(HWND window_handle, UINT message,
  WPARAM word_param, LPARAM long_param) {
  switch (message) {
  case WM_ERASEBKGND:
    return 1;
  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(window_handle, &ps);
    // TODO(Huldra): Add any drawing code that uses hdc here...
    EndPaint(window_handle, &ps);
    break;
  }
  case WM_KEYUP:
    arctic::OnKey(word_param, long_param, false);
    break;
  case WM_KEYDOWN:
    arctic::OnKey(word_param, long_param, true);
    break;
  case WM_CHAR:
    arctic::OnChar(word_param, long_param);
    break;
  case WM_SYSKEYDOWN:
    if (word_param == VK_RETURN && (HIWORD(long_param) & KF_ALTDOWN)) {
      ToggleFullscreen();
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

LRESULT CALLBACK InnerWndProc(HWND inner_window_handle, UINT message,
  WPARAM word_param, LPARAM long_param) {
  switch (message) {
  case WM_ERASEBKGND:
    return 1;
  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(inner_window_handle, &ps);
    // TODO(Huldra): Add any drawing code that uses hdc here...
    EndPaint(inner_window_handle, &ps);
    break;
  }
  case WM_LBUTTONUP:
    arctic::OnMouse(kKeyMouseLeft, word_param, long_param, false);
    break;
  case WM_LBUTTONDOWN:
    arctic::OnMouse(kKeyMouseLeft, word_param, long_param, true);
    break;
  case WM_RBUTTONUP:
    arctic::OnMouse(kKeyMouseRight, word_param, long_param, false);
    break;
  case WM_RBUTTONDOWN:
    arctic::OnMouse(kKeyMouseRight, word_param, long_param, true);
    break;
  case WM_MBUTTONUP:
    arctic::OnMouse(kKeyMouseWheel, word_param, long_param, false);
    break;
  case WM_MBUTTONDOWN:
    arctic::OnMouse(kKeyMouseWheel, word_param, long_param, true);
    break;
  case WM_MOUSEMOVE:
    arctic::OnMouse(kKeyCount, word_param, long_param, false);
    break;
  case WM_MOUSEWHEEL:
    arctic::OnMouseWheel(word_param, long_param);
    break;
  default:
    return DefWindowProc(inner_window_handle, message,
      word_param, long_param);
  }
  return 0;
}

HBRUSH g_black_brush;

template<class T>
void LoadGlFunction(const char* name, T *out_ptr) {
  *out_ptr = reinterpret_cast<T>(wglGetProcAddress(name));
  Check(*out_ptr != nullptr, "Error loading function: ", name);
}

void LoadGl() {
  LoadGlFunction("glActiveTexture", &glActiveTexture);
  LoadGlFunction("glAttachShader", &glAttachShader);
  LoadGlFunction("glBindAttribLocation", &glBindAttribLocation);
  LoadGlFunction("glCompileShader", &glCompileShader);
  LoadGlFunction("glCreateProgram", &glCreateProgram);
  LoadGlFunction("glCreateShader", &glCreateShader);
  LoadGlFunction("glDeleteProgram", &glDeleteProgram);
  LoadGlFunction("glDeleteShader", &glDeleteShader);
  LoadGlFunction("glEnableVertexAttribArray", &glEnableVertexAttribArray);
  LoadGlFunction("glGetProgramiv", &glGetProgramiv);
  LoadGlFunction("glGetProgramInfoLog", &glGetProgramInfoLog);
  LoadGlFunction("glGetShaderiv", &glGetShaderiv);
  LoadGlFunction("glGetShaderInfoLog", &glGetShaderInfoLog);
  LoadGlFunction("glGetUniformLocation", &glGetUniformLocation);
  LoadGlFunction("glLinkProgram", &glLinkProgram);
  LoadGlFunction("glShaderSource", &glShaderSource);
  LoadGlFunction("glUseProgram", &glUseProgram);
  LoadGlFunction("glUniform1i", &glUniform1i);
  LoadGlFunction("glUniform2i", &glUniform2i);
  LoadGlFunction("glUniform3i", &glUniform3i);
  LoadGlFunction("glUniform4i", &glUniform4i);
  LoadGlFunction("glUniform1f", &glUniform1f);
  LoadGlFunction("glUniform2f", &glUniform2f);
  LoadGlFunction("glUniform3f", &glUniform3f);
  LoadGlFunction("glUniform4f", &glUniform4f);
  LoadGlFunction("glVertexAttribPointer", &glVertexAttribPointer);
  LoadGlFunction("glGenFramebuffers", &glGenFramebuffers);
  LoadGlFunction("glDeleteFramebuffers", &glDeleteFramebuffers);
  LoadGlFunction("glBindFramebuffer", &glBindFramebuffer);
  LoadGlFunction("glFramebufferTexture2D", &glFramebufferTexture2D);
  LoadGlFunction("glCheckFramebufferStatus", &glCheckFramebufferStatus);
  LoadGlFunction("glBlendFuncSeparate", &glBlendFuncSeparate);
  LoadGlFunction("glGenBuffers", &glGenBuffers);
  LoadGlFunction("glDeleteBuffers", &glDeleteBuffers);
  LoadGlFunction("glBindBuffer", &glBindBuffer);
  LoadGlFunction("glBufferData", &glBufferData);
  LoadGlFunction("glBufferSubData", &glBufferSubData);
}

void CreateMainWindow(SystemInfo *system_info) {
  char title_bar_text[] = {"Arctic Engine"};
  char window_class_name[] = {"ArcticEngineWindowClass"};
  char inner_window_class_name[] = {"ArcticEngineInnterWindowClass"};

  Si32 screen_width = GetSystemMetrics(SM_CXSCREEN);
  Si32 screen_height = GetSystemMetrics(SM_CYSCREEN);

  /*{
    DEVMODE dmScreenSettings;                   // Device Mode
    memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
  // Makes Sure Memory's Cleared
  dmScreenSettings.dmSize = sizeof(dmScreenSettings);
  // Size Of The Devmode Structure
  dmScreenSettings.dmPelsWidth = screen_width;
  // Selected Screen Width
  dmScreenSettings.dmPelsHeight = screen_height;
  // Selected Screen Height
  dmScreenSettings.dmBitsPerPel = 32;
  // Selected Bits Per Pixel
  dmScreenSettings.dmFields =
  DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
  // Try To Set Selected Mode And Get Results.
  // NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
  if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN)
  != DISP_CHANGE_SUCCESSFUL) {
  // If The Mode Fails, Offer Two Options.  Quit Or Run In A Window.
  MessageBox(NULL, "The requested fullscreen mode is not" \
  " supported by\nthe video card. Setting windowed mode.",
  "Arctic Engine", MB_OK | MB_ICONEXCLAMATION);
  }
  }*/

  g_black_brush = CreateSolidBrush(0);

  WNDCLASSEX wcex;
  memset(&wcex, 0, sizeof(wcex));
  wcex.cbSize = sizeof(wcex);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = arctic::WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = system_info->instance_handle;
  wcex.hIcon = LoadIcon(system_info->instance_handle, MAKEINTRESOURCE(IDI_ICON1));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = g_black_brush;
  wcex.lpszClassName = window_class_name;
  // wcex.hIconSm = LoadIcon(wcex.hInstance,
  //      MAKEINTRESOURCE(IDI_SMALL_APP_ICON));

  ATOM register_class_result = RegisterClassEx(&wcex);

  g_window_width = screen_width;
  g_window_height = screen_height;

  HWND window_handle = CreateWindowExA(WS_EX_APPWINDOW,
    window_class_name, title_bar_text,
    WS_OVERLAPPEDWINDOW,
    0, 0, screen_width, screen_height, nullptr, nullptr,
    system_info->instance_handle, nullptr);

  arctic::Check(window_handle, "Can't create the Main Window! Code: WIN07.");

  memset(&wcex, 0, sizeof(wcex));
  wcex.cbSize = sizeof(wcex);
  wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wcex.lpfnWndProc = arctic::InnerWndProc;
  wcex.hInstance = system_info->instance_handle;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = g_black_brush;
  wcex.lpszClassName = inner_window_class_name;

  ATOM register_inner_class_result = RegisterClassEx(&wcex);

  RECT client_rect;
  GetClientRect(window_handle, &client_rect);
  HWND inner_window_handle = CreateWindowExA(0,
    inner_window_class_name, "", WS_CHILD | WS_VISIBLE, 0, 0,
    client_rect.right - client_rect.left,
    client_rect.bottom - client_rect.top,
    window_handle, 0, system_info->instance_handle, 0);

  arctic::Check(inner_window_handle, "Can't create the Main Window! Code: WIN08.");

  //  ShowWindow(window_handle, cmd_show);
  ShowWindow(window_handle, SW_MINIMIZE);
  ShowWindow(window_handle, SW_MAXIMIZE);
  UpdateWindow(window_handle);

  Check(!!system_info, "Error, system_info: nullptr in CreateMainWindow");
  system_info->window_handle = window_handle;
  system_info->inner_window_handle = inner_window_handle;
  system_info->screen_width = screen_width;
  system_info->screen_height = screen_height;
}

void PrepareForTheEasyMainCall();

void EngineThreadFunction(SystemInfo system_info) {
  //  Init opengl start

  HDC hdc = GetDC(system_info.inner_window_handle);
  Check(hdc != nullptr, "Can't get the Device Context. Code: WIN01.");

  unsigned int pixel_format = ChoosePixelFormat(hdc, &pfd);
  Check(pixel_format != 0, "Can't choose the Pixel Format. Code: WIN02.");

  BOOL is_ok = SetPixelFormat(hdc, pixel_format, &pfd);
  Check(!!is_ok, "Can't set the Pixel Format. Code: WIN03.");

  HGLRC hrc = wglCreateContext(hdc);
  Check(hrc != nullptr, "Can't create the GL Context. Code: WIN04.");

  is_ok = wglMakeCurrent(hdc, hrc);
  Check(!!is_ok, "Can't make the GL Context current. Code: WIN05.");

  LoadGl();

  arctic::GetEngine()->Init(system_info.screen_width,
    system_info.screen_height);
  //  Init opengl end

  arctic::PrepareForTheEasyMainCall();
  EasyMain();

  ExitProcess(0);  //-V2014
}

void ExitProgram(Si32 exit_code) {
  ExitProcess(exit_code);  //-V2014
}

void Swap() {
  HDC hdc = wglGetCurrentDC();
  BOOL res = SwapBuffers(hdc);
  CheckWithLastError(res != FALSE, "SwapBuffers error in Swap.");

  RECT client_rect;
  GetClientRect(g_system_info.window_handle, &client_rect);

  Si32 wid = client_rect.right - client_rect.left;
  Si32 hei = client_rect.bottom - client_rect.top;
  if (g_window_width != wid || g_window_height != hei) {
    SetWindowPos(g_system_info.inner_window_handle, 0, 0, 0,
      wid, hei, SWP_NOZORDER);
    RECT rect;
    res = GetClientRect(g_system_info.inner_window_handle, &rect);
    g_window_width = wid;
    g_window_height = hei;
    arctic::GetEngine()->OnWindowResize(rect.right, rect.bottom);
  }
}

bool IsVSyncSupported() {
  const char* (WINAPI *wglGetExtensionsStringEXT)();
  wglGetExtensionsStringEXT = reinterpret_cast<const char* (WINAPI*)()>(  // NOLINT
    wglGetProcAddress("wglGetExtensionsStringEXT"));
  if (wglGetExtensionsStringEXT == nullptr) {
    return false;
  }
  // CheckWithLastError(wglGetExtensionsStringEXT != nullptr,
  // "Error in wglGetProcAddress(\"wglGetExtensionsStringEXT\"): ");
  const char *extensions = wglGetExtensionsStringEXT();
  if (strstr(extensions, "WGL_EXT_swap_control") == nullptr) {
    return false;
  }
  return true;
}

bool SetVSync(bool is_enable) {
  if (!IsVSyncSupported()) {
    return false;
  }
  bool (APIENTRY *wglSwapIntervalEXT)(int);
  wglSwapIntervalEXT = reinterpret_cast<bool (APIENTRY *)(int)>(  // NOLINT
    wglGetProcAddress("wglSwapIntervalEXT"));
  CheckWithLastError(wglSwapIntervalEXT != nullptr,
    "Error in wglGetProcAddress(\"wglSwapIntervalEXT\"): ");
  bool is_ok = wglSwapIntervalEXT(is_enable ? 1 : 0);
  CheckWithLastError(is_ok, "Error in SetVSync: ");
  return is_ok;
}

Trivalent DoesDirectoryExist(const char *path) {
  struct stat info;
  if (stat(path, &info) != 0) {
    return kTrivalentFalse;
  } else if (info.st_mode & S_IFDIR) {
    return kTrivalentTrue;
  } else {
    return kTrivalentUnknown;
  }
}

bool MakeDirectory(const char *path) {
  BOOL is_ok = CreateDirectory(path, NULL);
  return is_ok;
}

bool GetCurrentPath(std::string *out_dir) {
  char cwd[8 << 10];
  DWORD res = GetCurrentDirectoryA(sizeof(cwd), cwd);
  if (res > 0) {
    out_dir->assign(cwd);
    return true;
  }
  return false;
}

bool GetDirectoryEntries(const char *path,
     std::vector<DirectoryEntry> *out_entries) {
  Check(out_entries,
        "GetDirectoryEntries Error. Unexpected nullptr in out_entries!");
  out_entries->clear();

  std::string canonic_path = CanonicalizePath(path);
  if (canonic_path.size() == 0) {
    *Log() << "GetDirectoryEntries can't canonize path: \"" << path << "\""
      << std::endl;
    return false;
  }
  std::stringstream search_pattern;
  search_pattern << canonic_path << "\\*";

  WIN32_FIND_DATA find_data;
  HANDLE find_handle = FindFirstFile(search_pattern.str().c_str(), &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    *Log() << "GetDirectoryEntires error in FindFirstFile, path: \""
      << path << "\", canonic path: \"" << canonic_path << "\"" << std::endl;
    return false;
  }

  while (true) {
    DirectoryEntry entry;
    entry.title = find_data.cFileName;
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      entry.is_directory = kTrivalentTrue;
    } else {
      entry.is_file = kTrivalentTrue;
    }
    out_entries->push_back(entry);
    if (FindNextFile(find_handle, &find_data) == 0) {
      break;
    }
  }

  DWORD last_error = GetLastError();
  if (last_error != ERROR_NO_MORE_FILES) {
    FatalWithLastError("GetDirectoryEntries error in FindNextFile");
  }
  FindClose(find_handle);
  return true;
}

void SlashesToBackslashes(std::string *in_out_str) {
  for (std::size_t idx = 0; idx < in_out_str->size(); ++idx) {
    if ((*in_out_str)[idx] == '/') {
      (*in_out_str)[idx] = '\\';
    }
  }
}

std::string CanonicalizePath(const char *path) {
  Check(path, "CanonicalizePath error, path can't be nullptr");
  std::string path_string(path);
  SlashesToBackslashes(&path_string);
  char canonic_path[MAX_PATH];

  DWORD res = GetFullPathName(path_string.c_str(),
    MAX_PATH, canonic_path, NULL);

  BOOL is_ok = (res > 0 && res <= MAX_PATH);
  std::string result;
  if (is_ok) {
    result.assign(canonic_path);
  }
  return result;
}

std::string RelativePathFromTo(const char *from, const char *to) {
  std::string from_abs = CanonicalizePath(from);
  std::string to_abs = CanonicalizePath(to);
  char relative_path[MAX_PATH];
  BOOL is_ok = PathRelativePathTo(relative_path,
    from_abs.c_str(), FILE_ATTRIBUTE_DIRECTORY,
    to_abs.c_str(), FILE_ATTRIBUTE_DIRECTORY);
  std::string result;
  if (is_ok) {
    if (relative_path[0] == '.' && relative_path[1] == '\\') {
      result.assign(relative_path + 2);
    } else {
      result.assign(relative_path);
    }
  }
  return result;
}

std::string PrepareInitialPath() {
  std::string initial_path;
  arctic::GetCurrentPath(&initial_path);
  return initial_path;
}

}  // namespace arctic

int APIENTRY wWinMain(_In_ HINSTANCE instance_handle,
  _In_opt_ HINSTANCE prev_instance_handle,
  _In_ LPWSTR command_line,
  _In_ int cmd_show) {
  UNREFERENCED_PARAMETER(prev_instance_handle);
  UNREFERENCED_PARAMETER(cmd_show);

  // save the instance handle
  arctic::g_system_info.instance_handle = instance_handle;

  // use the real resolution on high-resolution displays
  BOOL is_ok_w = SetProcessDPIAware();
  arctic::Check(is_ok_w != FALSE,
    "Error from SetProessDPIAware! Code: WIN06.");

  // afaik disabling ghosting makes the app faster
  DisableProcessWindowsGhosting();

  // remove quotes from command line
  if (command_line) {
    if (command_line[0] == L'"') {
      arctic::Si32 i;
      for (i = 0; command_line[i]; i++) {
      }
      command_line[i - 1] = 0;
      command_line++;
    }
  }
  int num_args = 0;
  wchar_t **args = CommandLineToArgvW(GetCommandLineW(), &num_args);


  // not-so OS-specific stuff
  std::string initial_path = arctic::PrepareInitialPath();
  arctic::StartLogger();
  arctic::SoundPlayer soundPlayer;
  soundPlayer.Initialize();
  arctic::CreateMainWindow(instance_handle, &arctic::g_system_info);
  arctic::GetEngine()->SetArgcArgvW(num_args,
    const_cast<const wchar_t **>(args));

  arctic::GetEngine()->SetInitialPath(initial_path);

  std::thread engine_thread(arctic::EngineThreadFunction,
    arctic::g_system_info);
  engine_thread.detach();
  while (true) {
    MSG msg;
    BOOL ret = GetMessage(&msg, NULL, 0, 0);
    if (ret == 0) {
      break;
    } else if (ret == -1) {
      // handle the error and possibly exit
      break;
    } else {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  soundPlayer.Deinitialize();
  arctic::StopLogger();
  ExitProcess(0);  //-V2014
  return 0;
}

#endif  // ARCTIC_PLATFORM_WINDOWS
