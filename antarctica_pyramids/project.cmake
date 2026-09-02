# Included from CMakeLists.txt, which `wizard update` rewrites; this file stays.
#
# The self-test: `--selftest` plays 300 random turns in a hidden window and
# exits with 0 if no invariant of the game broke, see RunSelftest in main.cpp.
# The working directory is the project folder, where data/ and settings.ini
# are; on macOS the bundle keeps its own copy of data/ but ctest runs the
# executable inside it from here all the same. `ctest` from this directory
# runs it after `make`.
#
# The second run has no sound device: ARCTIC_DISABLE_AUDIO keeps the game
# mute on Linux (a build bot has no speaker to open), and the game must not
# care. On macOS the variable is not read and the run is an ordinary one on
# another seed.
if (NOT EMSCRIPTEN)
  enable_testing()
  add_test(NAME antarctica_pyramids_selftest
    COMMAND $<TARGET_FILE:${PROJECT_NAME}> --selftest --seed 1)
  set_tests_properties(antarctica_pyramids_selftest PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TIMEOUT 300)
  add_test(NAME antarctica_pyramids_selftest_silent
    COMMAND $<TARGET_FILE:${PROJECT_NAME}> --selftest --seed 2)
  set_tests_properties(antarctica_pyramids_selftest_silent PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    ENVIRONMENT "ARCTIC_DISABLE_AUDIO=1"
    TIMEOUT 300)
endif ()
