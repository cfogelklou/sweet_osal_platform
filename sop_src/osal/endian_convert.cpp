/**
* COPYRIGHT	(c)	Applicaudia 2020
* @file     endian_convert.cpp
* @brief    Portable endian conversion routines to ease swapping between big-endian
*           and little-endian on different machine types.
*/
#include "endian_convert.h"
#include "utils/platform_log.h"
#include <string.h>

LOG_MODNAME("endian_convert.cpp");

extern "C" {

  
// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint8_t * HostToLE16(const uint16_t w16, uint8_t * const pb2){
  LOG_ASSERT(pb2);
  uint16_t out = HostToLE16W(w16);
  memcpy(pb2, &out, sizeof(out));
  return pb2;
}

// ////////////////////////////////////////////////////////////////////////////
uint16_t HostToLE16W(const uint16_t w16){
  return HOSTTOLE16(w16);
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint16_t LE16ToHost(const uint8_t * const pb2, uint16_t * const pw16) {
  LOG_ASSERT(pb2);
  uint16_t cnv;
  memcpy(&cnv, pb2, sizeof(cnv));
  const uint16_t rval = LE16WToHost(cnv);
  if (pw16) {
    *pw16 = rval;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
uint16_t LE16WToHost(const uint16_t w16){
  return LE16TOHOST(w16);
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint8_t * HostToBE16(const uint16_t w16, uint8_t * const pb2){
  LOG_ASSERT(pb2);
  const uint16_t out = HostToBE16W(w16);
  memcpy(pb2, &out, sizeof(out));
  return pb2;
}

// ////////////////////////////////////////////////////////////////////////////
uint16_t HostToBE16W(const uint16_t w16){
  return HOSTTOBE16(w16);
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint16_t BE16ToHost(const uint8_t * const pb2, uint16_t * const pw16 ){
  LOG_ASSERT(pb2);
  uint16_t cnv;
  memcpy(&cnv, pb2, sizeof(cnv));
  const uint16_t rval = BE16WToHost(cnv);
  if (pw16) {
    *pw16 = rval;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
uint16_t BE16WToHost(const uint16_t w16) {
  return BE16TOHOST(w16);
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint8_t * HostToLE32(const uint32_t w32, uint8_t * const pb4){
  LOG_ASSERT(pb4);
  uint32_t out = HostToLE32W(w32);
  memcpy(pb4, &out, sizeof(out));
  return pb4;
}

// ////////////////////////////////////////////////////////////////////////////
uint32_t HostToLE32W(const uint32_t w32){
  return HOSTTOLE32(w32);
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint32_t LE32ToHost(const uint8_t * const pb4, uint32_t * const pw32 ){
  LOG_ASSERT(pb4);
  uint32_t cnv;
  memcpy(&cnv, pb4, sizeof(cnv));
  const uint32_t rval = LE32WToHost(cnv);
  if (pw32) {
    *pw32 = rval;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
uint32_t LE32WToHost(const uint32_t w32) {
  return LE32TOHOST(w32);
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint8_t * HostToBE32(const uint32_t w32, uint8_t * const pb4){
  LOG_ASSERT(pb4);
  const uint32_t out = HostToBE32W(w32);
  memcpy(pb4, &out, sizeof(out));
  return pb4;
}

// ////////////////////////////////////////////////////////////////////////////
uint32_t HostToBE32W(const uint32_t w32){
  return HOSTTOBE32(w32);
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint32_t BE32ToHost(const uint8_t * const pb4, uint32_t * const pw32 ){
  LOG_ASSERT(pb4);
  uint32_t cnv;
  memcpy(&cnv, pb4, sizeof(cnv));
  const uint32_t rval = BE32WToHost(cnv);
  if (pw32) {
    *pw32 = rval;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
uint32_t BE32WToHost(const uint32_t w32) {
  return BE32TOHOST(w32);
}

// ////////////////////////////////////////////////////////////////////////////
static void check64() {
  // we need at least this for the  SWAPBYTES64() macro
  ASSERT_AT_COMPILE_TIME(sizeof(unsigned long long) >= sizeof(uint64_t)); 
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint8_t *HostToLE64(const uint64_t w64, uint8_t * const pb8) {
  (void)check64();
  LOG_ASSERT(pb8);
  const uint64_t out = HostToLE64W(w64);
  memcpy(pb8, &out, sizeof(out));
  return pb8;
}

// ////////////////////////////////////////////////////////////////////////////
uint64_t HostToLE64W(const uint64_t w64){
  return HOSTTOLE64(w64);
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint64_t LE64ToHost(const uint8_t * const pb8, uint64_t * const pw64) {
  LOG_ASSERT(pb8);
  uint64_t cnv;
  memcpy(&cnv, pb8, sizeof(cnv));
  const uint64_t rval = LE64WToHost(cnv);
  if (pw64) {
    *pw64 = rval;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
uint64_t LE64WToHost(const uint64_t w64){
  return LE64TOHOST(w64);
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint8_t *HostToBE64(const uint64_t w64, uint8_t * const pb8) {
  LOG_ASSERT(pb8);
  const uint64_t out = HostToBE64W(w64);
  memcpy(pb8, &out, sizeof(out));
  return pb8;
}

// ////////////////////////////////////////////////////////////////////////////
uint64_t HostToBE64W(const uint64_t w64){
  return HOSTTOBE64(w64);
}

// ////////////////////////////////////////////////////////////////////////////
// Safe for unaligned data
uint64_t BE64ToHost(const uint8_t * const pb8, uint64_t * const pw64) {
  LOG_ASSERT(pb8);
  uint64_t cnv;
  memcpy(&cnv, pb8, sizeof(cnv));
  const uint64_t rval = BE64WToHost(cnv);
  if (pw64) {
    *pw64 = rval;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
uint64_t BE64WToHost(const uint64_t w64){
  return BE64TOHOST(w64);
}

} // extern "C"
