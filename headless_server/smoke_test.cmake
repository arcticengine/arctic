# The smoke test of headless_server, run by ctest as
#   cmake -DSERVER=<executable> -DPORT=<port> -P smoke_test.cmake
#
# The server and the client start at the same moment, as two commands of one
# execute_process. CMake pipes the output of the first command into the input
# of the second, so the client goes first: the server never reads its input and
# the client, which exits first, is never written to after it is gone (the
# other order would end the server with SIGPIPE on its last printf). The client
# retries the connection until the server is listening, then sends ping, ticks
# and stop; the last one ends the server, so both processes exit and both exit
# codes must be zero. The tick budget is a safety net for a server that never
# gets the stop.
if (NOT DEFINED SERVER)
  message(FATAL_ERROR "smoke_test.cmake needs -DSERVER=<path to headless_server>")
endif ()
if (NOT DEFINED PORT)
  set(PORT 21113)
endif ()

execute_process(
  COMMAND "${SERVER}" --client --port ${PORT}
  COMMAND "${SERVER}" --port ${PORT} --ticks 1800
  RESULTS_VARIABLE results
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output
  TIMEOUT 60)

message(STATUS "server output:\n${output}")
list(GET results 0 client_result)
list(GET results 1 server_result)
if (NOT server_result EQUAL 0)
  message(FATAL_ERROR "the server exited with ${server_result}")
endif ()
if (NOT client_result EQUAL 0)
  message(FATAL_ERROR "the client exited with ${client_result}")
endif ()
