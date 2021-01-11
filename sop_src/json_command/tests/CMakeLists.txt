cmake_minimum_required(VERSION 3.5)
project(json_commands_test)

set(CMAKE_CXX_STANDARD 11)

if(NOT DEFINED SOP_TOP_DIR)
    set(SOP_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
endif()

if (WIN32)
  add_definitions(-DMSVC)
endif()

include(${SOP_TOP_DIR}/sop_src/LibraryFiles.cmake)

add_definitions(-DSODIUM_STATIC -DDEBUG -D_CONSOLE -DNATIVE_LITTLE_ENDIAN)
#add_definitions(-DED25519_COMPAT)
if (WIN32)
elseif (APPLE)
    set(CMAKE_BUILD_TYPE Debug)
elseif (UNIX)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

file(GLOB LOCAL_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../*.hpp
)



set(SOURCE_FILES
    # Google Test / Mock
    ${GTEST_SRC}

    ${SOP_SRC}

    ${LOCAL_SRC}

    ${SOP_TOP_DIR}/sop_src/LibraryFiles.cmake
)

list(REMOVE_DUPLICATES SOURCE_FILES)

list(REMOVE_DUPLICATES SOURCE_FILES)

include_directories(
#       ${SOP_TOP_DIR}

#       ${SOP_COMMON_SRC}

#       ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/include/sodium

#       ${SOP_COMMON_SRC}/mbedtls
#       ${SOP_EXTERN_LIBS}/mbedtls/include/mbedtls
#       ${SOP_EXTERN_LIBS}/mbedtls/crypto/include
#       ${SOP_EXTERN_LIBS}/mbedtls/include

#       ${SOP_EXTERN_LIBS}/googletest/googletest
#       ${SOP_EXTERN_LIBS}/googletest/googletest/include
#       ${SOP_EXTERN_LIBS}/googletest/googlemock
#       ${SOP_EXTERN_LIBS}/googletest/googlemock/include
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(Threads REQUIRED)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32 crypt32 )
else (WIN32)
            
    target_link_libraries(${PROJECT_NAME} Threads::Threads )
endif (WIN32)

enable_testing()
add_test(${PROJECT_NAME} ${PROJECT_NAME})
