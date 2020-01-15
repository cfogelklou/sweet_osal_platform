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

add_definitions(-DMBEDTLS_CONFIG_FILE="sop_src/mbedtls/myconfig.h")

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


file(GLOB MINI_SOCKET_SRC
        ${SOP_COMMON_SRC}/mini_socket/*.c
        ${SOP_COMMON_SRC}/mini_socket/*.cpp
        ${SOP_COMMON_SRC}/mini_socket/*.h
        ${SOP_COMMON_SRC}/mini_socket/*.hpp
)

set(GTEST_SRC
        ${SOP_TOP_DIR}/ext/googletest/googletest/src/gtest-all.cc
        ${SOP_TOP_DIR}/ext/googletest/googlemock/src/gmock-all.cc
)
