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
  ${EXTERN_LIBS}/libsodium/src/libsodium/*.c
  ${EXTERN_LIBS}/libsodium/src/libsodium/*.h
  ${COMMON_SRC}/libsodium/*.c
  ${COMMON_SRC}/libsodium/*.cpp
  ${COMMON_SRC}/libsodium/*.hpp
)

list(REMOVE_ITEM LIBSODIUM_SRC ${EXTERN_LIBS}/libsodium/src/libsodium/sodium/version.c)
list(REMOVE_ITEM LIBSODIUM_SRC ${EXTERN_LIBS}/libsodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-ref.c)
list(REMOVE_ITEM LIBSODIUM_SRC ${EXTERN_LIBS}/libsodium/src/libsodium/crypto_aead/aegis256/aesni/aead_aegis256_aesni.c)
#list(REMOVE_ITEM LIBSODIUM_SRC ${EXTERN_LIBS}/libsodium/src/libsodium/sodium/core.c)

file(GLOB MBEDTLS_SRC
  ${EXTERN_LIBS}/mbedtls/library/*.c
  ${EXTERN_LIBS}/mbedtls/include/mbedtls/*.h
  ${EXTERN_LIBS}/mbedtls/library/ed25519/*.c
  ${EXTERN_LIBS}/mbedtls/library/ed25519/*.h
  ${EXTERN_LIBS}/mbedtls/crypto/library/*.c
  ${TOP_DIR}/src/mbedtls/*.cpp
  ${TOP_DIR}/src/mbedtls/*.c
)

list(REMOVE_ITEM MBEDTLS_SRC ${EXTERN_LIBS}/mbedtls/crypto/library/version.c)
list(REMOVE_ITEM MBEDTLS_SRC ${EXTERN_LIBS}/mbedtls/library/error.c)
list(REMOVE_ITEM MBEDTLS_SRC ${EXTERN_LIBS}/mbedtls/library/version_features.c)


file(GLOB TASK_SCHED_SRC
        ${TOP_DIR}/src/task_sched/*.cpp
        ${TOP_DIR}/src/task_sched/*.c
        ${TOP_DIR}/src/task_sched/*.h
        ${TOP_DIR}/src/task_sched/*.hpp
)

file(GLOB OSAL_SRC
        ${TOP_DIR}/src/osal/*.cpp
        ${TOP_DIR}/src/osal/*.c
        ${TOP_DIR}/src/osal/*.h
        ${TOP_DIR}/src/osal/*.hpp
)

file(GLOB BUF_IO_SRC
        ${TOP_DIR}/src/buf_io/*.cpp
        ${TOP_DIR}/src/buf_io/*.c
        ${TOP_DIR}/src/buf_io/*.h
        ${TOP_DIR}/src/buf_io/*.hpp
)

file(GLOB UTILS_SRC
        ${TOP_DIR}/src/utils/*.cpp
        ${TOP_DIR}/src/utils/*.c
        ${TOP_DIR}/src/utils/*.h
        ${TOP_DIR}/src/utils/*.hpp
)

file(GLOB AUDIO_SRC
        ${TOP_DIR}/src/audio/*.c
        ${TOP_DIR}/src/audio/*.cpp
        ${TOP_DIR}/src/audio/*.h
        ${TOP_DIR}/src/audio/*.hpp
)

file(GLOB TONE_GEN_SRC
        ${TOP_DIR}/src/tone_gen/*.c
        ${TOP_DIR}/src/tone_gen/*.cpp
        ${TOP_DIR}/src/tone_gen/*.h
        ${TOP_DIR}/src/tone_gen/*.hpp
)

file(GLOB TONE_DET_SRC
        ${TOP_DIR}/src/tone_det/*.c
        ${TOP_DIR}/src/tone_det/*.cpp
        ${TOP_DIR}/src/tone_det/*.h
        ${TOP_DIR}/src/tone_det/*.hpp
)

file(GLOB MINI_SOCKET_SRC
        ${TOP_DIR}/src/mini_socket/*.c
        ${TOP_DIR}/src/mini_socket/*.cpp
        ${TOP_DIR}/src/mini_socket/*.h
        ${TOP_DIR}/src/mini_socket/*.hpp
)

set(GTEST_SRC
        ${TOP_DIR}/ext/googletest/googletest/src/gtest-all.cc
        ${TOP_DIR}/ext/googletest/googlemock/src/gmock-all.cc
)
