/*
  Simplifies the crappy C-like interface on simple_plot.hpp
*/
#ifndef EZ_PLOT_HPP
#define EZ_PLOT_HPP

#include "simple_plot/simple_plot.hpp"
#include "utils/helper_macros.h"

#include <ostream>
#include <string>
#include <vector>

/**
  Simple plot is crappy.
  This makes it less crappy, but the API still needs work.
  Some example code:
    EzPlot plot;

#if 0
  const auto cb = [](struct SP_DataSetTag* pDS, SP_FloatT fX, SP_FloatT* pfY) {
    ResponseCurve* pCurve = (ResponseCurve *)pDS->pUserData;
    *pfY = pCurve->GetAmpForFreq(fX);
    LOG_TRACE(("Requesting amplitude for %fHz, returning %f\r\n", fX, *pfY));
    return SPLOT_SUCCESS;
  };

  auto d = plot.GenerateDataset(cb, "response", &curve);
#else
  auto d = plot.GenerateDataset(nullptr, "response", &curve);
#endif
  float minX;
  auto minY = curve.GetMin(&minX);
  float maxX;
  auto maxY = curve.GetMax(&maxX);
  d.fMinX = 0;
  d.fMaxX = fs / 2-1;
  plot.AddWaveform(&d);
  auto thePlotter = plot.getPlot();
  thePlotter.SetAxisRange(SP_YAXIS, minY, maxY);
  thePlotter.ReDraw();

  EzPlot::PrintFakeScreenToScreen(plot.getFakeScreen(), std::cout);
  LOG_TRACE((""));

 */

class EzPlot {
public:
  typedef struct ScreenYTag {
    std::vector<uint8_t> y;
  } ScreenY;

  typedef struct FakeScreenTag {
    std::vector<ScreenY> x;
    void* pUserData = nullptr;
  } FakeScreen;

private:
  static void test_SetPixel(
    struct SP_GfxApiTag* pGP,
    SP_IntT nX,
    SP_IntT nY,
    SP_ColourT nColour) {
    FakeScreen* pScreen =
      (FakeScreen*)pGP->pUserData;
    FakeScreen& s = *pScreen;
    if (nX >= s.x.size()) {
      s.x.resize(nX + 1);
    }
    auto& y = s.x[ nX ].y;
    if (nY >= y.size()) {
      y.resize(nY + 1);
    }

    s.x[ nX ].y[ nY ] = nColour;
  }

public:
  static void
    PrintFakeScreenToScreen(FakeScreen& s, std::ostream& o) {
    std::vector<std::string> yArr;
    int maxY         = 0;
    const auto xSize = s.x.size();
    for (int x = 0; x < xSize; x++) {
      maxY = MAX(maxY, s.x[ x ].y.size());
    }

    yArr.resize(maxY);
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < maxY; y++) {
        const char cX =
          (s.x[ x ].y[ y ]) ? '*' : ' ';
        yArr[ y ] += cX;
      }
    }

    for (int y = 0; y < maxY; y++) {
      o << yArr[ y ] << std::endl;
    }
  }

public:
  EzPlot()
    : plot(0, 0)
    , screen()
    , gfx()
    , minX(100000000)
    , maxX(-100000000)
    , nId(1) {
    gfx.nWidth    = 100;
    gfx.nHeight   = 40;
    gfx.SetPixel  = test_SetPixel;
    gfx.pUserData = &screen;

    plot.Init(&gfx);
    plot.fillWithBackgroundColor = true;
    plot.SetBackgroundColour(0);
  }

  ~EzPlot() {
    auto iter      = dataSets.begin();
    const auto end = dataSets.end();
    while (iter != end) {
      SP_DataSetT* pD = *iter;
      plot.RemoveWaveform(pD->nID);
      iter++;

      delete pD;
    }
  }

  FakeScreen& getFakeScreen() {
    return screen;
  }

  void AddWaveform(SP_DataSetT* pDataSet, const SP_PenT* pPen = nullptr) {
    plot.AddWaveform(pDataSet, pPen);
    if (maxX < minX) {
      minX = pDataSet->fMinX;
      maxX = pDataSet->fMaxX;
    } else {
      minX = MIN(pDataSet->fMinX, minX);
      maxX = MAX(pDataSet->fMaxX, maxX);
    }
    plot.SetAxisRange(SP_XAXIS, minX, maxX);
    if (plot.YAxis.fMax <= plot.YAxis.fMin) {
      plot.SetAxisRange(SP_YAXIS, -100, 100);
    }
  }

  SimplePlot& getPlot() {
    return plot;
  }

  SP_DataSetT& GenerateDataset(
    pFnGetYValue cb,
    std::string label = "",
    void* pUserData   = nullptr) {
    SP_DataSetT* pD = new SP_DataSetT;
    dataSets.push_back(pD);
    SP_DataSetT& d = *pD;

    d.fMinX = MIN(0, this->minX);
    d.fMaxX = MAX(0, this->maxX);
    d.GetYValue =
      [](struct SP_DataSetTag* pDS, SP_FloatT fX, SP_FloatT* pfY) {
        *pfY = fX;
        return SPLOT_SUCCESS;
      };
    if (cb) {
      d.GetYValue = cb;
    }
    d.nID       = nId++;
    d.pUserData = pUserData;
    strncpy(d.szDataSetLabel, label.c_str(), ARRSZN(d.szDataSetLabel));
    return d;
  }

private:
  SimplePlot plot;
  FakeScreen screen;
  SP_GfxApiT gfx;
  double minX;
  double maxX;
  int nId;
  std::vector<SP_DataSetT*> dataSets;
};


#endif
