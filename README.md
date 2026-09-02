![Arctic Engine](doc/logo_black_on_transparent.png)
### Designed to give you control and not take anything away.

Arctic Engine is an open-source free game engine released under the MIT license. Arctic Engine is implemented in C++ and focuses on simplicity.

Many developers have forgotten exactly why it is that we make games. It's joyless, disillusioning and discouraging for them.

In the 80's and 90's it was possible for a programmer to make a game alone and it was Fun.

Arctic Engine returns the power to the C++ programmer and makes game development fun again.

## Starting a project

Always make a project with the wizard, never by copying `template_project_name` by hand. The directory name, the CMake target, the bundle name, the Xcode and Visual Studio projects, the relative path to the engine and the engine file lists inside those projects all have to agree with each other, and the wizard is what makes them agree; a copied template builds under the wrong name or does not build at all.

Build the wizard once (see the build instructions below), then:

```bash
wizard create mygame                     # the default template
wizard create mygame --template hello    # a chosen one
wizard update ../mygame                  # refresh a project made earlier
wizard --help                            # the list of templates
```

A project name is a directory name and a build target name at once, so it takes lowercase latin letters, digits and underscores, and starts with a letter. The project is created next to the engine directory, which is what the `../arctic` include path in the generated project expects. `update` rewrites only the generated parts, engine file lists above all, and leaves your own sources alone; it finds the engine next to the wizard binary, so it works from any directory.

Run with no arguments and the wizard opens its window. With a subcommand it never opens one, so `create` and `update` work over ssh, in a container and on a build bot, and they end with 0 on success or 1 on a mistake in the arguments.

Build the project in the project directory:

```bash
cmake . && make -j 8                          # in the source directory
```

The templates are small on purpose; the place to see the subsystems working together is `antarctica_pyramids`, a roguelike of about two thousand lines in this repository. `game.cpp` is the maze and the turn, `ui.cpp` the menu, the panel of actions and the dialogs built with `GuiFactory` from the theme in `data/gui_theme.xml`, with `Panel::IsInside` telling a click on the interface from a click on the world; `sfx.cpp` synthesizes the effects into `Sound` objects at start and plays the music under a checkbox and a volume slider. The hero's actions come from `data/actions.csv` through `CsvTable`, the volume and the music flag live in `settings.ini` through `IniFile`, `--seed N` replays a maze through `SetRandomSeed`, and every level and outcome goes to `log.txt`. `SetMainWindowCloseHandler` turns the close box into a "Quit?" dialog, and `--selftest` plays three hundred random turns in a hidden window chosen by a startup mode decider, which is how `ctest` in that directory checks the game on a build bot. The `CMakeLists.txt` of every wizard project includes an optional `project.cmake` next to it for additions like that `add_test`, since `wizard update` rewrites `CMakeLists.txt` itself.

The engine needs clang; CMake honours `-DCMAKE_CXX_COMPILER=...` and the `CXX` environment variable, otherwise it looks for `clang++` in the PATH and says what to install when there is none. On Linux the ALSA headers are optional: without them the build warns, defines `ARCTIC_NO_ALSA` and the program runs mute. `-DARCTIC_GRAPHICS=auto|glx|gles` chooses the graphics backend, and `auto` takes GLX everywhere except a Raspberry Pi, which keeps GLES through EGL. `ARCTIC_HEADLESS=1 ARCTIC_DISABLE_HW=1` in the environment runs a program with no window, no GL context and no sound device, while `ARCTIC_DISABLE_AUDIO=1` alone only disables the sound.

## Rendering architecture

Arctic Engine provides two rendering paths that share a familiar API.

**Sprite** is the software renderer. Pixel data lives in CPU memory and all drawing (`DrawLine`, `DrawTriangle`, `DrawRectangle`, `SetPixel`, etc.) is performed on the CPU. This is the default path you get through `#include "engine/easy.h"`. It is great for learning, pixel art, procedural generation, and any situation where you want direct per-pixel control.

**HwSprite** is the hardware-accelerated renderer. Texture data lives on the GPU (OpenGL / OpenGL ES), and drawing is performed via GPU draw calls. The Draw() API mirrors Sprite, so switching from software to hardware rendering in most cases only requires replacing `Sprite` with `HwSprite` in your declarations. The `antarctica_pyramids` game in this repository mixes the two: the maze, the creatures and their health bars are hardware sprites and `DrawRectangleHw`, while the snow of the intro and the text are drawn in the software backbuffer that is composed above them. It is available through `engine/easy_hw_sprite.h` (also reachable via `engine/engine.h`, which `easy.h` includes).

Both paths share one coordinate system, and so do the mouse, the text and the GUI: pixels of the backbuffer, `(0, 0)` at the **bottom-left** corner, y growing **upward**. There is no separate interface space, so a world position and a panel position are directly comparable. Two things about it are worth knowing before the first sprite lands in the wrong place. A position given to `Draw` is where the sprite's **pivot** goes, which is its bottom-left corner only while the pivot is `(0, 0)` -- and a tga brings its own pivot from the origin field of the file, so the same call places a tga and a png differently. And a layout measured from the top of the screen has to be converted rather than negated by hand: `FromTopLeft()` and `ToTopLeft()` in `engine/easy_util.h` do it, `IsPointInSprite()` hit-tests a drawn sprite the way `Draw` actually placed it, and `SetInverseY()` is not the tool for this -- it mirrors the finished frame, text included. The whole convention is in the "Where Zero Is and Which Way Is Up" section of the documentation.

The order things end up in is fixed and worth remembering: among hardware sprites the later `Draw` call is the one on top, and the software backbuffer is composed above every hardware sprite regardless of call order, so `Font::Draw`, `DrawRectangle` and the GUI are always on top. `DrawRectangleHw` fills a rectangle through the hardware path without a texture of your own, for the walls and bars that would otherwise become a sprite each.

The window is named by `SetWindowTitle("...")` at any time, before the window exists as well, and `WindowTitle()` reads it back. By default it is the name of the executable file rather than the name of the engine, so two programs, or two copies of one, are told apart in the window list without writing a line.

## 3D and low-level GPU access

Beyond 2D sprites, the engine ships with infrastructure for 3D rendering. These headers are not part of the `easy.h` convenience include and should be included directly as needed:

**3D math** -- `Vec3F`, `Vec4F`, `Mat44F`, `QuaternionF`, `Frustum3F`, `Bound3F` (headers in `engine/`).

**Mesh system** -- the `Mesh` class with multiple vertex streams, primitive generators (`Mesh_GeneratePlane`, `Mesh_GenerateCube`, `Mesh_GenerateTorus`, `Mesh_PatchedSphere`), and file loaders for OBJ and PLY formats.

**OpenGL wrappers** -- `GlProgram` (shader programs), `GlBuffer` (vertex/index buffers), `GlTexture2D` (textures), `GlFramebuffer` (render targets). These give you direct but convenient access to the GPU pipeline for custom 3D rendering, shadow maps, post-processing, and anything else OpenGL can do.

**Skeletal animation** -- `piSkeleton` for bone hierarchies and skeletal transforms.

### Writing a 3D pass

The `cube` template (`wizard create mygame --template cube`) is a complete working example: a mesh, a camera, two moving point lights, a render target with a depth buffer, and a screenshot on F12. What follows is what it does and why.

Name the vertex elements after the shader attributes they feed, and the mesh binds them by itself:

```cpp
MeshVertexFormat format;
format.AddElement("vPosition", 3, kRMVEDT_Float);
format.AddElement("vNormal", 3, kRMVEDT_Float);
format.AddElement("vTexCoord", 2, kRMVEDT_Float);
mesh.Init(1, vertex_count, &format, kRMVEDT_Polys, 1, face_count);
// ... AddVertex and AddFace ...
program.Create(vertex_shader, fragment_shader,
    {"vPosition", "vNormal", "vTexCoord"});
mesh.Draw(program);
```

`Mesh::Draw` keeps the vertex and index buffers of the mesh on the GPU, uploads them again whenever the geometry changes, asks the program where each named element goes and skips the ones the shader does not declare. A format with no names keeps the old behaviour, element `i` to slot `i`. The names are not copied, so they have to outlive the mesh; string literals do.

`Init` fixes the capacity of the mesh. `AddVertex` and `AddFace` never grow it: past the capacity they write a line to the log and return -1, so an underestimate is visible rather than silently missing geometry. Count the geometry first, or call `Expand` to make room, remembering that it moves the buffers and invalidates every pointer `GetVertexData` returned.

The two names for the size of things are worth keeping apart. `ScreenSize()` is the resolution of the 2D backbuffer, which is whatever `ResizeScreen` last set, and `WindowSize()` is the window in real pixels of the display, which is twice the size in points on a HiDPI or Retina screen. The engine starts them equal and never touches the backbuffer afterwards, so a resized window leaves them apart, and a render target built for the old size then covers a part of the window. A 3D pass that wants the whole window follows the window itself:

```cpp
if (WindowSize() != known_size) {
  known_size = WindowSize();
  ResizeScreen(known_size);
  target.Create(known_size.x, known_size.y);
  target.sprite_instance()->framebuffer().AttachDepthBuffer(
      known_size.x, known_size.y);
}
```

Uniform arrays are a weak spot of some OpenGL ES and WebGL drivers, which is why the template gives each of its two lamps a `vec3` uniform of its own instead of an array of two. Prefer `light0Pos`, `light1Pos` to `lightPos[2]` in a shader that has to run on the web or on a phone.

## GUI

The `gui` template (`wizard create mygame --template gui`) is a complete program with every widget of `engine/gui.h` in it, each doing the job it exists for in a small dot-drawing toy: a `GuiFactory` with the theme from `data/gui_theme.xml` makes a `Panel` with a `TabControl` of two pages, `RadioButton`s for the shape of the dot, a horizontal and a vertical `Slider` for its radius and brightness, a `Dropdown` for its color, an `Image` with a preview redrawn on every change, an `Editbox` for a caption, a `Checkbox` and a `Scrollbar` for the grid, a `ListBox` of the dots placed, `Button`s that remove and clear them, a `Progressbar` of how full the scene is and a `Text` status line, with a tooltip where one helps. The input queue goes to the root panel through `ApplyInput`, every reaction is a named function set as the widget's callback (`OnSelect`, `OnSliderChange`, `OnChange`, `OnEditDone`, `OnButtonClick`), and the panel is drawn last so that it, its tooltips and the open dropdown list are on top. The one decision the template shows that the engine cannot make for you is which clicks belong to the world: `Panel::IsInside(backbuffer_pos)` says whether a click landed on the interface, and a click that did not puts a dot on the screen. A bare panel lets the clicks on its empty parts and on its labels through, which is right for widgets floating over the world and wrong for a window, so the template's panel is made a window with `SetClickable(true)`. The panel is positioned in the same backbuffer pixels the mouse reports, so there is nothing to convert.

`Screenshot()` returns the frame as a software `Sprite`, and `Sprite::Save` writes `.tga` or `.png`, so a screenshot key costs three lines. Call it before `ShowFrame()`: the engine assembles the frame a second time into a texture to read it back, because a window that has been shown can no longer be read.

## Image files

`Sprite::Load` and `HwSprite::Load` read `.tga` and `.png`, and `Sprite::Save` writes both; the extension decides and its case does not matter. Grayscale, palette based and true color files are all read, 8 or 16 bits per channel, run length encoded tga and interlaced png included, and what comes out is always a sprite of 8 bit rgba pixels. A file that can not be read leaves the sprite empty and writes the reason to the log instead of stopping the program, so check `Width()` when a missing asset matters. The one thing a png lacks is the origin field of a tga, which the engine takes for the pivot: a sprite loaded from a png starts with its pivot at zero. Fonts go through the same loader, so the texture named inside a `.fnt` may be a png too.

## Reproducible randomness

`Random32`, `Random64`, `RandomF` and their kin draw from four thread-local generators, one per width. `SetRandomSeed(seed)` starts the sequence of the calling thread from the beginning, which is what a level generator or a test needs to repeat itself, and every thread that has to be reproducible sets its own seed.

Saving a game or running several independent sequences on one thread needs more than a seed, because a seed only rewinds to the start. `GetRandomState()` returns everything the four generators are about to give and `SetRandomState(state)` puts it back, so the numbers continue from the place the state was taken:

```cpp
RandomState state = GetRandomState();
Si32 a = Random32(1, 100);
SetRandomState(state);
Si32 b = Random32(1, 100);  // the very same number
```

A state is a value of about ten kilobytes, so keeping one per world chunk and switching between them costs a copy. For a save file `RandomState::ToString` writes it as text and `RandomState::FromString` reads it back, in another run of the program as well; a text that does not parse is refused and leaves the state as it was, so check the returned `bool` instead of trusting a damaged save.

## Networking

**Sockets** -- `engine/arctic_platform_tcpip.h` gives you TCP and UDP sockets over IPv4 and IPv6 with a single API across the supported platforms, for protocols of your own. The header is not part of `easy.h`, so include it where you need it.

**A server with no window** -- the `headless_server` project in this repository is a complete one, in about two hundred lines: a startup mode decider that returns `StartupMode::kNoWindow`, a `ListenerSocket` on a port, non-blocking accepts and reads, a tick paced by `Time()` and `Sleep()`, and an exit on a command from a client or on a budget of ticks. The same executable run with `--client` talks to it, so the example is runnable by itself:

```
cd headless_server && cmake . && make -j 8
./headless_server.app/Contents/MacOS/headless_server --port 21112 &
./headless_server.app/Contents/MacOS/headless_server --client --port 21112
```

Pacing the loop by hand is not optional there. With a window, hidden or not, `ShowFrame()` swaps the buffers and vertical synchronization sets the pace; with `kNoWindow` there is nothing to swap and nothing to wait for, so a loop without a `Sleep()` of its own spins at the speed of the processor and eats a whole core.

**HTTP** -- `engine/httplib.h` is a vendored copy of cpp-httplib, a header-only HTTP/HTTPS library: `httplib::Client` makes requests, `httplib::Server` serves them. The whole library is one large header, so include it directly (it is not part of `easy.h`) and in as few translation units as you can, to keep compile times sane.

HTTPS additionally requires OpenSSL. A project made from the template wires it up on its own in the CMake build: `CMakeLists.txt` looks for OpenSSL, and when it is installed the build defines `CPPHTTPLIB_OPENSSL_SUPPORT` and links `OpenSSL::SSL`, `OpenSSL::Crypto` and (on macOS, for the keychain roots) the `Security` framework, so `httplib::SSLClient` just works. OpenSSL is never required: without it the build says so and only plain HTTP is available. Install it with `apt-get install libssl-dev` or `brew install openssl` and reconfigure to turn HTTPS on.

The bundled Visual Studio and Xcode projects leave TLS off, because they cannot test whether OpenSSL is installed the way a CMake configure step can, and enabling it unconditionally would break every generated project on a machine without OpenSSL. Plain HTTP works there out of the box. Turning TLS on is a matter of four settings, which in Xcode look like this:

```
GCC_PREPROCESSOR_DEFINITIONS  = $(inherited) CPPHTTPLIB_OPENSSL_SUPPORT
HEADER_SEARCH_PATHS           = ... /opt/homebrew/opt/openssl@3/include /usr/local/opt/openssl@3/include
LIBRARY_SEARCH_PATHS          = $(inherited) /opt/homebrew/opt/openssl@3/lib /usr/local/opt/openssl@3/lib
OTHER_LDFLAGS                 = $(inherited) -lssl -lcrypto -framework Security
```

Listing both prefixes covers Homebrew on Apple Silicon and on Intel, and a path that does not exist is simply ignored. Visual Studio needs the same four things, with the paths pointing at wherever OpenSSL is installed on that machine.

Nothing has to be added to those projects for the library itself: like every other engine header, `engine/httplib.h` is found through the include path, and the wizard's update mode picks it up when it refreshes the engine file list of an existing project.

API documentation: https://seaice.gitlab.io/arctic/index.html

Main discussion forum (in Russian): https://gamedev.ru/community/arctic/forum/

Windows:
antarctica pyramids [![Windows build status](https://ci.appveyor.com/api/projects/status/69n7xslx9f3tcoy3?svg=true)](https://ci.appveyor.com/project/FrostyMorning/arctic)
wizard [![Windows build status](https://ci.appveyor.com/api/projects/status/sa5a1rng94yb4w4o?svg=true)](https://ci.appveyor.com/project/FrostyMorning/arctic)

Linux: [![Linux build status](https://gitlab.com/seaice/arctic/badges/master/pipeline.svg)](https://gitlab.com/seaice/arctic/pipelines)


Scrum board: https://trello.com/b/9AnYCH7e/arctic-engine

Code of Conduct: [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)

Arctic Engine follows a bit modified Google C++ Style Guide: [https://google.github.io/styleguide/cppguide.html](https://google.github.io/styleguide/cppguide.html)
See [STYLE.md](STYLE.md) for the details.

## License
Licensed under the MIT license, see License.txt for details.

tl;drLegal: [https://www.tldrlegal.com/l/mit](https://www.tldrlegal.com/l/mit)

## Credits
See License.txt for details.

#### Arctic Engine code:

* Huldra
* Vlad2001_MFS
* The Lasting Curator

#### Third-party components:

* piLibs C++ (14/06/2016) by Íñigo Quílez ([http://www.iquilezles.org/code/piLibs/piLibs.htm](http://www.iquilezles.org/code/piLibs/piLibs.htm))
* stb single-file public domain (or MIT Licensed) libraries for C/C++ (30/06/2017) Copyright (c) 2017 Sean Barrett ([https://github.com/nothings/stb](https://github.com/nothings/stb))
* Miniz 2.1.0 (01/06/2019) Copyright 2013-2014 RAD Game Tools and Valve Software, Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC, Copyright (c) 2016 Martin Raiber ([https://github.com/richgel999/miniz](https://github.com/richgel999/miniz))
* OpenFBX (01/06/2019) by Mikulas Florek ([https://github.com/nem0/OpenFBX](https://github.com/nem0/OpenFBX))
* Collection of wait-free/lock-free queues (18/07/2020) Copyright (c) 2018 Vitaliy Manushkin ([https://gitlab.com/agrianius/mt_queue](https://gitlab.com/agrianius/mt_queue))
* [Coverage-guided fuzz testing](https://docs.gitlab.com/ee/user/application_security/coverage_fuzzing/#coverage-guided-fuzz-testing-ultimate) added by @stkerr at GitLab
* Sound mixing function proposed by Mikle
* pugixml. Light-weight, simple and fast XML parser for C++ with XPath support. Copyright (c) 2006 - 2020 Arseny Kapoulkine ([https://pugixml.org/](https://pugixml.org/))
* SocketSys. Modular C++17 Socket Wrapper that supports multiple operating systems. Copyright (c) 2020 Asyc ([https://github.com/Asyc/SocketSys](https://github.com/Asyc/SocketSys))
* cpp-httplib 0.50.1. A C++ header-only cross platform HTTP/HTTPS library. Copyright (c) 2026 Yuji Hirose ([https://github.com/yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib))
* option-parser. A Lightweight, header-only CLI option parser for C++ Copyright (c) 2020 Luke de Oliveira <lukedeo@ldo.io>, Copyright (c) 2017 Romain Sylvian ([https://github.com/lukedeo/option-parser](https://github.com/lukedeo/option-parser))
* easing function collection Copyright (c) 2019 Juan Carlos, Copyright (c) 2001 Robert Penner
* Acutest -- Another C/C++ Unit Test facility. Copyright (c) 2013 - 2017 Martin Mitas ([http://github.com/mity/acutest](http://github.com/mity/acutest))
* OpenGL headers Copyright (c) 2013 - 2016 The Khronos Group Inc.
* Library for Anti-commutative Dual Complex Numbers. Copyright (c) 2014 Shizuo KAJI <shizuo.kaji@gmail.com> ([http://arxiv.org/abs/1601.01754](http://arxiv.org/abs/1601.01754))

#### ArcticOne font:

* Barry Schwartz
* Huldra
* Vitaliy Manushkin

#### Third-party data:

* Living Nightmare by snowflake Ft: Blue Wave Theory ([http://dig.ccmixter.org/files/snowflake/54422](http://dig.ccmixter.org/files/snowflake/54422)) (c) copyright 2016 Licensed under a Creative Commons Attribution (3.0) license.
* Some of the sounds in this project were created by David McKee (ViRiX) soundcloud.com/virix

## Tools used

#### UML Editor

* Visual Paradigm Community Edition ([https://www.visual-paradigm.com/download/community.jsp](https://www.visual-paradigm.com/download/community.jsp))

#### IDE

* Visual Studio Community 2019
([https://www.visualstudio.com/](https://www.visualstudio.com/))
* CLion ([https://www.jetbrains.com/?from=ArcticEngine](https://www.jetbrains.com/?from=ArcticEngine))

#### Python

* Python 2.7 ([https://www.python.org/downloads/](https://www.python.org/downloads/))

#### Documentation generator

* Doxygen 1.8.13 ([http://www.doxygen.org](http://www.doxygen.org))

#### Bitmap Font Generator

* BMFont Bitmap Font Generator 1.14 beta ([http://www.angelcode.com/products/bmfont](http://www.angelcode.com/products/bmfont))

#### Linter

* Copyright (c) 2009 Google Inc. All rights reserved.

## Ubuntu and Raspbian linux build instruction

Just execute the following commands in terminal line by line to install all the required libraries and tools, clone the repository to ~/arctic, build and run the demo project: 

```bash
sudo apt-get install git cmake clang libasound2-dev libglu1-mesa-dev freeglut3-dev libgles2-mesa-dev libssl-dev
cd ~
git clone https://gitlab.com/seaice/arctic.git
cd ~/arctic
cd ./wizard
cmake .
make -j 4
./wizard
```

Of that list only `git`, `cmake`, `clang` and the X11 and OpenGL development files are mandatory. `libasound2-dev` gives sound, and without it the build warns and the program runs mute; `libgles2-mesa-dev` gives the GLES backend, which a desktop takes only when asked with `-DARCTIC_GRAPHICS=gles` or when there is no desktop OpenGL at all; `libssl-dev` gives HTTPS in `httplib`.

### Raspberry Pi notes

Arctic Engine has been tested only on Raspberry Pi 3 model B so far.

If you experience low sound quality on built-in audio output, in /boot/config.txt add the following line:
```
audio_pwm_mode=2
```
You might need to update your firmware in order for this to work.

### VS Code and Cursor notes

To set up the project in VS Code or Cursor, open your project directory as the workspace, then create `.vscode/c_cpp_properties.json` with include paths pointing at the arctic engine directory:
```json
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/../arctic",
                "${workspaceFolder}"
            ],
            "defines": [],
            "compilerPath": "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.xx.xxxxx/bin/Hostx64/x64/cl.exe",
            "cStandard": "c11",
            "cppStandard": "c++14",
            "intelliSenseMode": "windows-msvc-x64"
        }
    ],
    "version": 4
}
```
Adjust the `compilerPath` to match your compiler installation.

