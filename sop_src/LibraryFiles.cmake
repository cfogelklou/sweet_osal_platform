
set(SOP_EXTERN_LIBS ${SOP_TOP_DIR}/ext)
set(SOP_COMMON_SRC ${SOP_TOP_DIR}/sop_src)

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

if (WIN32)
    #set(FREERTOS_PORT 1)
    add_definitions(-DWIN32)
    add_definitions(-DNATIVE_LITTLE_ENDIAN)
elseif (APPLE)
    add_definitions(-DAPPLE=1 -D__APPLE__=1 -DTARGET_OS_OSX=1)
    add_definitions(-DNATIVE_LITTLE_ENDIAN)
elseif (UNIX)
    add_definitions(-DNATIVE_LITTLE_ENDIAN)
elseif (EMSCRIPTEN)
    add_definitions(-DNATIVE_LITTLE_ENDIAN)    
endif ()

if (EMSCRIPTEN)
  add_definitions(-DEMSCRIPTEN -D__EMSCRIPTEN__ -DRANDOMBYTES_CUSTOM_IMPLEMENTATION)
endif(EMSCRIPTEN)

add_definitions(-DSODIUM_STATIC -DDEV_MODE -DCONFIGURED=1 -DDEBUG -D_CONSOLE)
add_definitions(-DMBEDTLS_CONFIG_FILE="sop_src/mbedtls/myconfig.h")

include_directories(
       .
       ${SOP_COMMON_SRC}

       # Google test
       ${SOP_TOP_DIR}/ext/googletest/googletest
       ${SOP_TOP_DIR}/ext/googletest/googletest/include
       ${SOP_TOP_DIR}/ext/googletest/googlemock
       ${SOP_TOP_DIR}/ext/googletest/googlemock/include
	   
	   #json
	   ${SOP_TOP_DIR}/ext/json/include
       
       ${SOP_COMMON_SRC}/mbedtls
       ${SOP_EXTERN_LIBS}/mbedtls/include/mbedtls
       ${SOP_EXTERN_LIBS}/mbedtls/crypto/include
       ${SOP_EXTERN_LIBS}/mbedtls/include

       ${SOP_EXTERN_LIBS}/libsodium/src/libsodium
       ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/include/sodium

       ${SOP_TOP_DIR}
)


# USE recursive search to add all libsodium files.
file(GLOB_RECURSE LIBSODIUM_SRC
  ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/*.c
  ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/*.h
  ${SOP_COMMON_SRC}/libsodium/*.c
  ${SOP_COMMON_SRC}/libsodium/*.cpp
  ${SOP_COMMON_SRC}/libsodium/*.hpp
)

list(REMOVE_ITEM LIBSODIUM_SRC ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/sodium/version.c)
list(REMOVE_ITEM LIBSODIUM_SRC ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-ref.c)
list(REMOVE_ITEM LIBSODIUM_SRC ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/crypto_aead/aegis256/aesni/aead_aegis256_aesni.c)
#list(REMOVE_ITEM LIBSODIUM_SRC ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/sodium/core.c)
if (EMSCRIPTEN)
list(REMOVE_ITEM LIBSODIUM_SRC ${SOP_EXTERN_LIBS}/libsodium/src/libsodium/randombytes/randombytes.c)
endif()

file(GLOB MBEDTLS_BASICS_SRC
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/sha1.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/sha256.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/sha512.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/platform_util.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/base64.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/entropy.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/entropy_poll.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/threading.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/timing.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/memory_buffer_alloc.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/platform.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/hmac_drbg.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/md.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/md2.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/md4.c
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/md5.c
  ${SOP_COMMON_SRC}/mbedtls/*.cpp
  ${SOP_COMMON_SRC}/mbedtls/*.c
)
                                             
file(GLOB MBEDTLS_SRC
  ${SOP_EXTERN_LIBS}/mbedtls/library/*.c
  ${SOP_EXTERN_LIBS}/mbedtls/include/mbedtls/*.h
  ${SOP_EXTERN_LIBS}/mbedtls/library/ed25519/*.c
  ${SOP_EXTERN_LIBS}/mbedtls/library/ed25519/*.h
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/*.c
  ${SOP_COMMON_SRC}/mbedtls/*.cpp
  ${SOP_COMMON_SRC}/mbedtls/*.c
)
                                             
                                             

list(REMOVE_ITEM MBEDTLS_SRC ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/version.c)
list(REMOVE_ITEM MBEDTLS_SRC ${SOP_EXTERN_LIBS}/mbedtls/library/error.c)
list(REMOVE_ITEM MBEDTLS_SRC ${SOP_EXTERN_LIBS}/mbedtls/library/version_features.c)


file(GLOB TASK_SCHED_SRC
        ${SOP_COMMON_SRC}/task_sched/*.cpp
        ${SOP_COMMON_SRC}/task_sched/*.c
        ${SOP_COMMON_SRC}/task_sched/*.h
        ${SOP_COMMON_SRC}/task_sched/*.hpp
)

file(GLOB OSAL_SRC
        ${SOP_COMMON_SRC}/osal/*.cpp
        ${SOP_COMMON_SRC}/osal/*.c
        ${SOP_COMMON_SRC}/osal/*.h
        ${SOP_COMMON_SRC}/osal/*.hpp
)

file(GLOB BUF_IO_SRC
        ${SOP_COMMON_SRC}/buf_io/*.cpp
        ${SOP_COMMON_SRC}/buf_io/*.c
        ${SOP_COMMON_SRC}/buf_io/*.h
        ${SOP_COMMON_SRC}/buf_io/*.hpp
)

file(GLOB UTILS_SRC
        ${SOP_COMMON_SRC}/utils/*.cpp
        ${SOP_COMMON_SRC}/utils/*.c
        ${SOP_COMMON_SRC}/utils/*.h
        ${SOP_COMMON_SRC}/utils/*.hpp
)

file(GLOB PHONE_AL_SRC
        ${SOP_COMMON_SRC}/phone_al/*.cpp
        ${SOP_COMMON_SRC}/phone_al/*.c
        ${SOP_COMMON_SRC}/phone_al/*.h
        ${SOP_COMMON_SRC}/phone_al/*.hpp
)

file(GLOB MINI_SOCKET_SRC
        ${SOP_COMMON_SRC}/mini_socket/*.c
        ${SOP_COMMON_SRC}/mini_socket/*.cpp
        ${SOP_COMMON_SRC}/mini_socket/*.h
        ${SOP_COMMON_SRC}/mini_socket/*.hpp
)

file(GLOB SIMPLE_PLOT_SRC
        ${SOP_COMMON_SRC}/simple_plot/*.c
        ${SOP_COMMON_SRC}/simple_plot/*.cpp
        ${SOP_COMMON_SRC}/simple_plot/*.h
        ${SOP_COMMON_SRC}/simple_plot/*.hpp
)

set(GTEST_SRC
        ${SOP_TOP_DIR}/ext/googletest/googletest/src/gtest-all.cc
        ${SOP_TOP_DIR}/ext/googletest/googlemock/src/gmock-all.cc
)

set(SOP_SRC
    ${LIBSODIUM_SRC}
    ${MBEDTLS_BASICS_SRC}
    ${TASK_SCHED_SRC}
    ${OSAL_SRC}
    ${PHONE_AL_SRC}
    ${BUF_IO_SRC}
    ${UTILS_SRC}
    ${MINI_SOCKET_SRC}
    ${SIMPLE_PLOT_SRC}
    ${SOP_COMMON_SRC}/LibraryFiles.cmake
)

list(REMOVE_DUPLICATES SOP_SRC)


