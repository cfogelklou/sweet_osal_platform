cmake_minimum_required(VERSION 3.5)
project(transport_test)

set(CMAKE_CXX_STANDARD 11)

set(SOP_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

include(${SOP_TOP_DIR}/sop_src/LibraryFiles.cmake)

add_definitions(-DMEMPOOLS_DEBUG=1 -DNO_MEMPOOLS=0)

if (WIN32)
    add_definitions(-DMBEDTLS_NO_SOCKETS)
elseif (APPLE)
    set(CMAKE_BUILD_TYPE Debug)
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")    

elseif (UNIX)
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c11")

    #TODO the following definition is only needed on BASH under windows.
    add_definitions(-DMBEDTLS_NO_SOCKETS)
endif()

file(GLOB LOCAL_SRC
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

       ${SOP_EXTERN_LIBS}/googletest/googletest
       ${SOP_EXTERN_LIBS}/googletest/googletest/include
       ${SOP_EXTERN_LIBS}/googletest/googlemock
       ${SOP_EXTERN_LIBS}/googletest/googlemock/include
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
