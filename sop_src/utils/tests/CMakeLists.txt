cmake_minimum_required(VERSION 3.5)
project(utils_test)

set(CMAKE_CXX_STANDARD 11)

set(SOP_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

include(${SOP_TOP_DIR}/sop_src/LibraryFiles.cmake)

add_definitions(-DMEMPOOLS_DEBUG=1 -DNO_MEMPOOLS=0)


file(GLOB LOCAL_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

set(SOURCE_FILES
    # SOP src
    ${MBEDTLS_BASICS_SRC}
    ${TASK_SCHED_SRC}
    ${OSAL_SRC}
    ${BUF_IO_SRC}
    ${UTILS_SRC}

    ${GTEST_SRC}
    ${SOP_COMMON_SRC}/LibraryFiles.cmake

    # These main files.
    ${LOCAL_SRC}    
)

list(REMOVE_DUPLICATES SOURCE_FILES)

add_definitions(-DRUN_GTEST=1)

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
if (EMSCRIPTEN)
elseif (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
else (EMSCRIPTEN)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif (EMSCRIPTEN)

enable_testing()
add_test(${PROJECT_NAME} ${PROJECT_NAME})
