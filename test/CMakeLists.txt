cmake_minimum_required(VERSION 3.5)
project(sweet_osal_test)

set(CMAKE_CXX_STANDARD 11)

set(SOP_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

include(${SOP_TOP_DIR}/sop_src/LibraryFiles.cmake)
add_definitions(-DRUN_GTEST)

if (WIN32)
elseif (APPLE)
    set(CMAKE_BUILD_TYPE Debug)
elseif (UNIX)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

if (FREERTOS_PORT)
    add_definitions(-D__FREERTOS__ )
endif(FREERTOS_PORT)

add_definitions(-DMBEDTLS_CONFIG_FILE="sop_src/mbedtls/myconfig.h")

file(GLOB TEST_SRC
        ${SOP_TOP_DIR}/test/*.cpp
        ${SOP_TOP_DIR}/test/*.c
        ${SOP_TOP_DIR}/test/*.h
        ${SOP_TOP_DIR}/test/*.hpp
)

file(GLOB LOCAL_SRC
        ${SOP_COMMON_SRC}/LibraryFiles.cmake
)

set(SOURCE_FILES
        ${OSAL_SRC}

        ${MBEDTLS_BASICS_SRC}

        ${LIBSODIUM_SRC}

        ${TASK_SCHED_SRC}

        ${BUF_IO_SRC}
        
        ${LOCAL_SRC}

        ${UTILS_SRC}

        ${TEST_SRC}

        ${MINI_SOCKET_SRC}

        ${GTEST_SRC}

)

include_directories(
       .
       ${SAU_COMMON_SRC}
       ${SAU_TOP_DIR}

)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (EMSCRIPTEN)
  #Skip threads.
elseif (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
else ()
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif ()

enable_testing()
add_test(${PROJECT_NAME} ${PROJECT_NAME})
