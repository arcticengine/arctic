#!/bin/bash
# Regenerates the build files of every wizard-made project in this repository
# with the console wizard. Build the wizard first: cd wizard && cmake . && make
# headless_server is not on the list: it has a CMakeLists.txt of its own and no
# IDE projects, so there is nothing for the wizard to update there.
set -e
cd "$(dirname "$0")"
WIZARD=./wizard/wizard
if [ -x ./wizard/wizard.app/Contents/MacOS/wizard ]; then
  WIZARD=./wizard/wizard.app/Contents/MacOS/wizard
fi
for project in antarctica_pyramids benchmark tests wizard; do
  "${WIZARD}" update "${project}"
done
