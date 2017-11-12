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

#ifdef ARCTIC_PLATFORM_PI

#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <string.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xatom.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>  // NOLINT
#include <thread>  // NOLINT
#include <vector>

#include "engine/engine.h"
#include "engine/easy.h"
#include "engine/arctic_input.h"
#include "engine/arctic_platform.h"
#include "engine/byte_array.h"
#include "engine/rgb.h"
#include "engine/vec3f.h"


extern void EasyMain();

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

void Fatal(const char *message, const char *message_postfix) {
    size_t size = 1 +
        strlen(message) +
        (message_postfix ? strlen(message_postfix) : 0);
    char *full_message = static_cast<char *>(malloc(size));
    memset(full_message, 0, size);
    snprintf(full_message, size, "%s%s", message,
        (message_postfix ? message_postfix : ""));
    std::cerr << "Arctic Engine ERROR: " << full_message << std::endl;
    exit(1);
}

void Check(bool condition, const char *error_message,
        const char *error_message_postfix) {
    if (condition) {
        return;
    }
    Fatal(error_message, error_message_postfix);
}

static int glx_config[] = {
    GLX_DOUBLEBUFFER,
    GLX_RGBA,
    GLX_DEPTH_SIZE,
    16,
    None
};

struct SystemInfo {
    Si32 screen_width;
    Si32 screen_height;
};

static Si32 window_width = 0;
static Si32 window_height = 0;
static Si32 last_mouse_x = 0;
static Si32 last_mouse_y = 0;
static Display *x_display;
static Window x_window;
static Colormap x_color_map;
static XVisualInfo *glx_visual;
static const int kXEventMask = KeyPressMask | KeyReleaseMask | ButtonPressMask
        | ButtonReleaseMask | PointerMotionMask | ExposureMask
        | StructureNotifyMask;
static GLXContext glx_context;

KeyCode TranslateKeyCode(KeySym ks) {
    if (ks >= XK_a && ks <= XK_z) {
        ks = ks + XK_A - XK_a;
    }
    switch (ks) {
        case XK_Left:
            return kKeyLeft;
        case XK_Right:
            return kKeyRight;
        case XK_Up:
            return kKeyUp;
        case XK_Down:
            return kKeyDown;
        case XK_BackSpace:
            return kKeyBackspace;
        case XK_Tab:
            return kKeyTab;

        case XK_Return:
            return kKeyEnter;
        case XK_Home:
            return kKeyHome;
        case XK_End:
            return kKeyEnd;
        case XK_Page_Up:
            return kKeyPageUp;
        case XK_Page_Down:
            return kKeyPageDown;

        case XK_Shift_L:
            return kKeyLeftShift;
        case XK_Shift_R:
            return kKeyRightShift;
        case XK_Control_L:
            return kKeyLeftControl;
        case XK_Control_R:
            return kKeyRightControl;
        case XK_Alt_L:
            return kKeyLeftAlt;
        case XK_Alt_R:
            return kKeyRightAlt;
        case XK_Escape:
            return kKeyEscape;

        case XK_space:
            return kKeySpace;

        case XK_apostrophe:
            return kKeyApostrophe;

        case XK_comma:
            return kKeyComma;
        case XK_minus:
            return kKeyMinus;
        case XK_period:
            return kKeyPeriod;
        case XK_slash:
            return kKeySlash;
        case XK_0:
            return kKey0;
        case XK_1:
            return kKey1;
        case XK_2:
            return kKey2;
        case XK_3:
            return kKey3;
        case XK_4:
            return kKey4;
        case XK_5:
            return kKey5;
        case XK_6:
            return kKey6;
        case XK_7:
            return kKey7;
        case XK_8:
            return kKey8;
        case XK_9:
            return kKey9;

        case XK_semicolon:
            return kKeySemicolon;
        case XK_Cancel:
            return kKeyPause;
        case XK_equal:
            return kKeyEquals;
        case XK_Num_Lock:
            return kKeyNumLock;
        case XK_Scroll_Lock:
            return kKeyScrollLock;
        case XK_Caps_Lock:
            return kKeyCapsLock;
        case XK_A:
            return kKeyA;
        case XK_B:
            return kKeyB;
        case XK_C:
            return kKeyC;
        case XK_D:
            return kKeyD;
        case XK_E:
            return kKeyE;
        case XK_F:
            return kKeyF;
        case XK_G:
            return kKeyG;
        case XK_H:
            return kKeyH;
        case XK_I:
            return kKeyI;
        case XK_J:
            return kKeyJ;
        case XK_K:
            return kKeyK;
        case XK_L:
            return kKeyL;
        case XK_M:
            return kKeyM;
        case XK_N:
            return kKeyN;
        case XK_O:
            return kKeyO;
        case XK_P:
            return kKeyP;
        case XK_Q:
            return kKeyQ;
        case XK_R:
            return kKeyR;
        case XK_S:
            return kKeyS;
        case XK_T:
            return kKeyT;
        case XK_U:
            return kKeyU;
        case XK_V:
            return kKeyV;
        case XK_W:
            return kKeyW;
        case XK_X:
            return kKeyX;
        case XK_Y:
            return kKeyY;
        case XK_Z:
            return kKeyZ;
        case XK_bracketleft:
            return kKeyLeftSquareBracket;
        case XK_backslash:
            return kKeyBackslash;
        case XK_bracketright:
            return kKeyRightSquareBracket;

        case XK_dead_grave:
            return kKeyGraveAccent;
        case XK_F1:
            return kKeyF1;
        case XK_F2:
            return kKeyF2;
        case XK_F3:
            return kKeyF3;
        case XK_F4:
            return kKeyF4;
        case XK_F5:
            return kKeyF5;
        case XK_F6:
            return kKeyF6;
        case XK_F7:
            return kKeyF7;
        case XK_F8:
            return kKeyF8;
        case XK_F9:
            return kKeyF9;
        case XK_F10:
            return kKeyF10;
        case XK_F11:
            return kKeyF11;
        case XK_F12:
            return kKeyF12;

        case XK_KP_0:
            return kKeyNumpad0;
        case XK_KP_1:
            return kKeyNumpad1;
        case XK_KP_2:
            return kKeyNumpad2;
        case XK_KP_3:
            return kKeyNumpad3;
        case XK_KP_4:
            return kKeyNumpad4;
        case XK_KP_5:
            return kKeyNumpad5;
        case XK_KP_6:
            return kKeyNumpad6;
        case XK_KP_7:
            return kKeyNumpad7;
        case XK_KP_8:
            return kKeyNumpad8;
        case XK_KP_9:
            return kKeyNumpad9;
        case XK_KP_Divide:
            return kKeyNumpadSlash;
        case XK_KP_Multiply:
            return kKeyNumpadAsterisk;
        case XK_KP_Subtract:
            return kKeyNumpadMinus;
        case XK_KP_Add:
            return kKeyNumpadPlus;
        case XK_KP_Decimal:
            return kKeyNumpadPeriod;
        case XK_Print:
            return kKeyPrintScreen;
        case XK_KP_Enter:
            return kKeyEnter;
        case XK_Insert:
            return kKeyInsert;
        case XK_Delete:
            return kKeyDelete;
        case XK_section:
            return kKeySectionSign;
    }
    return kKeyUnknown;
}


void OnMouse(KeyCode key, Si32 mouse_x, Si32 mouse_y, bool is_down) {
    Check(window_width != 0, "Could not obtain window width in OnMouse");
    Check(window_height != 0, "Could not obtain window height in OnMouse");
    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;
    Si32 x = mouse_x;
    Si32 y = window_height - mouse_y;
    Vec2F pos(static_cast<float>(x) / static_cast<float>(window_width - 1),
        static_cast<float>(y) / static_cast<float>(window_height - 1));
    InputMessage msg;
    msg.kind = InputMessage::kMouse;
    msg.keyboard.key = key;
    msg.keyboard.key_state = (is_down ? 1 : 2);
    msg.mouse.pos = pos;
    msg.mouse.wheel_delta = 0;
    PushInputMessage(msg);
}

void OnMouseWheel(bool is_down) {
    Check(window_width != 0, "Could not obtain window width in OnMouseWheel");
    Check(window_height != 0,
        "Could not obtain window height in OnMouseWheel");

    Si32 z_delta = is_down ? -1 : 1;

    Si32 x = last_mouse_x;
    Si32 y = window_height - last_mouse_y;

    Vec2F pos(static_cast<float>(x) / static_cast<float>(window_width - 1),
        static_cast<float>(y) / static_cast<float>(window_height - 1));
    InputMessage msg;
    msg.kind = InputMessage::kMouse;
    msg.keyboard.key = kKeyCount;
    msg.keyboard.key_state = false;
    msg.mouse.pos = pos;
    msg.mouse.wheel_delta = z_delta;
    PushInputMessage(msg);
}

void OnKey(KeyCode key, bool is_down) {
    InputMessage msg;
    msg.kind = InputMessage::kKeyboard;
    msg.keyboard.key = key;
    msg.keyboard.key_state = (is_down ? 1 : 2);
    PushInputMessage(msg);
}


void PumpMessages() {
    XEvent ev;
    while (True == XCheckWindowEvent(x_display, x_window,
                KeyPressMask | KeyReleaseMask, &ev)) {
        KeySym ks = XkbKeycodeToKeysym(x_display, ev.xkey.keycode, 0, 0);
        if (ks) {
            arctic::KeyCode key = TranslateKeyCode(ks);
            if (key == kKeyUnknown) {
                ::KeyCode kcode = XKeysymToKeycode(x_display, ks);
                if (kcode != 0) {
                    ks = XkbKeycodeToKeysym(x_display, kcode, 0, 0);
                    key = TranslateKeyCode(ks);
                    std::cerr << "ks: " << ks << " key: " << key << std::endl;
                }
            }
            bool is_down = (ev.type == KeyPress);
            OnKey(key, is_down);
        }
    }

    while (True == XCheckWindowEvent(x_display, x_window,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &ev)) {
        if (ButtonPress == ev.type || ButtonRelease == ev.type) {
            if (ev.xbutton.button == Button4) {
                arctic::OnMouseWheel(false);  // up
            } else if (ev.xbutton.button == Button5) {
                arctic::OnMouseWheel(true);  // down
            } else {
                arctic::KeyCode key_code = kKeyCount;
                bool is_down = false;
                if (ev.type == ButtonPress) {
                    switch (ev.xbutton.button) {
                    case Button1:
                        key_code = kKeyMouseLeft;
                        is_down = true;
                        break;
                    case Button2:
                        key_code = kKeyMouseWheel;
                        is_down = true;
                        break;
                    case Button3:
                        key_code = kKeyMouseRight;
                        is_down = true;
                        break;
                    }
                } else if (ev.type == ButtonRelease) {
                    switch (ev.xbutton.button) {
                    case Button1:
                        key_code = kKeyMouseLeft;
                        break;
                    case Button2:
                        key_code = kKeyMouseWheel;
                        break;
                    case Button3:
                        key_code = kKeyMouseRight;
                        break;
                    }
                }
                arctic::OnMouse(key_code, ev.xbutton.x, ev.xbutton.y, is_down);
            }
        } else if (ev.type == MotionNotify) {
            arctic::OnMouse(kKeyCount, ev.xbutton.x, ev.xbutton.y, false);
        }
    }

    if (True == XCheckTypedWindowEvent(
                x_display, x_window, ConfigureNotify, &ev)) {
        window_width = ev.xconfigure.width;
        window_height = ev.xconfigure.height;
    }

    if (True == XCheckTypedWindowEvent(
                x_display, x_window, DestroyNotify, &ev)) {
        exit(0);
    }

    return;
}


void CreateMainWindow(SystemInfo *system_info) {
    const char *title = "Arctic Engine";

    x_display = XOpenDisplay(NULL);
    Check(x_display != NULL, "Can't open display.");

    XWindowAttributes window_attributes;
    Status is_good = XGetWindowAttributes(x_display,
            RootWindow(x_display, DefaultScreen(x_display)),
            &window_attributes);
    Check(is_good != 0, "Can't get window attributes.");
    window_width = window_attributes.width;
    window_height = window_attributes.height;

    Bool is_ok = glXQueryExtension(x_display, NULL, NULL);
    Check(is_ok, "Can't find OpenGL via glXQueryExtension.");

    glx_visual = glXChooseVisual(x_display,
            DefaultScreen(x_display), glx_config);
    Check(glx_visual != NULL, "Can't choose visual via glXChooseVisual.");

    x_color_map = XCreateColormap(x_display,
            RootWindow(x_display, glx_visual->screen),
            glx_visual->visual,
            AllocNone);

    glx_context = glXCreateContext(
            x_display, glx_visual, None, GL_TRUE);
    Check(glx_context != NULL, "Can't create context via glXCreateContext.");

    XSetWindowAttributes swa;
    swa.colormap = x_color_map;
    swa.border_pixel = 0;
    swa.event_mask = kXEventMask;

    x_window = XCreateWindow(x_display,
            RootWindow(x_display, glx_visual->screen),
            0, 0, window_width, window_height,
            1,
            glx_visual->depth,
            InputOutput,
            glx_visual->visual,
            CWEventMask | CWBorderPixel | CWColormap, &swa);


    system_info->screen_width = window_width;
    system_info->screen_height = window_height;

    XStoreName(x_display, x_window, title);

    XWMHints wmHints;
    wmHints.flags = 0;
    wmHints.initial_state = NormalState;
    XSetWMHints(x_display, x_window, &wmHints);

    XSetIconName(x_display, x_window, title);
    XMapWindow(x_display, x_window);

    glXMakeCurrent(x_display, x_window, glx_context);

    glClearColor(1.0F, 1.0F, 1.0F, 0.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glFlush();
    return;
}

static std::mutex g_sound_mixer_mutex;
struct SoundBuffer {
    easy::Sound sound;
    float volume = 1.0f;
    Si32 next_position = 0;
};
struct SoundMixerState {
    float master_volume = 0.7f;
    std::vector<SoundBuffer> buffers;
};
SoundMixerState g_sound_mixer_state;

void StartSoundBuffer(easy::Sound sound, float volume) {
    SoundBuffer buffer;
    buffer.sound = sound;
    buffer.volume = volume;
    buffer.next_position = 0;
    buffer.sound.GetInstance()->IncPlaying();
    std::lock_guard<std::mutex> lock(g_sound_mixer_mutex);
    g_sound_mixer_state.buffers.push_back(buffer);
}

void StopSoundBuffer(easy::Sound sound) {
    std::lock_guard<std::mutex> lock(g_sound_mixer_mutex);
    for (size_t idx = 0; idx < g_sound_mixer_state.buffers.size(); ++idx) {
        SoundBuffer &buffer = g_sound_mixer_state.buffers[idx];
        if (buffer.sound.GetInstance() == sound.GetInstance()) {
            if (idx != g_sound_mixer_state.buffers.size() - 1) {
                g_sound_mixer_state.buffers[idx] =
                    g_sound_mixer_state.buffers[
                        g_sound_mixer_state.buffers.size() - 1];
            }
            g_sound_mixer_state.buffers.pop_back();
            buffer.sound.GetInstance()->DecPlaying();
            idx--;
        }
    }
}

void SetMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(g_sound_mixer_mutex);
    g_sound_mixer_state.master_volume = volume;
}

float GetMasterVolume() {
    std::lock_guard<std::mutex> lock(g_sound_mixer_mutex);
    return g_sound_mixer_state.master_volume;
}


void Swap() {
    glFlush();
    glXSwapBuffers(x_display, x_window);
    PumpMessages();
}

bool IsVSyncSupported() {
    const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
    if (strstr(extensions, "GLX_SGI_swap_control") == nullptr) {
        return false;
    }
    return true;
}

bool SetVSync(bool is_enable) {
    if (!IsVSyncSupported()) {
        return false;
    }
    PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI =
        (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddress(
                (const GLubyte*)"glXSwapIntervalSGI");
    if (glXSwapIntervalSGI != NULL) {
        glXSwapIntervalSGI(is_enable ? 1 : 0);
        return true;
    }
    return false;
}

static unsigned int g_buffer_time_us = 50000;
static unsigned int g_period_time_us = 10000;

struct async_private_data {
    std::vector<Si16> samples;
    std::vector<Si32> mix;
    std::vector<Si16> tmp;
    snd_async_handler_t *ahandler;
    snd_pcm_t *handle;
    snd_output_t *output = NULL;
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
};

static async_private_data g_data;

void MixSound() {
    async_private_data *data = &g_data;

    Si32 buffer_samples_total = data->period_size * 2;
    Si32 buffer_bytes = data->period_size * 4;

    float master_volume = 1.0f;
    {
        memset(data->mix.data(), 0, 2 * buffer_bytes);
        std::lock_guard<std::mutex> lock(g_sound_mixer_mutex);
        master_volume = g_sound_mixer_state.master_volume;
        for (Ui32 idx = 0;
                idx < g_sound_mixer_state.buffers.size(); ++idx) {
            SoundBuffer &sound = g_sound_mixer_state.buffers[idx];

            Ui32 size = data->period_size;
            size = sound.sound.StreamOut(sound.next_position, size,
                    data->tmp.data(), buffer_samples_total);
            Si16 *in_data = data->tmp.data();
            for (Ui32 i = 0; i < size; ++i) {
                data->mix[i * 2] += static_cast<Si32>(
                        static_cast<float>(in_data[i * 2]) * sound.volume);
                data->mix[i * 2 + 1] += static_cast<Si32>(
                        static_cast<float>(in_data[i * 2 + 1]) * sound.volume);
                ++sound.next_position;
            }

            if (sound.next_position == sound.sound.DurationSamples()
                    || size == 0) {
                sound.sound.GetInstance()->DecPlaying();
                g_sound_mixer_state.buffers[idx] =
                    g_sound_mixer_state.buffers[
                    g_sound_mixer_state.buffers.size() - 1];
                g_sound_mixer_state.buffers.pop_back();
                --idx;
            }
        }
    }

    unsigned char *out_buffer = (unsigned char *)data->samples.data();
    for (Ui32 i = 0; i < buffer_samples_total; ++i) {
        Si16 res = static_cast<Si16>(Clamp(
                    static_cast<float>(data->mix[i]) * master_volume,
                    -32767.0, 32767.0));
        out_buffer[i * 2 + 0] = res & 0xff;
        out_buffer[i * 2 + 1] = (res >> 8) & 0xff;
    }
}

static void SoundMixerCallback(snd_async_handler_t *ahandler) {
    snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
    async_private_data *data = static_cast<async_private_data*>(
            snd_async_handler_get_callback_private(ahandler));

    while (true) {
        snd_pcm_sframes_t avail = snd_pcm_avail_update(handle);
        if (avail < data->period_size) {
            return;
        }

        MixSound();

        unsigned char *out_buffer = (unsigned char *)data->samples.data();
        int err = snd_pcm_writei(handle, out_buffer, data->period_size);
        Check(err >= 0, "Sound write error: ", snd_strerror(err));
        Check(err == data->period_size,
                "Sound write error: written != expected.");
    }
}

void SoundMixerThreadFunction() {
    while (true) {
        MixSound();

        Si16 *out_buffer = g_data.samples.data();
        Si32 size_left = g_data.period_size;
        while (size_left > 0) {
            int err = snd_pcm_writei(g_data.handle, out_buffer, size_left);
            if (err == -EAGAIN) {
                continue;
            } else if (err == -EPIPE) {
                err = snd_pcm_prepare(g_data.handle);
                Check(err >= 0, "Can't recover sound from underrun: ",
                        snd_strerror(err));
            } else if (err == -ESTRPIPE) {
                while (true) {
                    err = snd_pcm_resume(g_data.handle);
                    if (err != -EAGAIN) {
                        break;
                    }
                    sleep(1);
                }
                if (err < 0) {
                    err = snd_pcm_prepare(g_data.handle);
                    Check(err >= 0, "Can't recover sound from suspend: ",
                            snd_strerror(err));
                }
            } else {
                Check(err >= 0, "Can't write sound data: ",
                        snd_strerror(err));
            }
            out_buffer += err * 2;
            size_left -= err;
        }
    }
}

std::thread sound_thread;

void StartSoundMixer() {
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
    int err = snd_output_stdio_attach(&g_data.output, stdout, 0);
    Check(err >= 0, "Sound error output setup failed: ", snd_strerror(err));

    err = snd_pcm_open(&g_data.handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err == -ENOENT) {
        err = snd_pcm_open(&g_data.handle, "plughw:0,0",
                SND_PCM_STREAM_PLAYBACK, 0);
        Check(err >= 0, "Can't open 'plughw:0,0' sound device: ",
                snd_strerror(err));
    } else {
        Check(err >= 0, "Can't open 'default' sound device: ",
                snd_strerror(err));
    }

    err = snd_pcm_hw_params_any(g_data.handle, hwparams);
    Check(err >= 0, "Can't get sound configuration space: ", snd_strerror(err));
    err = snd_pcm_hw_params_set_rate_resample(g_data.handle, hwparams, 1);
    Check(err >= 0, "Can't set sound resampling: ", snd_strerror(err));
    err = snd_pcm_hw_params_set_access(g_data.handle, hwparams,
            SND_PCM_ACCESS_RW_INTERLEAVED);
    Check(err >= 0, "Can't set access type for sound: ", snd_strerror(err));
    err = snd_pcm_hw_params_set_format(g_data.handle, hwparams,
            SND_PCM_FORMAT_S16);
    Check(err >= 0, "Can't set sample format for sound: ", snd_strerror(err));
    err = snd_pcm_hw_params_set_channels(g_data.handle, hwparams, 2);
    Check(err >= 0, "Can't set 2 channels for sound: ", snd_strerror(err));
    unsigned int rate = 44100;
    err = snd_pcm_hw_params_set_rate_near(g_data.handle, hwparams, &rate, 0);
    Check(err >= 0, "Can't set 44100 Hz rate for sound: ", snd_strerror(err));
    Check(rate == 44100, "Sound output rate doesn't match requested 44100 Hz.");
    int dir;
    err = snd_pcm_hw_params_set_buffer_time_near(g_data.handle, hwparams,
            &g_buffer_time_us, &dir);
    Check(err >= 0, "Can't set buffer time for sound: ", snd_strerror(err));
    snd_pcm_uframes_t size;
    err = snd_pcm_hw_params_get_buffer_size(hwparams, &size);
    Check(err >= 0, "Can't get buffer size for sound: ", snd_strerror(err));
    g_data.buffer_size = size;
    err = snd_pcm_hw_params_set_period_time_near(g_data.handle, hwparams,
            &g_period_time_us, &dir);
    Check(err >= 0, "Can't set period time for sound: ", snd_strerror(err));
    err = snd_pcm_hw_params_get_period_size(hwparams, &size, &dir);
    Check(err >= 0, "Can't get period size for sound: ", snd_strerror(err));
    g_data.period_size = size;
    err = snd_pcm_hw_params(g_data.handle, hwparams);
    Check(err >= 0, "Can't set hw params for sound: ", snd_strerror(err));

    err = snd_pcm_sw_params_current(g_data.handle, swparams);
    Check(err >= 0, "Can't determine current sw params for sound: ",
            snd_strerror(err));
    err = snd_pcm_sw_params_set_start_threshold(g_data.handle, swparams,
            (g_data.buffer_size / g_data.period_size) * g_data.period_size);
    Check(err >= 0, "Can't set start threshold mode for sound: ",
            snd_strerror(err));
    err = snd_pcm_sw_params_set_avail_min(g_data.handle, swparams,
            g_data.period_size);
    Check(err >= 0, "Can't set avail min for sound: ", snd_strerror(err));
    err = snd_pcm_sw_params(g_data.handle, swparams);
    Check(err >= 0, "Can't set sw params for sound: ", snd_strerror(err));

    // start sound
    g_data.samples.resize(g_data.period_size * 2, 0);
    g_data.mix.resize(g_data.period_size * 2, 0);
    g_data.tmp.resize(g_data.period_size * 2, 0);
    err = snd_async_add_pcm_handler(&g_data.ahandler, g_data.handle,
            SoundMixerCallback, &g_data);
    if (err == -ENOSYS) {
        sound_thread = std::thread(arctic::SoundMixerThreadFunction);
    } else {
        Check(err >= 0, "Can't register async pcm handler for sound:",
                snd_strerror(err));
        for (int count = 0; count < 3; count++) {
            err = snd_pcm_writei(g_data.handle, g_data.samples.data(),
                    g_data.period_size);
            Check(err >= 0, "Sound pcm write error: ", snd_strerror(err));
            Check(err == g_data.period_size,
                    "Sound pcm write error: written != expected");
        }
        if (snd_pcm_state(g_data.handle) == SND_PCM_STATE_PREPARED) {
            err = snd_pcm_start(g_data.handle);
            Check(err >= 0, "Sound pcm start error: ", snd_strerror(err));
        }
    }
}

void StopSoundMixer() {
    int err = snd_async_del_handler(g_data.ahandler);
    Check(err >= 0, "Can't delete async sound handler", snd_strerror(err));
    snd_pcm_close(g_data.handle);
}

}  // namespace arctic


int main() {
    arctic::SystemInfo system_info;

    arctic::StartSoundMixer();
    CreateMainWindow(&system_info);
    arctic::easy::GetEngine();
    arctic::easy::GetEngine()->Init(system_info.screen_width,
            system_info.screen_height);

    EasyMain();

    XCloseDisplay(arctic::x_display);
    arctic::StopSoundMixer();

    return 0;
}

#endif  // ARCTIC_PLATFORM_PI
