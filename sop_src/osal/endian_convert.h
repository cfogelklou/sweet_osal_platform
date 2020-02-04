/**
* COPYRIGHT	(c)	Applicaudia 2020
* @file     endian_convert.h
* @brief    Portable endian conversion routines to ease swapping between big-endian
*           and little-endian on different machine types.
*/

#ifndef ENDIAN_CONVERT_H__
#define ENDIAN_CONVERT_H__

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#include "platform_type.h"
#if (TARGET_OS_IOS)
#define NATIVE_LITTLE_ENDIAN
#endif

#if !defined(NATIVE_LITTLE_ENDIAN) && !defined(NATIVE_BIG_ENDIAN)
#ifndef ccs
#error "Libsodium requires NATIVE_LITTLE_ENDIAN if NATIVE_BIG_ENDIAN is not defined."
#endif
#endif
#ifdef __cplusplus
// only for cplusplus since C doesn't support default values

// The functions that take uint8_t * as input/output are safe for unaligned data.

uint8_t * HostToLE16(const uint16_t w16, uint8_t * const pb2);
uint16_t HostToLE16W(const uint16_t w16);
uint8_t * HostToBE16(const uint16_t w16, uint8_t * const pb2);
uint16_t HostToBE16W(const uint16_t w16);
uint16_t LE16ToHost(const uint8_t * const pb2, uint16_t * const pw16 = 0);
uint16_t LE16WToHost(const uint16_t w16);
uint16_t BE16ToHost(const uint8_t * const pb2, uint16_t * const pw16 = 0);
uint16_t BE16WToHost(const uint16_t w16);

uint8_t * HostToLE32(const uint32_t w32, uint8_t * const pb4);
uint32_t HostToLE32W(const uint32_t w32);
uint8_t * HostToBE32(const uint32_t w32, uint8_t * const pb4);
uint32_t HostToBE32W(const uint32_t w32);
uint32_t LE32ToHost(const uint8_t * const pb4, uint32_t * const pw32 = 0);
uint32_t LE32WToHost(const uint32_t w32);
uint32_t BE32ToHost(const uint8_t * const pb4, uint32_t * const pw32 = 0);
uint32_t BE32WToHost(const uint32_t w32);

uint8_t *HostToLE64(const uint64_t w64, uint8_t * const pb8);
uint64_t HostToLE64W(const uint64_t w64);
uint8_t *HostToBE64(const uint64_t w64, uint8_t * const pb8);
uint64_t HostToBE64W(const uint64_t w64);
uint64_t LE64ToHost(const uint8_t * const pb8, uint64_t * const pw64 = 0);
uint64_t LE64WToHost(const uint64_t w64);
uint64_t BE64ToHost(const uint8_t * const pb8, uint64_t * const pw64 = 0);
uint64_t BE64WToHost(const uint64_t w64);


#endif // __cplusplus

#define SWAPBYTES64(u64) ( \
  ((((uint64_t)(u64)) & 0x00000000000000ffull) << (64-(1*8))) | \
  ((((uint64_t)(u64)) & 0x000000000000ff00ull) << (64-(3*8))) | \
  ((((uint64_t)(u64)) & 0x0000000000ff0000ull) << (64-(5*8))) | \
  ((((uint64_t)(u64)) & 0x00000000ff000000ull) << (64-(7*8))) | \
  ((((uint64_t)(u64)) & 0x000000ff00000000ull) >> (64-(7*8))) | \
  ((((uint64_t)(u64)) & 0x0000ff0000000000ull) >> (64-(5*8))) | \
  ((((uint64_t)(u64)) & 0x00ff000000000000ull) >> (64-(3*8))) | \
  ((((uint64_t)(u64)) & 0xff00000000000000ull) >> (64-(1*8))) \
)

#define SWAPBYTES32(u32) ( \
  ((((uint32_t)(u32)) & 0x000000ff) << 24) | \
  ((((uint32_t)(u32)) & 0x0000ff00) << 8) | \
  ((((uint32_t)(u32)) & 0x00ff0000) >> 8) | \
  ((((uint32_t)(u32)) & 0xff000000) >> 24) \
)

#define SWAPBYTES16(u16) ( \
  ((((uint16_t)(u16)) & 0x00ff) << 8) | \
  ((((uint16_t)(u16)) & 0xff00) >> 8) \
)

#ifdef NATIVE_BIG_ENDIAN
#define HOSTTOLE64(u64) SWAPBYTES64(u64)
#define HOSTTOLE32(u32) SWAPBYTES32(u32)
#define HOSTTOLE16(u16) SWAPBYTES16(u16)
#define HOSTTOBE64(u64) (u64)
#define HOSTTOBE32(u32) (u32)
#define HOSTTOBE16(u16) (u16)
#define LE64TOHOST(u64) SWAPBYTES64(u64)
#define LE32TOHOST(u32) SWAPBYTES32(u32)
#define LE16TOHOST(u16) SWAPBYTES16(u16)
#define BE64TOHOST(u64) (u64)
#define BE32TOHOST(u32) (u32)
#define BE16TOHOST(u16) (u16)
#else

#define HOSTTOLE64(u64) (u64)
#define HOSTTOLE32(u32) (u32)
#define HOSTTOLE16(u16) (u16)
#define HOSTTOBE64(u64) SWAPBYTES64(u64)
#define HOSTTOBE32(u32) SWAPBYTES32(u32)
#define HOSTTOBE16(u16) SWAPBYTES16(u16)
#define LE64TOHOST(u64) (u64)
#define LE32TOHOST(u32) (u32)
#define LE16TOHOST(u16) (u16)
#define BE64TOHOST(u64) SWAPBYTES64(u64)
#define BE32TOHOST(u32) SWAPBYTES32(u32)
#define BE16TOHOST(u16) SWAPBYTES16(u16)
#endif

#ifdef __cplusplus
}
#endif

#endif // ENDIAN_CONVERT_H__
