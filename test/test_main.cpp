#include <gtest/gtest.h>
#include <gmock/gmock.h>
using namespace testing;

#ifdef WIN32
#include <Windows.h>
#define usleep(x) Sleep(x/1000)
#endif


static void stupidRandom(uint8_t *buf, int cnt) {
  for (int i = 0; i < cnt; i++) {
    buf[i] = rand() % 255;
  }
}

TEST(TestAudioLib, bleah){

}

int main(int argc, char** argv){
  
  // The following line must be executed to initialize Google Mock
  // (and Google Test) before running the tests.
  ::testing::InitGoogleMock(&argc, argv);
  const int gtest_rval = RUN_ALL_TESTS();
  
  return gtest_rval;
}
