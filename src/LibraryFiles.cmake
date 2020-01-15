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

file(GLOB MBEDTLS_SRC
  ${SOP_EXTERN_LIBS}/mbedtls/library/*.c
  ${SOP_EXTERN_LIBS}/mbedtls/include/mbedtls/*.h
  ${SOP_EXTERN_LIBS}/mbedtls/library/ed25519/*.c
  ${SOP_EXTERN_LIBS}/mbedtls/library/ed25519/*.h
  ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/*.c
  ${SOP_TOP_DIR}/src/mbedtls/*.cpp
  ${SOP_TOP_DIR}/src/mbedtls/*.c
)

list(REMOVE_ITEM MBEDTLS_SRC ${SOP_EXTERN_LIBS}/mbedtls/crypto/library/version.c)
list(REMOVE_ITEM MBEDTLS_SRC ${SOP_EXTERN_LIBS}/mbedtls/library/error.c)
list(REMOVE_ITEM MBEDTLS_SRC ${SOP_EXTERN_LIBS}/mbedtls/library/version_features.c)


file(GLOB TASK_SCHED_SRC
        ${SOP_TOP_DIR}/src/task_sched/*.cpp
        ${SOP_TOP_DIR}/src/task_sched/*.c
        ${SOP_TOP_DIR}/src/task_sched/*.h
        ${SOP_TOP_DIR}/src/task_sched/*.hpp
)

file(GLOB OSAL_SRC
        ${SOP_TOP_DIR}/src/osal/*.cpp
        ${SOP_TOP_DIR}/src/osal/*.c
        ${SOP_TOP_DIR}/src/osal/*.h
        ${SOP_TOP_DIR}/src/osal/*.hpp
)

file(GLOB BUF_IO_SRC
        ${SOP_TOP_DIR}/src/buf_io/*.cpp
        ${SOP_TOP_DIR}/src/buf_io/*.c
        ${SOP_TOP_DIR}/src/buf_io/*.h
        ${SOP_TOP_DIR}/src/buf_io/*.hpp
)

file(GLOB UTILS_SRC
        ${SOP_TOP_DIR}/src/utils/*.cpp
        ${SOP_TOP_DIR}/src/utils/*.c
        ${SOP_TOP_DIR}/src/utils/*.h
        ${SOP_TOP_DIR}/src/utils/*.hpp
)


file(GLOB MINI_SOCKET_SRC
        ${SOP_TOP_DIR}/src/mini_socket/*.c
        ${SOP_TOP_DIR}/src/mini_socket/*.cpp
        ${SOP_TOP_DIR}/src/mini_socket/*.h
        ${SOP_TOP_DIR}/src/mini_socket/*.hpp
)

set(GTEST_SRC
        ${SOP_TOP_DIR}/ext/googletest/googletest/src/gtest-all.cc
        ${SOP_TOP_DIR}/ext/googletest/googlemock/src/gmock-all.cc
)
