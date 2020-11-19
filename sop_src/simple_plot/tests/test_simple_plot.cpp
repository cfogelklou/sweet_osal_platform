/**
 * COPYRIGHT    (c)	Applicaudia 2018
 * @file        tone_ble_crypto.cpp
 * @brief       Test of BLE Cryptography Primitives.
 */
#include <gtest/gtest.h>



#ifndef RUN_GTEST
#include <gmock/gmock.h>
using namespace testing;
#endif

#include "utils/byteq.h"
#include "gtest/gtest.h"
#include "tests/gtest_test_wrapper.hpp"
#include "buf_io/buf_io.hpp"
#include "buf_io/buf_io_queue.hpp"
#include "utils/crc.h"
#include "task_sched/task_sched.h"
#include "tone_gen/tone_gen_node.hpp"
#include "audio_router/audio_router.hpp"
#include "fft_multi/fft_multi.hpp"
#include "fft/fft_window.hpp"



LOG_MODNAME("test_note_listener.cpp")

class TestNoteListener : public GtestMempoolsWrapper {
protected:
  TestNoteListener() {
#ifdef WIN32
    OSALMSHookToHardware(false);
#endif
  }
  virtual ~TestNoteListener() {
#ifdef WIN32
    OSALMSHookToHardware(true);
#endif
  }
};


TEST_F(TestNoteListener, fftmulti) 
{
  FftMulti fft(512, 4);
}

TEST_F(TestNoteListener, fftmulti_4)
{
  pcm_t x[4096];
  float amp = 1.0f;
  double fs = 48000;
  FftMulti fft(512, 4);
  const size_t effectiveSz = fft.getEffectiveFftSize();
  LOG_ASSERT(effectiveSz == ARRSZ(x));

  for (int j = 1; j < 40; j++) {
    double f = j * 500;

    for (int i = 0; i < ARRSZ(x); i++) {
      x[i] = (fftin_t)(amp * sin(2 * M_PI * i * f / fs));
    }
    fft.sanityCheck();
    fft.addSamples(x, ARRSZ(x));
    fft.sanityCheck();
    fft.doFft();
    fft.sanityCheck();
    size_t numSamps = 0;
    const double* pRes = fft.getResult(numSamps);
    LOG_ASSERT(pRes);
    LOG_ASSERT(numSamps == ARRSZ(x));


    const size_t peakAmpIdx = FftT::findPeakFftIdx( pRes, numSamps);
    const auto freq = FftT::fftIdxToFreq(
      peakAmpIdx,
      effectiveSz,
      fs);

    const auto diff = f - freq;
    const auto _abs = fabs(diff);
    const auto err = _abs / freq;
    const auto pct = err * 100;
    LOG_TRACE(("Error at f=%d was %f percent\r\n", (int)f, pct));
    EXPECT_LT(pct, 1.0);
  }
}

TEST_F(TestNoteListener, fftmulti_5)
{
  pcm_t x[8192];
  float amp = 1.0f;
  double fs = 48000;
  FftMulti fft(512, 5);
  const size_t effectiveSz = fft.getEffectiveFftSize();
  LOG_ASSERT(effectiveSz == ARRSZ(x));

  for (int j = 1; j < 40; j++) {
    double f = j * 250;

    for (int i = 0; i < ARRSZ(x); i++) {
      x[i] = (fftin_t)(amp * sin(2 * M_PI * i * f / fs));
    }
    fft.sanityCheck();
    fft.addSamples(x, ARRSZ(x));
    fft.sanityCheck();
    fft.doFft();
    fft.sanityCheck();
    size_t numSamps = 0;
    const double* pRes = fft.getResult(numSamps);
    LOG_ASSERT(pRes);
    LOG_ASSERT(numSamps == ARRSZ(x));

    const size_t peakAmpIdx = FftT::findPeakFftIdx(
      pRes, numSamps);
    const auto freq = FftT::fftIdxToFreq(
      peakAmpIdx,
      effectiveSz,
      fs);

    const auto diff = f - freq;
    const auto _abs = fabs(diff);
    const auto err = _abs / freq;
    const auto pct = err * 100;
    LOG_TRACE(("Error at f=%d was %f percent\r\n", (int)f, pct));
    EXPECT_LT(pct, 1.0);
  }
}

TEST_F(TestNoteListener, fftmulti_5_lowfreq)
{
  pcm_t x[16384];
  float amp = 1.0f;
  double fs = 48000;
  FftMulti fft(1024, 5);
  const size_t effectiveSz = fft.getEffectiveFftSize();
  LOG_ASSERT(effectiveSz == ARRSZ(x));

  for (int j = 20; j < 250; j++) {
    double f = j;

    for (int i = 0; i < ARRSZ(x); i++) {
      x[i] = (fftin_t)(amp * sin(2 * M_PI * i * f / fs));
    }
    fft.sanityCheck();
    fft.addSamples(x, ARRSZ(x));
    fft.sanityCheck();
    fft.doFft();
    fft.sanityCheck();
    size_t numSamps = 0;
    const double* pRes = fft.getResult(numSamps);
    LOG_ASSERT(pRes);
    LOG_ASSERT(numSamps == ARRSZ(x));

    const size_t peakAmpIdx = FftT::findPeakFftIdx( pRes, numSamps);
    const auto freq = FftT::fftIdxToFreq( peakAmpIdx, effectiveSz, fs);

    const auto diff = f - freq;
    const auto _abs = fabs(diff);
    const auto err = _abs / freq;
    const auto pct = err * 100;
    LOG_TRACE(("detected %f at f at f=%d. Error %f percent\r\n", freq, (int)f, pct));
    LOG_TRACE((""));
    EXPECT_LT(pct, 7.0);
  }
}


#ifndef RUN_GTEST
int main(int argc, char** argv) {

  OSALInit();
  TaskSchedInit();

  int gtest_rval;
  {
      GTestRandomFileInitializer rndFile;

      // The following line must be executed to initialize Google Mock
      // (and Google Test) before running the tests.
      ::testing::InitGoogleMock(&argc, argv);

      gtest_rval = RUN_ALL_TESTS();

  }
  MemPoolsPrintUsage();

  TaskSchedQuit();

  return gtest_rval;

}
#endif
