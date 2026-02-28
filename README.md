![Arctic Engine](doc/logo_black_on_transparent.png)
### Designed to give you control and not take anything away.

Arctic Engine is an open-source free game engine released under the MIT license. Arctic Engine is implemented in C++ and focuses on simplicity.

Many developers have forgotten exactly why it is that we make games. It's joyless, disillusioning and discouraging for them.

In the 80's and 90's it was possible for a programmer to make a game alone and it was Fun.

Arctic Engine returns the power to the C++ programmer and makes game development fun again.

API documentation: https://seaice.gitlab.io/arctic/index.html

Main discussion forum (in Russian): https://gamedev.ru/community/arctic/forum/

Windows:
antarctica pyramids [![Windows build status](https://ci.appveyor.com/api/projects/status/69n7xslx9f3tcoy3?svg=true)](https://ci.appveyor.com/project/FrostyMorning/arctic)
wizard [![Windows build status](https://ci.appveyor.com/api/projects/status/sa5a1rng94yb4w4o?svg=true)](https://ci.appveyor.com/project/FrostyMorning/arctic)
filetest [![Windows build status](https://ci.appveyor.com/api/projects/status/7tb6wk4xdwhp4dlq?svg=true)](https://ci.appveyor.com/project/FrostyMorning/arctic)

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
sudo apt-get install git cmake clang libasound2-dev libglu1-mesa-dev freeglut3-dev libgles2-mesa-dev
cd ~
git clone https://gitlab.com/seaice/arctic.git
cd ~/arctic
cd ./wizard
cmake .
make -j 4
./wizard
```

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

