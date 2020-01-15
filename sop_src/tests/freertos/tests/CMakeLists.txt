cmake_minimum_required(VERSION 3.5)
project(freertos_test)

set(CMAKE_CXX_STANDARD 11)

if(NOT DEFINED POLESTAR_PAK_DIR)
    set(POLESTAR_PAK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
endif()
include(${POLESTAR_PAK_DIR}/Applicaudia.cmake)

if (WIN32)
    set(FREERTOS_PORT 1)
    add_definitions(-DWIN32)
else ()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif ()

if (FREERTOS_PORT)
    add_definitions(-D__FREERTOS__ )
endif(FREERTOS_PORT)

include(${ROOT_SRC}/LibraryFiles.cmake)


set(GTEST_SRC
        ${POLESTAR_PAK_DIR}/extern_libs/googletest/googletest/src/gtest-all.cc
        ${POLESTAR_PAK_DIR}/extern_libs/googletest/googlemock/src/gmock-all.cc
)

if (FREERTOS_PORT)
    set(FREERTOS_DIR ${ROOT_SRC}/ecu/pak_spc564b/components/spc5_freertos_component_rla/lib)

    file(GLOB FREERTOS_SRC
        ${ROOT_SRC}/tests/freertos/portable/MSVC-MingW/*.c
        ${FREERTOS_DIR}/src/*.c
    ) 

endif(FREERTOS_PORT)

file(GLOB TESTS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
    ${ROOT_SRC}/tests/embedded_tests/test_osal.cpp
    ${ROOT_SRC}/tests/embedded_tests/test_mempools.cpp
    ${ROOT_SRC}/tests/embedded_tests/test_random.cpp
    
)


set(SOURCE_FILES
        ${PLATFORM_SRC}

        # FreeRTOS
        ${FREERTOS_SRC}

        ${GTEST_SRC}

        ${BLEIOQ_BASE_SRC}

        ${UTILS_SRC}

        ${TESTS_SRC}
)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${EXTERN_LIBS}/../
        ${POLESTAR_PAK_DIR}/src
        ${POLESTAR_PAK_DIR}/src/common_src
        ${ROOT_SRC}
        ${EXTERN_LIBS}/libsodium/src/libsodium/include/sodium
        ${COMMON_SRC}
        ${COMMON_SRC}/mbedtls
        ${ROOT_SRC}/tests/freertos/portable/MSVC-MingW
        ${ROOT_SRC}/tests/freertos
        ${FREERTOS_DIR}/include
        ${EXTERN_LIBS}/mbedtls/include/mbedtls
        ${EXTERN_LIBS}/mbedtls/include
        ${EXTERN_LIBS}
        ${EXTERN_LIBS}/json/src
        # Google test (the real thing)
        ${EXTERN_LIBS}/googletest/googletest
        ${EXTERN_LIBS}/googletest/googletest/include
        ${POLESTAR_PAK_DIR}/extern_libs/googletest/googlemock
        ${POLESTAR_PAK_DIR}/extern_libs/googletest/googlemock/include
)

include_directories(
    ${COMMON_SRC}/cifra    
)

if (WIN32)
include_directories(
       ${PLATFORM}/pc_port/windows
 )
endif (WIN32)


add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (WIN32)

    if (MINGW)
        target_link_libraries(${PROJECT_NAME} winmm ws2_32)
    else(WINGW)
        target_link_libraries(${PROJECT_NAME} ws2_32)
    endif(MINGW)

else (WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif (WIN32)

enable_testing()
add_test(${PROJECT_NAME} ${PROJECT_NAME})
