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
#include <GL/gl.h>
#include <GL/glu.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <memory>

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

void Check(bool condition, const char *error_message) {
    if (condition) {
        return;
    }
    Fatal(error_message);
}

void Fatal(const char *message) {
    MessageBox(NULL, message, "Arctic Engine", MB_OK | MB_ICONERROR);
    exit(1);
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

Si32 g_width;
Si32 g_height;
Ui32 g_texture_name;
ByteArray g_texture_data;
std::mt19937 g_twister;

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
    }
    case WM_KEYUP:
        arctic::OnKey(word_param, long_param, false);
        break;
    case WM_KEYDOWN:
        arctic::OnKey(word_param, long_param, true);
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
bool CreateMainWindow(HINSTANCE instance_handle, int cmd_show) {
    // WCHAR title_bar_text[MAX_LOADSTRING];
    // WCHAR window_class_name[MAX_LOADSTRING];

    // LoadStringW(instance_handle, IDS_APP_TITLE,
    //     title_bar_text, MAX_LOADSTRING);
    // LoadStringW(instance_handle, IDC_DEMO,
    //     window_class_name, MAX_LOADSTRING);

    WCHAR title_bar_text[] = {L"Arctic Engine"};
    WCHAR window_class_name[] = {L"ArcticEngineWindowClass"};

    g_width = GetSystemMetrics(SM_CXSCREEN);
    g_height = GetSystemMetrics(SM_CYSCREEN);

    {
        DEVMODE dmScreenSettings;                   // Device Mode
        memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
        // Makes Sure Memory's Cleared
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        // Size Of The Devmode Structure
        dmScreenSettings.dmPelsWidth = g_width;
        // Selected Screen Width
        dmScreenSettings.dmPelsHeight = g_height;
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
    wcex.style = CS_HREDRAW | CS_VREDRAW;
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

    HWND window_handle = CreateWindowExW(WS_EX_APPWINDOW,
        window_class_name, title_bar_text,
        WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN/*WS_OVERLAPPEDWINDOW*/,
        0, 0, g_width, g_height, nullptr, nullptr,
        instance_handle, nullptr);
    if (!window_handle) {
        return false;
    }

    //  Init opengl start

    HDC hdc = GetDC(window_handle);
    if (!hdc) {
        Fatal("Can't get the Device Context. Code: WIN01.");
        return false;
    }
    unsigned int pixel_format = ChoosePixelFormat(hdc, &pfd);
    if (!pixel_format) {
        Fatal("Can't choose the Pixel Format. Code: WIN02.");
        return false;
    }

    BOOL is_ok = SetPixelFormat(hdc, pixel_format, &pfd);
    if (!is_ok) {
        Fatal("Can't set the Pixel Format. Code: WIN03.");
        return false;
    }

    HGLRC hrc = wglCreateContext(hdc);

    if (!hrc) {
        Fatal("Can't create the GL Context. Code: WIN04.");
        return false;
    }

    is_ok = wglMakeCurrent(hdc, hrc);
    if (!is_ok) {
        Fatal("Can't make the GL Context current. Code: WIN05.");
        return false;
    }

    g_texture_data.Resize(g_width * g_height * 4);

    glEnable(GL_TEXTURE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGenTextures(1, &g_texture_name);
    // generate a texture handler really reccomanded (mandatory in openGL 3.0)
    glBindTexture(GL_TEXTURE_2D, g_texture_name);
    // tell openGL that we are using the texture

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, g_width, g_height, 0, GL_RGB,
        GL_UNSIGNED_BYTE, g_texture_data.GetVoidData());
    // send the texture data

    //  Init opengl end

    ShowWindow(window_handle, cmd_show);
    UpdateWindow(window_handle);
    return true;
}

void Swap() {
    HDC hdc = wglGetCurrentDC();
    wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE);
}

ByteArray gVisibleVerts;
ByteArray gVisibleNormals;
ByteArray gTexCoords;
ByteArray gVisibleIndices;

Si32 gVerts = 0;
Si32 gNormals = 0;
Si32 gTex = 0;
Si32 gIndices = 0;

void Draw2d(Si32 g_width, Si32 g_height, Ui8 *data) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_width, g_height, GL_RGB,
        GL_UNSIGNED_BYTE, static_cast<GLvoid*>(data));

    // render
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_texture_name);

    // draw quad

    gVisibleVerts.Resize(16 << 20);
    gVisibleNormals.Resize(16 << 20);
    gVisibleIndices.Resize(16 << 20);
    gTexCoords.Resize(16 << 20);

    gVerts = 0;
    gNormals = 0;
    gTex = 0;
    gIndices = 0;

    Vec3F *vertex = static_cast<Vec3F*>(gVisibleVerts.GetVoidData());
    Vec3F *normal = static_cast<Vec3F*>(gVisibleNormals.GetVoidData());
    Vec2F *tex = static_cast<Vec2F*>(gTexCoords.GetVoidData());
    Ui32 *index = static_cast<Ui32*>(gVisibleIndices.GetVoidData());

    Vec3F base = Vec3F(-1.f, -1.f, 0.f);
    Vec3F tx = Vec3F(2.f, 0.f, 0.f);
    Vec3F ty = Vec3F(0.f, 2.f, 0.f);
    Vec3F n = Vec3F(0.f, 0.f, 1.f);

    Si32 idx = gVerts;
    vertex[gVerts] = base;
    ++gVerts;
    vertex[gVerts] = base + tx;
    ++gVerts;
    vertex[gVerts] = base + ty + tx;
    ++gVerts;
    vertex[gVerts] = base + ty;
    ++gVerts;

    normal[gNormals] = n;
    ++gNormals;
    normal[gNormals] = n;
    ++gNormals;
    normal[gNormals] = n;
    ++gNormals;
    normal[gNormals] = n;
    ++gNormals;

    tex[gTex] = Vec2F(0.0f, 0.0f);
    ++gTex;
    tex[gTex] = Vec2F(1.0f, 0.0f);
    ++gTex;
    tex[gTex] = Vec2F(1.0f, 1.0f);
    ++gTex;
    tex[gTex] = Vec2F(0.0f, 1.0f);
    ++gTex;

    index[gIndices] = idx;
    gIndices++;
    index[gIndices] = idx + 1;
    gIndices++;
    index[gIndices] = idx + 2;
    gIndices++;
    index[gIndices] = idx + 2;
    gIndices++;
    index[gIndices] = idx + 3;
    gIndices++;
    index[gIndices] = idx;
    gIndices++;

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, gVisibleVerts.GetVoidData());
    glNormalPointer(GL_FLOAT, 0, gVisibleNormals.GetVoidData());
    glTexCoordPointer(2, GL_FLOAT, 0, gTexCoords.GetVoidData());
    glDrawElements(GL_TRIANGLES, gIndices, GL_UNSIGNED_INT,
        gVisibleIndices.GetVoidData());

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    Swap();
}

void Draw() {
    Rgb* img = static_cast<Rgb*>(g_texture_data.GetVoidData());

    for (Si32 y = 0; y < g_height; ++y) {
        Ui8 rnd = g_twister() % 256;
        Si32 wid = g_twister() % 16;
        for (Si32 xi = 0; xi < wid; ++xi) {
            img->r = rnd;
            img->g = rnd;
            img->b = rnd;
            ++img;
        }
        for (Si32 x = wid; x < g_width; x += 16) {
            rnd = g_twister() % 256;
            wid = std::min(g_width, x + 16);
            for (Si32 xi = x; xi < wid; ++xi) {
                img->r = rnd;
                img->g = rnd;
                img->b = rnd;
                ++img;
            }
        }
    }

    Draw2d(g_width, g_height, g_texture_data.data());
}

void ProcessUserInput() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (msg.message == WM_QUIT) {
        exit(0);
    }
}

}  // namespace arctic

int APIENTRY wWinMain(_In_ HINSTANCE instance_handle,
    _In_opt_ HINSTANCE prev_instance_handle,
    _In_ LPWSTR command_line,
    _In_ int cmd_show) {
    UNREFERENCED_PARAMETER(prev_instance_handle);
    UNREFERENCED_PARAMETER(command_line);

    BOOL is_ok = SetProcessDPIAware();
    arctic::Check(is_ok != FALSE, "Error from SetProessDPIAware! Code: WIN06.");

    if (!arctic::CreateMainWindow(instance_handle, cmd_show)) {
        arctic::Fatal("Can't create the Main Window! Code: WIN07.");
        return FALSE;
    }

    arctic::ProcessUserInput();
    EasyMain();
    return 0;
}


#endif  // ARCTIC_PLATFORM_WINDOWS
