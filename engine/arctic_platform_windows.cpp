// The MIT License(MIT)
//
// Copyright 2017 Huldra
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

#include "engine/arctic_platform_def.h"

#ifdef ARCTIC_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <thread>  // NOLINT
#include <vector>

#include "engine/engine.h"
#include "engine/easy.h"
#include "engine/arctic_input.h"
#include "engine/arctic_platform.h"
#include "engine/byte_array.h"
#include "engine/rgb.h"
#include "engine/vec3f.h"

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glu32.lib")

#define MAX_LOADSTRING 255

extern void EasyMain();

namespace arctic {

inline void Check(bool condition, const char *error_message) {
    if (condition) {
        return;
    }
    Fatal(error_message);
}

void Fatal(const char *message) {
    MessageBox(NULL, message, "Arctic Engine", MB_OK | MB_ICONERROR);
    ExitProcess(1);
}

static void FatalWithLastError(const char* message_prefix) {
    DWORD dw = GetLastError();
    char *message_postfix = "";
    char *message = "";
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&message_postfix,
        0, NULL);

    size_t size = strlen(message_prefix) + strlen(message_postfix) + 1;
    message = static_cast<char *>(LocalAlloc(LMEM_ZEROINIT, size));
    sprintf_s(message, size, "%s%s", message_prefix, message_postfix);
    Fatal(message);
}

static void CheckWithLastError(bool condition, const char *message_prefix) {
    if (condition) {
        return;
    }
    FatalWithLastError(message_prefix);
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
    HWND window_handle;
    Si32 screen_width;
    Si32 screen_height;
};

KeyCode TranslateKeyCode(WPARAM word_param) {
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

static Si32 window_width = 0;
static Si32 window_height = 0;

void OnMouse(KeyCode key, WPARAM word_param, LPARAM long_param, bool is_down) {
    Check(window_width != 0, "Could not obtain window width in OnMouse");
    Check(window_height != 0, "Could not obtain window height in OnMouse");
    Si32 x = GET_X_LPARAM(long_param);
    Si32 y = window_height - GET_Y_LPARAM(long_param);
    Vec2F pos(static_cast<float>(x) / static_cast<float>(window_width - 1),
        static_cast<float>(y) / static_cast<float>(window_height - 1));
    InputMessage msg;
    msg.kind = InputMessage::kMouse;
    msg.keyboard.key = key;
    msg.keyboard.key_state = (is_down ? 1 : 2);
    msg.mouse.pos = pos;
    PushInputMessage(msg);
}

void OnKey(WPARAM word_param, LPARAM long_param, bool is_down) {
    KeyCode key = TranslateKeyCode(word_param);
    InputMessage msg;
    msg.kind = InputMessage::kKeyboard;
    msg.keyboard.key = key;
    msg.keyboard.key_state = (is_down ? 1 : 2);
    PushInputMessage(msg);
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
        break;
    }
    case WM_KEYUP:
        arctic::OnKey(word_param, long_param, false);
        break;
    case WM_KEYDOWN:
        arctic::OnKey(word_param, long_param, true);
        break;
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
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(window_handle, message, word_param, long_param);
    }
    return 0;
}

//
// Creates main window.
//
bool CreateMainWindow(HINSTANCE instance_handle, int cmd_show,
        SystemInfo *system_info) {
    // WCHAR title_bar_text[MAX_LOADSTRING];
    // WCHAR window_class_name[MAX_LOADSTRING];

    // LoadStringW(instance_handle, IDS_APP_TITLE,
    //     title_bar_text, MAX_LOADSTRING);
    // LoadStringW(instance_handle, IDC_DEMO,
    //     window_class_name, MAX_LOADSTRING);

    WCHAR title_bar_text[] = {L"Arctic Engine"};
    WCHAR window_class_name[] = {L"ArcticEngineWindowClass"};

    Si32 screen_width = GetSystemMetrics(SM_CXSCREEN);
    Si32 screen_height = GetSystemMetrics(SM_CYSCREEN);

    {
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
    }

    WNDCLASSEXW wcex;
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc = arctic::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance_handle;
    //  wcex.hIcon = LoadIcon(instance_handle, MAKEINTRESOURCE(IDI_APP_ICON));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    //  wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DEMO);
    wcex.lpszClassName = window_class_name;
    // wcex.hIconSm = LoadIcon(wcex.hInstance,
    //      MAKEINTRESOURCE(IDI_SMALL_APP_ICON));

    ATOM register_class_result = RegisterClassExW(&wcex);

    window_width = screen_width;
    window_height = screen_height;

    HWND window_handle = CreateWindowExW(WS_EX_APPWINDOW,
        window_class_name, title_bar_text,
        WS_POPUP /*| WS_CLIPSIBLINGS | WS_CLIPCHILDREN/*WS_OVERLAPPEDWINDOW*/,
        0, 0, screen_width, screen_height, nullptr, nullptr,
        instance_handle, nullptr);
    if (!window_handle) {
        return false;
    }

    ShowWindow(window_handle, cmd_show);
    UpdateWindow(window_handle);

    Check(!!system_info, "Error, system_info: nullptr in CreateMainWindow");
    system_info->window_handle = window_handle;
    system_info->screen_width = screen_width;
    system_info->screen_height = screen_height;
    return true;
}

void EngineThreadFunction(SystemInfo system_info) {
    //  Init opengl start

    HDC hdc = GetDC(system_info.window_handle);
    Check(hdc != nullptr, "Can't get the Device Context. Code: WIN01.");

    unsigned int pixel_format = ChoosePixelFormat(hdc, &pfd);
    Check(pixel_format != 0, "Can't choose the Pixel Format. Code: WIN02.");

    BOOL is_ok = SetPixelFormat(hdc, pixel_format, &pfd);
    Check(!!is_ok, "Can't set the Pixel Format. Code: WIN03.");

    HGLRC hrc = wglCreateContext(hdc);
    Check(hrc != nullptr, "Can't create the GL Context. Code: WIN04.");

    is_ok = wglMakeCurrent(hdc, hrc);
    Check(!!is_ok, "Can't make the GL Context current. Code: WIN05.");

    arctic::easy::GetEngine()->Init(system_info.screen_width,
        system_info.screen_height);
    //  Init opengl end

    EasyMain();

    ExitProcess(0);
}


void Swap() {
    HDC hdc = wglGetCurrentDC();
    SwapBuffers(hdc);
}

bool IsVSyncSupported() {
    const char* (WINAPI *wglGetExtensionsStringEXT)();
    wglGetExtensionsStringEXT = reinterpret_cast<const char* (WINAPI*)()>(  // NOLINT
        wglGetProcAddress("wglGetExtensionsStringEXT"));
    CheckWithLastError(wglGetExtensionsStringEXT != nullptr,
        "Error in wglGetProcAddress(\"wglGetExtensionsStringEXT\"): ");
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

void ProcessUserInput() {
    while (true) {
        MSG msg;
        BOOL ret = GetMessage(&msg, NULL, 0, 0);
        if (ret == 0) {
            return;
        } else if (ret == -1) {
            // handle the error and possibly exit
            return;
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

std::vector<Ui8> ReadWholeFile(const char *file_name) {
    HANDLE file_handle = CreateFile(file_name, GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    CheckWithLastError(file_handle != INVALID_HANDLE_VALUE,
        "Error in ReadWholeFile. CreateFile: ");
    LARGE_INTEGER file_size;
    file_size.QuadPart = 0ull;
    BOOL is_ok = GetFileSizeEx(file_handle, &file_size);
    CheckWithLastError(!!is_ok, "Error in ReadWholeFile. GetFileSizeEx: ");
    std::vector<Ui8> data;
    if (file_size.QuadPart != 0ull) {
        Check(file_size.HighPart == 0,
            "Error in ReadWholeFile, file is too large.");
        data.resize(static_cast<size_t>(file_size.QuadPart));
        DWORD bytes_read = 0ul;
        is_ok = ReadFile(file_handle, data.data(), file_size.LowPart,
            &bytes_read, NULL);
        CheckWithLastError(!!is_ok, "Error in ReadWholeFile. ReadFile: ");
        Check(bytes_read == file_size.LowPart,
            "Error in ReadWholeFile. Read size mismatch.");
    }
    is_ok = CloseHandle(file_handle);
    CheckWithLastError(!!is_ok, "Error in ReadWholeFile. CloseHandle: ");
    return data;
}

void WriteWholeFile(const char *file_name, const Ui8 *data,
        const Ui64 data_size) {
    // TODO(Huldra): Implement this
}



}  // namespace arctic


int APIENTRY wWinMain(_In_ HINSTANCE instance_handle,
        _In_opt_ HINSTANCE prev_instance_handle,
        _In_ LPWSTR command_line,
        _In_ int cmd_show) {
    UNREFERENCED_PARAMETER(prev_instance_handle);
    UNREFERENCED_PARAMETER(command_line);

    BOOL is_ok_w = SetProcessDPIAware();
    arctic::Check(is_ok_w != FALSE,
        "Error from SetProessDPIAware! Code: WIN06.");

    DisableProcessWindowsGhosting();

    arctic::SystemInfo system_info;
    bool is_ok = arctic::CreateMainWindow(instance_handle, cmd_show,
        &system_info);
    arctic::Check(is_ok, "Can't create the Main Window! Code: WIN07.");

    arctic::easy::GetEngine();

    std::thread engine_thread(arctic::EngineThreadFunction, system_info);
    arctic::ProcessUserInput();
    ExitProcess(0);
//    engine_thread.join();
    return 0;
}


#endif  // ARCTIC_PLATFORM_WINDOWS
