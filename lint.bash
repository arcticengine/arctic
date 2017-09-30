#!/bin/bash
echo off
for filename in ./engine/*; do
    python ./tools/cpplint.py ${filename}
done
for filename in ./demo/*.h; do
    python ./tools/cpplint.py ${filename}
done
for filename in ./demo/*.cpp; do
    python ./tools/cpplint.py ${filename}
done
