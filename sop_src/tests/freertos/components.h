// Dummy definitions

#ifndef _COMPONENTS_H_
#define _COMPONENTS_H_

#include <iostream>
#include <cstring>
#include <stdint.h>

typedef int SerialDriver;
typedef int SerialConfig;

static const int SERIAL_MSG_OK = 0;
static const SerialDriver _SD1 = 1;
SerialDriver SD1 = _SD1;

int sd_lld_write(SerialDriver *sdp, uint8_t *buffer, uint16_t len) {
  using namespace std;
  if (*sdp != _SD1) {
    cout << "WARNING: sdp isn't _SD1" << endl;
  }
  if (strlen((const char*)buffer)+1 != len) {
    cout << "WARNING: '" << buffer << "' has wrong length (" << strlen((const char*)buffer) << " != " << len << ")" << endl;
  }
  cout << buffer << endl;
  return SERIAL_MSG_OK;
}

void sd_lld_start(SerialDriver *sdp, const SerialConfig *config) {
  using namespace std;
  if (*sdp != _SD1) {
    cout << "WARNING: sdp isn't _SD1" << endl;
  }
}

void componentsInit() {}

void irqIsrEnable() {}

#endif // _COMPONENTS_H_