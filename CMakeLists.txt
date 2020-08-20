cmake_minimum_required(VERSION 3.5)

set(MODULE_NAME runtime)
project(${MODULE_NAME} CXX)
message(STATUS "Configuring ${MODULE_NAME} ... ")


###############################################
# option
###############################################
# path
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(LIBRARY_OUTPUT_PATH    "${CMAKE_BINARY_DIR}")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -ggdb -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -ggdb -std=c++11")

option(RUNTIME_JUST_LIBRARY "Runtime build library only" ON)

# component
set(GPERFTOOLS_DIR "${${MODULE_NAME}_SOURCE_DIR}/3rdparty/gperftools")
set(SPDLOG_DIR     "${${MODULE_NAME}_SOURCE_DIR}/3rdparty/spdlog")
set(GPU_SOURCE     "${${MODULE_NAME}_SOURCE_DIR}/gpu")
set(CPU_SOURCE     "${${MODULE_NAME}_SOURCE_DIR}/cpu")
set(TEST_SOURCE    "${${MODULE_NAME}_SOURCE_DIR}/tests")


###############################################
# function
###############################################
function(MAKE_GPERFTOOLS SOURCES_DIR)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -w")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -w")
  find_program(COMMAND_AUTORECONF autoreconf)
  if(NOT(COMMAND_AUTORECONF))
    message(FATAL_ERROR "Miss command automake libtool. Install automake libtool first")
  endif()

  file(GLOB COMMAND_AUTOGEN "${SOURCES_DIR}/autogen.sh")
  if((COMMAND_AUTOGEN) AND (COMMAND_AUTORECONF))
      execute_process(COMMAND ${COMMAND_AUTOGEN} WORKING_DIRECTORY "${SOURCES_DIR}" RESULT_VARIABLE EXECUTE_RESULT)
    if(EXECUTE_RESULT)
        message(FATAL_ERROR "Execute ./autogen.sh failed in ${SOURCES_DIR}")
    endif()
  endif()

  file(GLOB COMMAND_CONFIGURE "${SOURCES_DIR}/configure")
  if(COMMAND_CONFIGURE)
      execute_process(COMMAND ${COMMAND_CONFIGURE} --prefix=${LIBRARY_OUTPUT_PATH} --quiet
          WORKING_DIRECTORY "${SOURCES_DIR}" RESULT_VARIABLE EXEUCTE_RESULT)
    if(EXECUTE_RESULT)
        message(FATAL_ERROR "Execute ./configure failed in ${SOURCES_DIR}")
    endif()
  endif()

  execute_process(COMMAND make -j32    WORKING_DIRECTORY "${SOURCES_DIR}")
  execute_process(COMMAND make install WORKING_DIRECTORY "${SOURCES_DIR}")
endfunction()

function(MODULE_NAME, LINK_FLAGS)
  message(STATUS "Enable module test ${MODULE_NAME}")
  file(GLOB_RECURSE MODULE_SOURCES "${TEST_SOURCE}/${MODULE_NAME}*.cpp")
  if (NOT MODULE_SOURCES)
    message(STATUS "Miss module test ${MODULE_NAME}")
    return()
  endif()
  add_executable(${MODULE_NAME} ${MODULE_SOURCES})
  target_link_libraries(${MODULE_NAME} PRIVATE ${LINK_FLAGS})

  add_test(NAME ${MODULE_NAME} COMMAND ./test/${MODULE_NAME})
  set_tests_properties(${MODULE_NAME} PROPERTIER PASS_REGULAR_EXPERSSION "PASSED")
  set_tests_properties(${MODULE_NAME} PROPERTIER FAIL_REGULAR_EXPERSSION "FAILED")
endfunction()

###############################################
# configuration
###############################################
# gperftools
message(STATUS "[ gperftools ]")
if(EXISTS "${GPERFTOOLS_DIR}")
  if(EXISTS "${CMAKE_BINARY_DIR}/bin/pprof")
    message(STATUS "Found gperftools already exsits, skip make gperftools.")
  else()
    MAKE_GPERFTOOLS(${GPERFTOOLS_DIR})
  endif()
else()
  # GPERFTOOLS_DIR NOT EXISTS
  message(FATAL_ERROR "Git pull gperftools into 3rdparty.")
endif()

# spdlog
message(STATUS "[ spdlog ]")
if(EXISTS "${SPDLOG_DIR}/CMakeLists.txt")
  add_subdirectory(${SPDLOG_DIR})
else()
  # SPDLOG_DIR NOT EXISTS
  message(FATAL_ERROR "Git pull spdlog into 3rdparty.")
endif()

file(GLOB_RECURSE SOURCES "${GPU_SOURCE}/*.cpp" "${CPU_SOURCE}/*.cpp")
add_library(${MODULE_NAME} ${SOURCES})
target_include_directories(${MODULE_NAME} PUBLIC  ${${MODULE_NAME}_SOURCE_DIR})  # .h of this dir.
target_include_directories(${MODULE_NAME} PRIVATE ${SPDLOG_DIR}/include)         # .h of spdlog
target_include_directories(${MODULE_NAME} PRIVATE ${EXECUTABLE_OUTPUT_PATH}/include)  # .h of gperftools (together with output lib)

###############################################
# ctest
###############################################
if(RUNTIME_JUST_LIBRARY)
  message(STATUS "Building just the library, omitting all tests.")
else()
  enable_testing()
  find_program(FIND find)
  find_program(FILE file)
  find_program(GREP grep)
  if((FIND) AND (FILE) AND (GREP))
      add_test(
        NAME "just_ascii"
        COMMAND sh -c "${FIND}  include src windows tools singleheader tests examples benchmark -path benchmark/checkperf-reference -prune -name '*.h'  -o -name '*.cpp' -type f  -exec ${FILE} '{}' \; |${GREP} -v ASCII || exit 0  && exit 1"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      )
  endif()
endif()
