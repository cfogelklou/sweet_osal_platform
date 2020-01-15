cmake_minimum_required(VERSION 3.5)
project(transport_test)

set(CMAKE_CXX_STANDARD 11)



if(NOT DEFINED APPLICAUDIA_LIB_DIR)
    set(APPLICAUDIA_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
endif()
include(${APPLICAUDIA_LIB_DIR}/Applicaudia.cmake)
include(${ROOT_SRC}/LibraryFiles.cmake)

add_definitions(-DMEMPOOLS_DEBUG=1 -DNO_MEMPOOLS=0)

if (WIN32)
elseif (UNIX)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99")

#TODO the following definition is only needed on BASH under windows.
add_definitions(-DMBEDTLS_NO_SOCKETS)
endif()


file(GLOB UTILS_SRC
        ${SOP_COMMON_SRC}/utils/*.cpp
        ${SOP_COMMON_SRC}/utils/*.c
        ${SOP_COMMON_SRC}/utils/*.h
        ${SOP_COMMON_SRC}/utils/*.hpp
)

file(GLOB LOCAL_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

set(GTEST_SRC
        ${APPLICAUDIA_LIB_DIR}/extern_libs/googletest/googletest/src/gtest-all.cc
        ${APPLICAUDIA_LIB_DIR}/extern_libs/googletest/googlemock/src/gmock-all.cc
)

set(SOURCE_FILES
        ${PLATFORM_SRC}
        ${RTC_EMUL_SRC}

        ${ALL_CRYPTO_SRC}

        ${GTEST_SRC}

        ${BLEIOQ_BASE_SRC}

        ${UTILS_SRC}

        ${LOCAL_SRC}
)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${SOP_EXTERN_LIBS}/../
        ${APPLICAUDIA_LIB_DIR}/src
        ${APPLICAUDIA_LIB_DIR}/src/common_src
        ${ROOT_SRC}
        ${SOP_COMMON_SRC}
        ${SOP_COMMON_SRC}/mbedtls
        ${SOP_COMMON_SRC}/acp
        ${SOP_COMMON_SRC}/libsodium
        ${SOP_COMMON_SRC}/cifra
        ${PROJECTS}/testapp/freertos/portable/MSVC-MingW
        ${FREERTOS_DIR}/include
        ${SOP_EXTERN_LIBS}/mbedtls/include/mbedtls
        ${SOP_EXTERN_LIBS}/mbedtls/include
        ${SOP_EXTERN_LIBS}
        ${SOP_EXTERN_LIBS}/json/src
        ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/include/sodium
        # Google test (the real thing)
        ${SOP_EXTERN_LIBS}/googletest/googletest
        ${SOP_EXTERN_LIBS}/googletest/googletest/include
        ${APPLICAUDIA_LIB_DIR}/extern_libs/googletest/googlemock
        ${APPLICAUDIA_LIB_DIR}/extern_libs/googletest/googlemock/include
)

if (WIN32)
include_directories(
       ${PLATFORM}/pc_port/windows
 )
endif (WIN32)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
if (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
else (WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif (WIN32)

enable_testing()
add_test(${PROJECT_NAME} ${PROJECT_NAME})
