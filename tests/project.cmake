# =============================================================================
# Running the suite
# =============================================================================
# Included from CMakeLists.txt, which `wizard update` rewrites; this file stays.
#
# The suite is a single executable that runs every test and reports through its
# exit code, so `./tests.app/Contents/MacOS/tests` (or ./tests elsewhere) is
# still the shortest way to run it. It is registered with ctest as well, so that
# `ctest` from this directory works the way it does in any other project, and so
# that a name can be picked out with `ctest -R`.
#
# The working directory matters: the tests find their data relative to it and
# the logger writes log.txt into it. The window stays off the screen because
# main.cpp registers a startup mode decider asking for a hidden one, so a run
# by hand is as quiet as a run through ctest; the GL context is still made,
# which the graphics tests need.
if (NOT EMSCRIPTEN)
  enable_testing()
  add_test(NAME arctic_engine_tests COMMAND $<TARGET_FILE:${PROJECT_NAME}>)
  set_tests_properties(arctic_engine_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    TIMEOUT 900)
endif ()
