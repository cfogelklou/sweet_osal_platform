#ifdef RUN_GTEST
#include <stdbool.h>
int TestBoolC_GetBoolSize(void) {
  return sizeof(bool);
}

int TestBoolC_GetTrue(void) {
  return (int)true;
}

int TestBoolC_GetFalse(void) {
  return (int)false;
}
#endif
