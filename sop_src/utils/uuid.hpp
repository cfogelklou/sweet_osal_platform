#ifndef UUID_HPP
#define UUID_HPP

#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus

namespace UUID {
  
  const size_t UUID_BYTES = 16;
  
  /*
   The UUID for Data Communication Service shall be calculated using UUID
   Version 5. The input shall be “0000fe43-0000-1000-8000-00805f9b34fb”
   (the pairing service uuid) as namespace and the actor certificate SKI
   in hex string without any delimiter as name.
 */
  static const uint8_t pairing_service_id[16] = {
    // 0000fe43
    0x00, 0x00, 0xfe, 0x43,
    // 0000
    0x00, 0x00,
    // 1000
    0x10, 0x00,
    // 8000
    0x80, 0x00,
    // 00805f9b34fb
    0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
  };
  
  /*
   The iBeacon shall contain a UUID defining the company
   (E20A-39F4-73F5-4BC4-1864-17D1-AD07-A962) and the major and minor
   part defining the unique car.
   */
  static const uint8_t i_beacon_id[UUID_BYTES] = {
    // E20A-39F4
    0xe2, 0x0a, 0x39, 0xf4,
    // 73F5
    0x73, 0xF5,
    // 4BC4
    0x4B, 0xC4,
    // 1864
    0x18, 0x64,
    // 17D1AD07A962
    0x17, 0xD1, 0xAD, 0x07, 0xA9, 0x62
  };
  
  // ///////////////////////////////////////////////////////////////////
  void uuid_create_v5_from_name(
                                  uint8_t uuid_out[UUID_BYTES],     /* resulting UUID */
                                  const uint8_t ns[UUID_BYTES], /* UUID of the namespace */
                                  const uint8_t name[],   /* the name from which to generate a UUID */
                                  const size_t namelen           /* the length of the name */
  );
  
  // ///////////////////////////////////////////////////////////////////
  void ski_to_data_communication_id(
    const uint8_t ski[20],
    uint8_t out[UUID_BYTES] );
  
  // ///////////////////////////////////////////////////////////////////
  void asc_to_uuid(const char *asc, uint8_t bin[UUID_BYTES]);
  
  // ///////////////////////////////////////////////////////////////////
  // The TI chip requires UUID to be saved in reverse.
  void reverse_uuid(uint8_t bin[UUID_BYTES]);
}

#endif // #ifdef __cplusplus

#endif
