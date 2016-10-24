echo off
for %%f in (./engine/*) do c:\python27\python.exe ./tools/cpplint.py ./engine/%%f
for %%f in (./demo/*.h ./demo/*.cpp) do  c:\python27\python.exe ./tools/cpplint.py ./demo/%%f
pause