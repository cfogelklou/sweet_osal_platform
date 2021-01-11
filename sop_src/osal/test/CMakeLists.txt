cmake_minimum_required(VERSION 3.5)
project(osal_test)

set(CMAKE_CXX_STANDARD 11)

set(SOP_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)



include(${SOP_TOP_DIR}/sop_src/LibraryFiles.cmake)
add_definitions(-DRUN_GTEST)

if (WIN32)
    #set(FREERTOS_PORT 1)
elseif (APPLE)
    set(CMAKE_BUILD_TYPE Debug)
elseif (UNIX)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

file(GLOB LOCAL_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp

)

set(SOURCE_FILES

        # Google Test / Mock
        ${GTEST_SRC}

        ${SOP_SRC}

        # These main files.
        ${LOCAL_SRC}

)

list(REMOVE_DUPLICATES SOURCE_FILES)

include_directories(
       ${SOP_TOP_DIR}

       ${SOP_COMMON_SRC}

       ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/include/sodium

       ${SOP_COMMON_SRC}/mbedtls
       ${SOP_EXTERN_LIBS}/mbedtls/include/mbedtls
       ${SOP_EXTERN_LIBS}/mbedtls/crypto/include
       ${SOP_EXTERN_LIBS}/mbedtls/include

       ${SOP_EXTERN_LIBS}/cifra/src
       ${SOP_EXTERN_LIBS}/cifra/src/ext
       ${SOP_EXTERN_LIBS}/googletest/googletest
       ${SOP_EXTERN_LIBS}/googletest/googletest/include
       ${SOP_EXTERN_LIBS}/googletest/googlemock
       ${SOP_EXTERN_LIBS}/googletest/googlemock/include
       ${SOP_COMMON_SRC}/osal
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (EMSCRIPTEN)
  #Skip threads.
elseif (WIN32)
  target_link_libraries(${PROJECT_NAME} ws2_32)
else (WIN32)
  find_package(Threads REQUIRED)    
  target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif()

enable_testing()
add_test(${PROJECT_NAME} ${PROJECT_NAME})
