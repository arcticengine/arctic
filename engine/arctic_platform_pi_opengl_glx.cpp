// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2019 Huldra
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

#include "engine/arctic_platform_def.h"
#include "engine/arctic_platform.h"

#ifdef ARCTIC_PLATFORM_PI_OPENGL_GLX

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xatom.h>

#include "engine/easy.h"

extern void EasyMain();

namespace arctic {

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

Si32 g_window_width = 0;
Si32 g_window_height = 0;
Display *g_x_display;
Window g_x_window;

static Colormap g_x_color_map;
static XVisualInfo *g_glx_visual;
static const int kXEventMask = KeyPressMask | KeyReleaseMask | ButtonPressMask
  | ButtonReleaseMask | PointerMotionMask | ExposureMask
  | StructureNotifyMask;
static GLXContext g_glx_context;

void PumpMessages();

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*,
    GLXFBConfig, GLXContext, Bool, const int*);

void CreateMainWindow(SystemInfo *system_info) {
  const char *title = "Arctic Engine";

  g_x_display = XOpenDisplay(NULL);
  Check(g_x_display != NULL, "Can't open display.");

  XWindowAttributes window_attributes;
  Status is_good = XGetWindowAttributes(g_x_display,
      RootWindow(g_x_display, DefaultScreen(g_x_display)),
      &window_attributes);
  Check(is_good != 0, "Can't get window attributes.");
  g_window_width = window_attributes.width;
  g_window_height = window_attributes.height;

  Bool is_ok = glXQueryExtension(g_x_display, NULL, NULL);
  Check(is_ok, "Can't find OpenGL via glXQueryExtension.");

  g_glx_visual = glXChooseVisual(g_x_display,
      DefaultScreen(g_x_display), glx_config);
  Check(g_glx_visual != NULL, "Can't choose visual via glXChooseVisual.");

  g_x_color_map = XCreateColormap(g_x_display,
      RootWindow(g_x_display, g_glx_visual->screen),
      g_glx_visual->visual,
      AllocNone);

  glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
    glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

  int context_attribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 0,
    // GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
    None
  };

  static int visual_attribs[] = {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_DOUBLEBUFFER, true,
    GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1,
    None
  };


  int num_fbc = 0;
  GLXFBConfig *fbc = glXChooseFBConfig(g_x_display,
      DefaultScreen(g_x_display),
      visual_attribs, &num_fbc);
  if (!fbc) {
    printf("glXChooseFBConfig() failed\n");
    exit(1);
  }

  g_glx_context = glXCreateContextAttribsARB(
      g_x_display, fbc[0], None, GL_TRUE,
      context_attribs);
  Check(g_glx_context != NULL, "Can't create context via glXCreateContext.");

  XSetWindowAttributes swa;
  swa.colormap = g_x_color_map;
  swa.border_pixel = 0;
  swa.event_mask = kXEventMask;

  g_x_window = XCreateWindow(g_x_display,
      RootWindow(g_x_display, g_glx_visual->screen),
      0, 0, g_window_width, g_window_height,
      1,
      g_glx_visual->depth,
      InputOutput,
      g_glx_visual->visual,
      CWEventMask | CWBorderPixel | CWColormap, &swa);


  system_info->screen_width = g_window_width;
  system_info->screen_height = g_window_height;

  XStoreName(g_x_display, g_x_window, title);

  XWMHints wmHints;
  wmHints.flags = 0;
  wmHints.initial_state = NormalState;
  XSetWMHints(g_x_display, g_x_window, &wmHints);

  XSetIconName(g_x_display, g_x_window, title);
  XMapWindow(g_x_display, g_x_window);

  glXMakeCurrent(g_x_display, g_x_window, g_glx_context);

  glClearColor(1.0F, 1.0F, 1.0F, 0.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glFlush();
  return;
}

void ExitProgram() {
  exit(0);
}

void Swap() {
  glFlush();
  glXSwapBuffers(g_x_display, g_x_window);
  PumpMessages();
  arctic::easy::GetEngine()->OnWindowResize(g_window_width, g_window_height);
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

bool IsFullScreen() {
  return false;
}

void SetFullScreen(bool/* is_enable*/) {
  return;
}


}  // namespace arctic


int main(int argc, char **argv) {
  arctic::SystemInfo system_info;

  arctic::StartLogger();
  arctic::SoundPlayer soundPlayer;
  soundPlayer.Initialize();
  CreateMainWindow(&system_info);
  arctic::easy::GetEngine();
  arctic::easy::GetEngine()->SetArgcArgv(argc,
    const_cast<const char **>(argv));
  arctic::easy::GetEngine()->Init(system_info.screen_width,
      system_info.screen_height);

  EasyMain();

  XCloseDisplay(arctic::g_x_display);
  soundPlayer.Deinitialize();
  arctic::StopLogger();

  return 0;
}

#endif  // ARCTIC_PLATFORM_PI_OPENGL_GLX
