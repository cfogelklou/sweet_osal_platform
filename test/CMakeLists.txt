cmake_minimum_required(VERSION 3.5)
project(audio_test)

set(CMAKE_CXX_STANDARD 11)

set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(EXTERN_LIBS ${TOP_DIR}/ext)
set(COMMON_SRC ${TOP_DIR}/src)


add_definitions(-DSODIUM_STATIC -DDEBUG -D_CONSOLE -DNATIVE_LITTLE_ENDIAN -DMBEDTLS_CONFIG_FILE="src/mbedtls/myconfig.h")
#add_definitions(-DED25519_COMPAT)



if (WIN32)
    #set(FREERTOS_PORT 1)
    add_definitions(-DWIN32)
elseif (APPLE)
    add_definitions(-DAPPLE=1 -D__APPLE__=1 -DTARGET_OS_OSX=1)
    set(CMAKE_BUILD_TYPE Debug)
elseif (UNIX)
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

include(${COMMON_SRC}/LibraryFiles.cmake)

file(GLOB TEST_SRC
        ${TOP_DIR}/test/*.cpp
        ${TOP_DIR}/test/*.c
        ${TOP_DIR}/test/*.h
        ${TOP_DIR}/test/*.hpp
)

file(GLOB LOCAL_SRC
        ${TOP_DIR}/src/LibraryFiles.cmake
        ${TOP_DIR}/src/*.c
        ${TOP_DIR}/src/*.cpp
        ${TOP_DIR}/src/*.h
        ${TOP_DIR}/src/*.hpp
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

        ${TONE_GEN_SRC}
        ${TONE_DET_SRC}
        ${MINI_SOCKET_SRC}

        ${GTEST_SRC}

        ${AUDIO_SRC}
)

include_directories(
       .
       ${TOP_DIR}/src/mbedtls
       ${TOP_DIR}/src
       # Google test (the real thing)
       ${TOP_DIR}/ext/googletest/googletest
       ${TOP_DIR}/ext/googletest/googletest/include
       ${TOP_DIR}/ext/googletest/googlemock
       ${TOP_DIR}/ext/googletest/googlemock/include
       
       ${TOP_DIR}/ext/mbedtls/include/mbedtls
       ${TOP_DIR}/ext/mbedtls/crypto/include
       ${EXTERN_LIBS}/mbedtls/include
       ${EXTERN_LIBS}/libsodium/src/libsodium
       ${EXTERN_LIBS}/libsodium/src/libsodium/include/sodium

       ${TOP_DIR}
       ${TOP_DIR}/src
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
