#ifndef CRC_H__
#define CRC_H__
/*
  Provides CRC16-CCITT and CRC32 implementations.
*/

#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

uint16_t CRC_16( const uint8_t *pAddr, const unsigned int bytes );

#ifdef __cplusplus
// Continuation CRC, allows for CRC to be calculated over several buffers.
uint16_t CRC_16_CONT(const uint8_t *data_p, const unsigned int bytes, const uint16_t crc16 = 0xffff);
#else // __cplusplus
uint16_t CRC_16_CONT(const uint8_t *data_p, const unsigned int bytes, const uint16_t crc16);
#endif // __cplusplus

uint32_t CRC_32( const uint8_t * pAddr, const uint32_t bytes );

#ifdef __cplusplus
uint32_t CRC_32_CONT(const uint8_t *pAddr, const uint32_t bytes, uint32_t crc = 0xffffffff);
#else
uint32_t CRC_32_CONT(const uint8_t *pAddr, const uint32_t bytes, uint32_t crc);
#endif

uint32_t CRC_32_CONT_FINISH(uint32_t crc);

// Note... NOT compatible with CRC_32() in this file. Use for internal calculations only.
uint32_t CRC_32_FAST(
  const uint8_t * const buf,
  const size_t size,
#ifdef __cplusplus
  uint32_t crc = 0xffffffff,
  const bool finish = true
#else
  uint32_t crc,
  const bool finish
#endif
);
  
  
#ifdef __cplusplus
}
#endif




#endif
