Arctic Engine
=============

Arctic Engine will solve your problem of where to start with documentation,
by providing a basic explanation of how to do it easily.

Look how easy it is to use:

#include "engine/easy.h"

using namespace arctic;
using namespace arctic::easy;

void EasyMain() {
  while (!IsKeyDownward(kKeyEscape)) {
    Clear();
    DrawCircle(Vec2Si32(100, 100), 50, Rgba(255, 255, 255));
    ShowFrame();
  }
}

Features
--------

- Easy to use
- Quick code-build-run cycle

Installation
------------

Clone or download the arctic engine repository. Enter the wizard directory. Open and build the project with Visual Stuio, XCode or CLion or build it with cmake . && make.
Run the wizard and follow the instructions.

Contribute
----------

- Issue Tracker: https://gitlab.com/huldra/arctic/-/issues
- Source Code: https://gitlab.com/huldra/arctic/-/tree/master

Support
-------

If you are having issues, please let us know: arcticengine@gmail.com

License
-------

Arctic Engine is licensed under the MIT license.
