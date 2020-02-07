#include "uuid.hpp"

//#define USE_UUID_EXTERN
#ifdef USE_UUID_EXTERN
#include "extern_libs/uuid/src/sysdep.h"
#include "extern_libs/uuid/src/uuid.h"
#endif
#include "mbedtls/sha1.h"
#include "osal/endian_convert.h"
#include <string.h>

#include "utils/platform_log.h"

LOG_MODNAME("uuid.cpp");

#ifdef USE_UUID_EXTERN
void UUID::uuid_create_v5_from_name(
                                    uint8_t uuid_out[16],     /* resulting UUID */
                                    const uint8_t ns[16], /* UUID of the namespace */
                                    const uint8_t name[],   /* the name from which to generate a UUID */
                                    const size_t namelen           /* the length of the name */
){
  rfc4122_uuid_t ns_uuid;
  rfc4122_uuid_t out_uuid;
  memcpy(&ns_uuid, ns, 16 );
  ns_uuid.time_low = BE32ToHost(&ns[0]);
  ns_uuid.time_mid = BE16ToHost(&ns[4]);
  ns_uuid.time_hi_and_version = BE16ToHost(&ns[6]);
  uuid_create_sha1_from_name(
                             &out_uuid,
                             ns_uuid,
                             (void *)&name[0],
                             (int)namelen);
  memcpy(&uuid_out[0], &out_uuid, 16);
  HostToBE32(out_uuid.time_low, &uuid_out[0]);
  HostToBE16(out_uuid.time_mid, &uuid_out[4]);
  HostToBE16(out_uuid.time_hi_and_version, &uuid_out[6]);

}
#else
void UUID::uuid_create_v5_from_name(
                                    uint8_t uuid_out[16],     /* resulting UUID */
                                    const uint8_t ns[16], /* UUID of the namespace */
                                    const uint8_t name[],   /* the name from which to generate a UUID */
                                    const size_t namelen           /* the length of the name */
){

  mbedtls_sha1_context c;
  unsigned char hash[20];
  
  mbedtls_sha1_init(&c);
  mbedtls_sha1_starts(&c);
  mbedtls_sha1_update_ret(&c, ns, 16);
  mbedtls_sha1_update_ret(&c, name, namelen);
  
  mbedtls_sha1_finish_ret(&c, hash);
  mbedtls_sha1_free(&c);

  memcpy( uuid_out, hash, 16);

  /* the hash is in network byte order at this point */
  //uuid->time_hi_and_version &= 0x0FFF;
  //uuid->time_hi_and_version |= (v << 12);
  uuid_out[6] &= 0x0f;
  uuid_out[6] |= 0x50;

  //uuid->clock_seq_hi_and_reserved &= 0x3F;
  uuid_out[8] &= 0x3f;

  //uuid->clock_seq_hi_and_reserved |= 0x80;
  uuid_out[8] |= 0x80;
}
#endif


void UUID::ski_to_data_communication_id(
                                        const uint8_t ski[20],
                                        uint8_t out[16] )
{
  UUID::uuid_create_v5_from_name(out, pairing_service_id, ski, 20 );
}

// ///////////////////////////////////////////////////////////////////
static uint8_t toHex(const char c){
  uint8_t r = 0xff;
  if ((c >= '0') && (c <= '9')) {r = c - '0' + 0;}
  else if ((c >= 'a') && (c <= 'f')) {r = c - 'a' + 10;}
  else if ((c >= 'A') && (c <= 'F')) {r = c - 'A' + 10;}
  return r;
}

// ///////////////////////////////////////////////////////////////////
void UUID::asc_to_uuid(const char *asc, uint8_t bin[UUID_BYTES]){
  const size_t len = strlen(asc);
  int nybbleIdx = 0;
  uint8_t byte = 0;
  size_t o = 0;
  size_t i = 0;
  for (; i < len; i++){
    const uint8_t nybb = toHex(asc[i]);
    if (0xff != nybb){
      byte = (byte << 4) | nybb;
      if (0 == ((++nybbleIdx) & 0x01)){
        bin[o++] = byte;
        byte = 0;
      }
    }
  }
  LOG_ASSERT(UUID_BYTES == o);
  LOG_ASSERT(0 == asc[i]);
}

// ///////////////////////////////////////////////////////////////////
void UUID::reverse_uuid(uint8_t bin[UUID_BYTES]){
  uint8_t tmp[UUID_BYTES];
  memcpy(tmp, bin, UUID_BYTES);
  for (size_t i = 0; i < UUID_BYTES; i++){
    const size_t o = UUID_BYTES - 1 - i;
    bin[o] = tmp[i];
  }
}
