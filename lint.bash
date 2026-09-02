#!/bin/bash
# Runs cpplint over every source directory of the repository. The generated
# resource.h files and the OpenGL headers are skipped.
lint_dir() {
  local dir="$1"
  for filename in "${dir}"/*.h "${dir}"/*.cpp; do
    if [ ! -f "${filename}" ]; then
      continue
    fi
    case "${filename}" in
      */resource.h|./engine/glcorearb.h|./engine/glext.h)
        continue
        ;;
    esac
    python ./tools/cpplint.py "${filename}"
  done
}
for dir in ./engine ./antarctica_pyramids ./benchmark ./headless_server \
    ./template_project_name ./tests ./wizard; do
  lint_dir "${dir}"
done
