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
set(CMAKE_CXX_FLAGS_DEBUG   "$ENV{CXXFLAGS} -O0 -ggdb -std=c++14")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -ggdb -std=c++14")

option(RUNTIME_JUST_LIBRARY "Runtime build library only" ON)

# 3rdparty
set(GPERFTOOLS_DIR "${${MODULE_NAME}_SOURCE_DIR}/3rdparty/gperftools")
set(SPDLOG_DIR     "${${MODULE_NAME}_SOURCE_DIR}/3rdparty/spdlog")
set(GOOGLETEST_DIR "${${MODULE_NAME}_SOURCE_DIR}/3rdparty/googletest")

# component
set(COMMON_SOURCE_DIR "${${MODULE_NAME}_SOURCE_DIR}/common")
set(GPU_SOURCE_DIR  "${${MODULE_NAME}_SOURCE_DIR}/gpu")
set(CPU_SOURCE_DIR  "${${MODULE_NAME}_SOURCE_DIR}/cpu")
set(TEST_SOURCE_DIR "${${MODULE_NAME}_SOURCE_DIR}/tests")


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
  file(GLOB_RECURSE MODULE_SOURCES "${TEST_SOURCE_DIR}/${MODULE_NAME}*.cpp")
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

message(STATUS "[ googletest ]")
if(EXISTS "${GOOGLETEST_DIR}/CMakeLists.txt")
  add_subdirectory(${GOOGLETEST_DIR})
else()
  # SPDLOG_DIR NOT EXISTS
  message(FATAL_ERROR "Google test is not exit.")
endif()

if(NOT RUNTIME_JUST_LIBRARY)
  add_definitions(-DRUNTIME_DEBUG);
endif()

file(GLOB_RECURSE SOURCES "${COMMON_SOURCE_DIR}/*.cpp" "${GPU_SOURCE_DIR}/*.cpp" "${CPU_SOURCE_DIR}/*.cpp")
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
  message(STATUS "Building the library and also all tests.")
  file(GLOB_RECURSE TEST_SOURCES "${TEST_SOURCE_DIR}/*/*.cpp")
  add_executable(runtime-gtest ${TEST_SOURCE_DIR}/main.cpp ${TEST_SOURCES})
  target_include_directories(runtime-gtest PRIVATE ${GOOGLETEST_DIR}/googletest/include)
  target_link_libraries(runtime-gtest ${MODULE_NAME} gtest)
endif()
