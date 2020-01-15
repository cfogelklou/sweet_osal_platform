cmake_minimum_required(VERSION 3.5)
project(freertos_test)

set(CMAKE_CXX_STANDARD 11)

if(NOT DEFINED SOP_TOP_DIR)
    set(SOP_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
endif()

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

include(${SOP_TOP_DIR}/sop_src/LibraryFiles.cmake)

if (FREERTOS_PORT)
    set(FREERTOS_DIR ${ROOT_SRC}/ecu/pak_spc564b/components/spc5_freertos_component_rla/lib)

    file(GLOB FREERTOS_SRC
        ${SOP_COMMON_SRC}/tests/freertos/portable/MSVC-MingW/*.c
        ${FREERTOS_DIR}/src/*.c
    ) 

endif(FREERTOS_PORT)

file(GLOB TESTS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
    ${SOP_COMMON_SRC}/osal/test/test_osal.cpp
    ${SOP_COMMON_SRC}/osal/test/test_mempools.cpp
    ${SOP_COMMON_SRC}/osal/test/test_random.cpp
    
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

       ${ROOT_SRC}/tests/freertos/portable/MSVC-MingW
       ${ROOT_SRC}/tests/freertos
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
