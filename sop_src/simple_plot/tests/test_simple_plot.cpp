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
#include "simple_plot/simple_plot.hpp"



LOG_MODNAME("test_simple_plot.cpp")

class TestSimplePlot : public GtestMempoolsWrapper {
protected:
  TestSimplePlot() {
#ifdef WIN32
    OSALMSHookToHardware(false);
#endif
  }
  virtual ~TestSimplePlot() {
#ifdef WIN32
    OSALMSHookToHardware(true);
#endif
  }
};


TEST_F(TestSimplePlot, constructor) 
{
  SimplePlot plot;
}

typedef struct ScreenYTag {
  std::vector<uint8_t>y;
}ScreenY;

typedef struct FakeScreenTag {
  std::vector< ScreenY> x;
  void* pUserData = nullptr;
}FakeScreen;

static void test_SetPixel(struct SP_GfxApiTag* pGP, SP_IntT nX, SP_IntT nY, SP_ColourT nColour) {
  FakeScreen* pScreen = (FakeScreen*)pGP->pUserData;
  FakeScreen& s = *pScreen;
  LOG_TRACE(("Pixel(%d,%d) = %d\r\n", nX, nY, nColour));
  s.x[nX].y[nY] = nColour;
}

TEST_F(TestSimplePlot, three_elements)
{
  SimplePlot plot(2,2);
  FakeScreen screen;
  screen.x.resize(3);
  for (int i = 0; i < screen.x.size(); i++) {
    screen.x[i].y.resize(3);
  }

  SP_GfxApiT gfx = {0};
  gfx.nWidth = 3;
  gfx.nHeight = 3;
  gfx.SetPixel = test_SetPixel;
  gfx.pUserData = &screen;

  plot.Init(&gfx);

  SP_DataSetT dataSet = {0};
  dataSet.nID = 1;
  dataSet.GetYValue = [](struct SP_DataSetTag* pDS, SP_FloatT fX, SP_FloatT* pfY) {
    LOG_TRACE(("SimplePlot asked for pixel at %f\r\n", fX));
    *pfY = fX;
    return SPLOT_SUCCESS;
  };
  dataSet.fMinX = 0;
  dataSet.fMaxX = 2;
  
  plot.AddWaveform(&dataSet);

  plot.ReDraw();

  LOG_ASSERT(screen.x[0].y[0] == 1);
  LOG_ASSERT(screen.x[0].y[1] == 0);
  LOG_ASSERT(screen.x[1].y[1] == 1);
  LOG_ASSERT(screen.x[1].y[0] == 0);
  LOG_ASSERT(screen.x[2].y[2] == 1);
  LOG_ASSERT(screen.x[2].y[1] == 0);
}

TEST_F(TestSimplePlot, five_elements)
{
  SimplePlot plot(4, 4);
  FakeScreen screen;
  screen.x.resize(5);
  for (int i = 0; i < screen.x.size(); i++) {
    screen.x[i].y.resize(5);
  }

  SP_GfxApiT gfx = { 0 };
  gfx.nWidth = 5;
  gfx.nHeight = 5;
  gfx.SetPixel = test_SetPixel;
  gfx.pUserData = &screen;

  plot.Init(&gfx);

  SP_DataSetT dataSet = { 0 };
  dataSet.nID = 1;
  dataSet.GetYValue = [](struct SP_DataSetTag* pDS, SP_FloatT fX, SP_FloatT* pfY) {
    LOG_TRACE(("SimplePlot asked for pixel at %f\r\n", fX));
    *pfY = fX;
    return SPLOT_SUCCESS;
  };
  dataSet.fMinX = 0;
  dataSet.fMaxX = 4;

  plot.AddWaveform(&dataSet);

  plot.ReDraw();

  LOG_ASSERT(screen.x[0].y[0] == 1);
  LOG_ASSERT(screen.x[1].y[1] == 1);
  LOG_ASSERT(screen.x[2].y[2] == 1);
  LOG_ASSERT(screen.x[3].y[3] == 1);
  LOG_ASSERT(screen.x[4].y[4] == 1);
}

TEST_F(TestSimplePlot, five_elements_with_border)
{
  SimplePlot plot(4, 4);
  FakeScreen screen;
  screen.x.resize(5);
  for (int i = 0; i < screen.x.size(); i++) {
    screen.x[i].y.resize(5);
  }

  SP_GfxApiT gfx = { 0 };
  gfx.nWidth = 5;
  gfx.nHeight = 5;
  gfx.SetPixel = test_SetPixel;
  gfx.pUserData = &screen;

  plot.Init(&gfx);
  plot.fillWithBackgroundColor = true;

  const auto bg = 5;
  plot.SetBackgroundColour(bg);

  SP_DataSetT dataSet = { 0 };
  dataSet.nID = 1;
  dataSet.GetYValue = [](struct SP_DataSetTag* pDS, SP_FloatT fX, SP_FloatT* pfY) {
    LOG_TRACE(("SimplePlot asked for pixel at %f\r\n", fX));
    *pfY = fX;
    return SPLOT_SUCCESS;
  };
  dataSet.fMinX = 0;
  dataSet.fMaxX = 4;

  plot.AddWaveform(&dataSet);

  plot.ReDraw();

  LOG_ASSERT(screen.x[0].y[0] == 1);
  LOG_ASSERT(screen.x[1].y[1] == 1);
  LOG_ASSERT(screen.x[2].y[2] == 1);
  LOG_ASSERT(screen.x[3].y[3] == 1);
  LOG_ASSERT(screen.x[4].y[4] == 1);

  // Check background color
  LOG_ASSERT(screen.x[0].y[4] == bg);
  LOG_ASSERT(screen.x[4].y[0] == bg);
}


TEST_F(TestSimplePlot, five_elements_small_scale)
{
  SimplePlot plot(1, 1);
  FakeScreen screen;
  screen.x.resize(5);
  for (int i = 0; i < screen.x.size(); i++) {
    screen.x[i].y.resize(5);
  }

  SP_GfxApiT gfx = { 0 };
  gfx.nWidth = 5;
  gfx.nHeight = 5;
  gfx.SetPixel = test_SetPixel;
  gfx.pUserData = &screen;

  plot.Init(&gfx);
  plot.fillWithBackgroundColor = true;

  const auto bg = 5;
  plot.SetBackgroundColour(bg);

  SP_DataSetT dataSet = { 0 };
  dataSet.nID = 1;
  dataSet.GetYValue = [](struct SP_DataSetTag* pDS, SP_FloatT fX, SP_FloatT* pfY) {
    LOG_TRACE(("SimplePlot asked for pixel at %f\r\n", fX));
    *pfY = fX;
    return SPLOT_SUCCESS;
  };
  dataSet.fMinX = 0;
  dataSet.fMaxX = 1;

  plot.AddWaveform(&dataSet);

  plot.ReDraw();

  LOG_ASSERT(screen.x[0].y[0] == 1);
  LOG_ASSERT(screen.x[1].y[1] == 1);
  LOG_ASSERT(screen.x[2].y[2] == 1);
  LOG_ASSERT(screen.x[3].y[3] == 1);
  LOG_ASSERT(screen.x[4].y[4] == 1);

  // Check background color
  LOG_ASSERT(screen.x[0].y[4] == bg);
  LOG_ASSERT(screen.x[4].y[0] == bg);
}


TEST_F(TestSimplePlot, five_elements_huge_scale)
{
  SimplePlot plot(100, 100);
  FakeScreen screen;
  screen.x.resize(5);
  for (int i = 0; i < screen.x.size(); i++) {
    screen.x[i].y.resize(5);
  }

  SP_GfxApiT gfx = { 0 };
  gfx.nWidth = 5;
  gfx.nHeight = 5;
  gfx.SetPixel = test_SetPixel;
  gfx.pUserData = &screen;

  plot.Init(&gfx);
  plot.fillWithBackgroundColor = true;

  const auto bg = 5;
  plot.SetBackgroundColour(bg);

  SP_DataSetT dataSet = { 0 };
  dataSet.nID = 1;
  dataSet.GetYValue = [](struct SP_DataSetTag* pDS, SP_FloatT fX, SP_FloatT* pfY) {
    LOG_TRACE(("SimplePlot asked for pixel at %f\r\n", fX));
    *pfY = fX;
    return SPLOT_SUCCESS;
  };
  dataSet.fMinX = 0;
  dataSet.fMaxX = 100;

  plot.AddWaveform(&dataSet);

  plot.ReDraw();

  LOG_ASSERT(screen.x[0].y[0] == 1);
  LOG_ASSERT(screen.x[1].y[1] == 1);
  LOG_ASSERT(screen.x[2].y[2] == 1);
  LOG_ASSERT(screen.x[3].y[3] == 1);
  LOG_ASSERT(screen.x[4].y[4] == 1);

  // Check background color
  LOG_ASSERT(screen.x[0].y[4] == bg);
  LOG_ASSERT(screen.x[4].y[0] == bg);
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
