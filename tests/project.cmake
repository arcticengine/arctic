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

# =============================================================================
# SIGIO heap-race reproduction binary (Linux / ALSA only)
# =============================================================================
# Production StartSoundMixer always uses a dedicated thread. This separate
# target reintroduces MixSound/SoundCheck-from-signal only under
# ARCTIC_TEST_SIGIO_REPRO so the historical "free(): double free detected in
# tcache" hazard can be demonstrated. It is NOT part of the default ./tests
# suite. See tools/alsa/run_sigio_heap_repro.sh.
if (UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND ALSA_FOUND)
  set(SIGIO_REPRO_SOURCES ${SRC_FILES})
  list(REMOVE_ITEM SIGIO_REPRO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
  # Drop the acutest suite sources; this binary has its own main.
  file(GLOB SIGIO_REPRO_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")
  if (SIGIO_REPRO_TEST_SOURCES)
    list(REMOVE_ITEM SIGIO_REPRO_SOURCES ${SIGIO_REPRO_TEST_SOURCES})
  endif ()
  list(APPEND SIGIO_REPRO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/sigio_repro/main.cpp")

  add_executable(tests_sigio_repro ${SIGIO_REPRO_SOURCES})
  # ARCTIC_NO_MAIN: use sigio_repro/main.cpp instead of the GLX platform main.
  target_compile_definitions(tests_sigio_repro PRIVATE ARCTIC_TEST_SIGIO_REPRO ARCTIC_NO_MAIN)

  # Mirror the Linux link set of the main suite.
  target_link_libraries(tests_sigio_repro
    ${OPENGL_gl_LIBRARY}
    ${X11_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${ALSA_LIBRARY}
  )
  if (GSTREAMER_FOUND)
    target_link_libraries(tests_sigio_repro ${GSTREAMER_LIBRARIES})
    target_link_directories(tests_sigio_repro PUBLIC ${GSTREAMER_LIBRARY_DIRS})
  endif ()
  if (OPENSSL_FOUND)
    target_link_libraries(tests_sigio_repro OpenSSL::SSL OpenSSL::Crypto)
  endif ()

  # Convenience: `make sigio_heap_repro` builds the binary.
  add_custom_target(sigio_heap_repro DEPENDS tests_sigio_repro)
endif ()
