cmake_minimum_required(VERSION 3.16)
project(clDsp C)

set(CMAKE_C_STANDARD 99)

add_library(clDsp ${CMAKE_CURRENT_SOURCE_DIR}/src/fir_filter.c ${CMAKE_CURRENT_SOURCE_DIR}/src/fir_filter_naive.c ${CMAKE_CURRENT_SOURCE_DIR}/src/fir_filter_volk.c ${CMAKE_CURRENT_SOURCE_DIR}/src/fir_filter_float8.c ${CMAKE_CURRENT_SOURCE_DIR}/src/fir_filter_memmap.c)

find_package(PkgConfig REQUIRED)

pkg_check_modules(PC_VOLK REQUIRED volk)
include_directories(${PC_VOLK_INCLUDE_DIRS})
target_link_libraries(clDsp ${PC_VOLK_LINK_LIBRARIES})

pkg_check_modules(PC_OpenCL REQUIRED OpenCL)
include_directories(${PC_OpenCL_INCLUDE_DIRS})
target_link_libraries(clDsp ${PC_OpenCL_LINK_LIBRARIES})

find_package(Threads REQUIRED)
target_link_libraries(clDsp ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(clDsp m)

enable_testing()

file(GLOB TEST_RESOURCES test/resources/*)
file(COPY ${TEST_RESOURCES} DESTINATION "${CMAKE_BINARY_DIR}")
file(GLOB PERF_SOURCES test/perf_*.c)
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test/test_*.c)
pkg_check_modules(PC_CHECK REQUIRED check)
include_directories(${PC_CHECK_INCLUDE_DIRS})

foreach (curTest ${TEST_SOURCES})
    get_filename_component(curTestName ${curTest} NAME_WE)
    add_test(NAME ${curTestName} COMMAND ${curTestName} ${curTest})
    add_executable(${curTestName} ${curTest})
    target_link_libraries(${curTestName} clDsp ${PC_CHECK_LINK_LIBRARIES})
endforeach ()

foreach(curPerfTest ${PERF_SOURCES})
    get_filename_component(curPerfTestName ${curPerfTest} NAME_WE)
    add_executable(${curPerfTestName} ${curPerfTest})
    target_link_libraries(${curPerfTestName} clDsp ${PC_CHECK_LINK_LIBRARIES})
endforeach()