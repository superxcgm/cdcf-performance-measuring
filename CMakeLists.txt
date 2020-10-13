cmake_minimum_required(VERSION 3.10)
project(cdcf_performance_measuring)

INCLUDE_DIRECTORIES("/usr/local/include")
INCLUDE_DIRECTORIES("/usr/local/include/hdr")
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

message("----------------------")
message("--------  ${PROJECT_SOURCE_DIR}/lib  ------")
message("----------------------")

#link_directories("${PROJECT_SOURCE_DIR}/lib")
#LINK_LIBRARIES("${PROJECT_SOURCE_DIR}/lib/libhdr_histogram_static.a")


#find_package(Threads)
#FIND_LIBRARY(RUNTIME_LIB hdr_histogram_static ${PROJECT_SOURCE_DIR}/lib NO_DEFAULT_PATH)
#message("----------------------")
#message("--------  ${RUNTIME_LIB}  ------")
#message("----------------------")

#INCLUDE_DIRECTORIES("/Users/yuecheng.pei/Desktop/rep/lib/HdrHistogram_c/src")
#set_property(TARGET ${PROJECT_NAME} PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/lib/libhdr_histogram_static.a)

find_package(hdr_histogram 0.10.0)
add_executable(${PROJECT_NAME} src/main.cc src/circle_barrier.cc)
target_link_libraries(${PROJECT_NAME} caf::caf cdcf::cdcf hdr_histogram::hdr_histogram)