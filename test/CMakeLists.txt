cmake_minimum_required(VERSION 3.5)
project(sweet_osal_test)

set(CMAKE_CXX_STANDARD 11)

set(SOP_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SOP_EXTERN_LIBS ${SOP_TOP_DIR}/ext)
set(SOP_COMMON_SRC ${SOP_TOP_DIR}/src)

add_definitions(-DSODIUM_STATIC -DDEBUG -D_CONSOLE -DMBEDTLS_CONFIG_FILE="src/mbedtls/myconfig.h")

if (WIN32)
    #set(FREERTOS_PORT 1)
    add_definitions(-DWIN32)
    add_definitions(-DNATIVE_LITTLE_ENDIAN)
elseif (APPLE)
    add_definitions(-DAPPLE=1 -D__APPLE__=1 -DTARGET_OS_OSX=1)
    add_definitions(-DNATIVE_LITTLE_ENDIAN)
    set(CMAKE_BUILD_TYPE Debug)
elseif (UNIX)
    add_definitions(-DNATIVE_LITTLE_ENDIAN)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

if (FREERTOS_PORT)
    add_definitions(-D__FREERTOS__ )
endif(FREERTOS_PORT)

#defines we need to build libsodium
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  #For MSVC less than or equal to 10.0, "inline" doesn't exist.
  if (MSVC_VERSION)
    if (MSVC_VERSION LESS 1601)
      add_definitions(-Dinline=__inline)
    endif ()
  endif ()
  add_definitions("/wd4146 /wd4244 /wd4996 -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
endif ()

add_definitions(-DMBEDTLS_CONFIG_FILE="src/mbedtls/myconfig.h")

include(${SOP_COMMON_SRC}/LibraryFiles.cmake)

file(GLOB TEST_SRC
        ${SOP_TOP_DIR}/test/*.cpp
        ${SOP_TOP_DIR}/test/*.c
        ${SOP_TOP_DIR}/test/*.h
        ${SOP_TOP_DIR}/test/*.hpp
)

file(GLOB LOCAL_SRC
        ${SOP_TOP_DIR}/src/LibraryFiles.cmake
        ${SOP_TOP_DIR}/src/*.c
        ${SOP_TOP_DIR}/src/*.cpp
        ${SOP_TOP_DIR}/src/*.h
        ${SOP_TOP_DIR}/src/*.hpp
)

set(SOURCE_FILES
        ${OSAL_SRC}

        ${MBEDTLS_SRC}

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
       ${SOP_TOP_DIR}/src/mbedtls
       ${SOP_TOP_DIR}/src

       # Google test
       ${SOP_TOP_DIR}/ext/googletest/googletest
       ${SOP_TOP_DIR}/ext/googletest/googletest/include
       ${SOP_TOP_DIR}/ext/googletest/googlemock
       ${SOP_TOP_DIR}/ext/googletest/googlemock/include
       
       ${SOP_TOP_DIR}/ext/mbedtls/include/mbedtls
       ${SOP_TOP_DIR}/ext/mbedtls/crypto/include
       ${SOP_EXTERN_LIBS}/mbedtls/include
       ${SOP_EXTERN_LIBS}/libsodium/src/libsodium
       ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/include/sodium

       ${SOP_TOP_DIR}
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# target_link_libraries(${PROJECT_NAME} curl)
if (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
else (WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif (WIN32)

enable_testing()
add_test(${PROJECT_NAME} ${PROJECT_NAME})
