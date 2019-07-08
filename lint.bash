#!/bin/bash
echo off
for filename in ./engine/*; do
    python ./tools/cpplint.py ${filename}
done
for filename in ./antarctica_pyramids/*.h; do
    python ./tools/cpplint.py ${filename}
done
for filename in ./antarctica_pyramids/*.cpp; do
    python ./tools/cpplint.py ${filename}
done
for filename in ./template_project_name/*.h; do
python ./tools/cpplint.py ${filename}
done
for filename in ./template_project_name/*.cpp; do
python ./tools/cpplint.py ${filename}
done
for filename in ./wizard/*.h; do
python ./tools/cpplint.py ${filename}
done
for filename in ./wizard/*.cpp; do
python ./tools/cpplint.py ${filename}
done
