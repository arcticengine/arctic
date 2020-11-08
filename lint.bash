#!/bin/bash
echo off
for filename in ./engine/*; do
  if [ $filename != "./engine/glcorearb.h" -a $filename != "./engine/glext.h" ]; then
    python ./tools/cpplint.py ${filename}
  fi
done
for filename in ./antarctica_pyramids/*.h; do
  if [ $filename != "./antarctica_pyramids/resource.h" ]; then
    python ./tools/cpplint.py ${filename}
  fi
done
for filename in ./antarctica_pyramids/*.cpp; do
    python ./tools/cpplint.py ${filename}
done
for filename in ./template_project_name/*.h; do
  if [ $filename != "./template_project_name/resource.h" ]; then
    python ./tools/cpplint.py ${filename}
  fi
done
for filename in ./template_project_name/*.cpp; do
  python ./tools/cpplint.py ${filename}
done
for filename in ./wizard/*.h; do
  if [ $filename != "./wizard/resource.h" ]; then
    python ./tools/cpplint.py ${filename}
  fi
done
for filename in ./wizard/*.cpp; do
  python ./tools/cpplint.py ${filename}
done
