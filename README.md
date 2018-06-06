![Arctic Engine](doc/logo_black_on_transparent.png)
### Designed to give you control and not take anything away.

Arctic Engine is an open-source free game engine released under the MIT license. Arctic Engine is implemented in C++ and focuses on simplicity.

Many developers have forgotten exactly why it is that we make games. It's joyless, disillusioning and discouraging for them.

In the 80's and 90's it was possible for a programmer to make a game alone and it was Fun.

Arctic Engine returns the power to the C++ programmer and makes game development fun again.

Main discussion forum (in Russian): http://arcticengine.com/

Windows:
antarctica pyramids [![Windows build status](https://ci.appveyor.com/api/projects/status/69n7xslx9f3tcoy3?svg=true)](https://ci.appveyor.com/project/FrostyMorning/arctic)
wizard [![Windows build status](https://ci.appveyor.com/api/projects/status/sa5a1rng94yb4w4o?svg=true)](https://ci.appveyor.com/project/FrostyMorning/arctic)
template [![Windows build status](https://ci.appveyor.com/api/projects/status/nelacdfwhihmnjx0?svg=true)](https://ci.appveyor.com/project/FrostyMorning/arctic)
fjortris [![Windows build status](https://ci.appveyor.com/api/projects/status/av1nhi147xf7ye6d?svg=true)](https://ci.appveyor.com/project/FrostyMorning/arctic)
filetest [![Windows build status](https://ci.appveyor.com/api/projects/status/7tb6wk4xdwhp4dlq?svg=true)](https://ci.appveyor.com/project/FrostyMorning/arctic)

Linux: [![Linux build status](https://gitlab.com/huldra/arctic/badges/master/build.svg)](https://gitlab.com/huldra/arctic/pipelines)

Scrum board: https://gitlab.com/huldra/arctic/boards?=

Code of Conduct: [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)

Arctic Engine follows Google C++ Style Guide: [https://google.github.io/styleguide/cppguide.html](https://google.github.io/styleguide/cppguide.html)

## License
Licensed under the MIT license, see License.txt for details.

tl;drLegal: [https://www.tldrlegal.com/l/mit](https://www.tldrlegal.com/l/mit)

## Credits

#### Arctic Engine code:

* Huldra aka FrostyMorning

#### Third-party components:

* piLibs C++ (14/06/2016) by Íñigo Quílez ([http://www.iquilezles.org/code/piLibs/piLibs.htm](http://www.iquilezles.org/code/piLibs/piLibs.htm))
* stb single-file public domain libraries for C/C++ (30/06/2017) Copyright (c) 2017 Sean Barrett ([https://github.com/nothings/stb](https://github.com/nothings/stb))

#### ArcticOne font:

* Huldra
* Barry Schwartz

#### Third-party data:

* Living Nightmare by snowflake Ft: Blue Wave Theory ([http://dig.ccmixter.org/files/snowflake/54422](http://dig.ccmixter.org/files/snowflake/54422)) (c) copyright 2016 Licensed under a Creative Commons Attribution (3.0) license.

## Tools used

#### UML Editor

* Visual Paradigm Community Edition ([https://www.visual-paradigm.com/download/community.jsp](https://www.visual-paradigm.com/download/community.jsp))

#### IDE

* Visual Studio Community 2017
([https://www.visualstudio.com/](https://www.visualstudio.com/))

#### Python

* Python 2.7 ([https://www.python.org/downloads/](https://www.python.org/downloads/))

#### Documentation generator

* Doxygen 1.8.13 ([http://www.doxygen.org](http://www.doxygen.org))

#### Bitmap Font Generator

* BMFont Bitmap Font Generator 1.14 beta ([http://www.angelcode.com/products/bmfont](http://www.angelcode.com/products/bmfont))

## Ubuntu and Raspbian linux build instruction

Just execute the following commands in terminal line by line to install all the required libraries and tools, clone the repository to ~/arctic, build and run the demo project: 

```bash
sudo apt-get install git cmake clang libasound2-dev libglu1-mesa-dev freeglut3-dev
cd ~
git clone https://gitlab.com/huldra/arctic.git
cd ~/arctic
cd ./wizard
cmake .
make all
./wizard
```

### Raspberry Pi notes

Arctic Engine has been tested only on Raspberry Pi 3 model B so far.

If you experience low sound quality on built-in audio output, in /boot/config.txt add the following line:
```
audio_pwm_mode=2
```
You might need to update your firmware in order for this to work.
