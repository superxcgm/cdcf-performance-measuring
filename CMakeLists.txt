cmake_minimum_required(VERSION 3.10)
project(cdcf_performance_measuring)

INCLUDE_DIRECTORIES("/usr/local/include")

set(CMAKE_CXX_STANDARD 17)
set(CONAN_DISABLE_CHECK_COMPILER 1)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/build)
if ("${CONAN_COMPILER}" STREQUAL "gcc")
    set(CMAKE_C_FLAGS ${CONAN_C_FLAGS})
    set(CMAKE_CXX_FLAGS ${CONAN_CXX_FLAGS})
    set(CMAKE_SHARED_LINKER_FLAGS "${CONAN_SHARED_LINKER_FLAGS}")
endif ()

find_package(caf REQUIRED)
find_package(cdcf REQUIRED)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

conan_basic_setup()

add_executable(${PROJECT_NAME} src/main.cc src/circle_barrier.cc)
target_link_libraries(${PROJECT_NAME} caf::caf cdcf::cdcf )