cmake_minimum_required(VERSION 3.5)

enable_testing()

if(NOT DEFINED SOP_TOP_DIR)
    set(SOP_TOP_DIR ${CMAKE_SOURCE_DIR}/..)
endif()

add_subdirectory(osal/test)

#if (LINUX OR WIN32)
#  if (NOT MINGW)
#    add_subdirectory(tests/freertos/tests)
#  endif(NOT MINGW)
#endif(LINUX OR WIN32)

#if(NOT MSVC)
#  add_subdirectory(ecu)
#endif()
