

#include "simple_plot.hpp"
#include "simple_plot/simple_plot_interp.hpp"

#include <string.h>

#include "utils/platform_log.h"
#include "utils/helper_macros.h"

#define SP_ASSERT LOG_ASSERT

//---------------------------------------------
typedef struct sp_DrawGridlinesDataTag
{
  int             nPixelRange;
  int             nLineLength;
} sp_DrawGridlinesDataT;

//---------------------------------------------
typedef void (*pFnForEachGridLineCallback)(
  SimplePlot* pPlot,
  SPAxisPickerT nAxis,
  SPGridPickerT nGrid,
  SP_FloatT fGridValue,
  void* pUserData);




static void    sp_DrawLine(SP_GfxApiT* pGP, SP_IntT nStartX, SP_IntT nStartY, SP_IntT nEndX, SP_IntT nEndY, SP_PenT* pPen);
static void    sp_FillRegion(SP_GfxApiT* pGP, SP_BoundingBoxT* pBoundingBox, SP_ColourT nColour);
static SP_IntT sp_GfxApiInit(SP_GfxApiT* pGP);
static SP_IntT sp_ValueToPixel(SP_AxisT* pAxis, SP_FloatT fPixelRange, SP_FloatT fValue);
static void    sp_ForEachGridLine(SimplePlot* pPlot, SPAxisPickerT nAxis, SPGridPickerT nGrid, pFnForEachGridLineCallback pCB, void* pUserData);
static void    sp_DrawGridlinesCallback(SimplePlot* pPlot, SPAxisPickerT nAxis, SPGridPickerT nGrid, SP_FloatT fGridValue, void* pUserData);
static void    sp_DrawGridlines(SimplePlot* pPlot);
static void    sp_ReDrawCallback(SP_DataSetPlotT* pDataSetPlot, void* pUserData);
static void    sp_MemSwap(void* p1, void* p2, int nSize);
static SP_FloatT  sp_PixelToValue(SP_AxisT* pAxis, SP_IntT nPixelRange, SP_IntT nPixelValue);
static void    sp_IntSwap(SP_IntT* px, SP_IntT* py);
static SP_IntT sp_Abs(SP_IntT x);
static SP_IntT sp_Sign(SP_IntT x);
static void    sp_SortXYPoints(SP_InterpPointDataT* pPoints, SP_IntT nStructSize, SP_IntT nPoints);
static SP_IntT   sp_Round(SP_FloatT x);

//---------------------------------------------
SimplePlot::SimplePlot(SP_FloatT xMax, SP_FloatT yMax, SP_FloatT xMin, SP_FloatT yMin)
  : fillWithBackgroundColor(false)
  , drawGridlines(false)
  , szPlotLabel()
  , pGP(nullptr)
  , nBackgroundColour()
  , DataSets()
  , nDataSets()
  , XAxis()
  , YAxis()
  , bDirtyFlag(false)
  , GridlineCallback(nullptr)
  , pUserData(nullptr)
{
  Init(nullptr, 0);
  SetAxisRange(SP_XAXIS, xMin, xMax);
  SetAxisRange(SP_YAXIS, yMin, yMax);
}

//---------------------------------------------
SimplePlot::~SimplePlot() {

}

//---------------------------------------------
// Sets the background pen colour & returns the old colour
SP_ColourT SimplePlot::SetBackgroundColour(SP_ColourT nColour)
{
  auto* pPlot = this;
  SP_ColourT nOldColour;

  nOldColour = pPlot->nBackgroundColour;
  pPlot->nBackgroundColour = nColour;

  return nOldColour;
}


//---------------------------------------------
// Public version
SP_FloatT   SimplePlot::PixelToValue(SPAxisPickerT nAxis, SP_IntT nPixelValue)
{
  auto* pPlot = this;
  SP_AxisT* pAxis;
  SP_IntT         nPixelRange;
  if (nAxis == SP_XAXIS)
  {
    pAxis = &pPlot->XAxis;
    nPixelRange = pPlot->pGP->nWidth - 1;
  }
  else
  {
    pAxis = &pPlot->YAxis;
    nPixelRange = pPlot->pGP->nHeight - 1;
  }
  return sp_PixelToValue(pAxis, nPixelRange, nPixelValue);
}


//---------------------------------------------
// Get the pixel that the given value represents on the axis
SP_IntT     SimplePlot::ValueToPixel(SPAxisPickerT nAxis, SP_FloatT fValue)
{
  auto* pPlot = this;
  SP_AxisT* pAxis;
  SP_FloatT       fPixelRange;
  if (nAxis == SP_XAXIS)
  {
    pAxis = &pPlot->XAxis;
    fPixelRange = (SP_FloatT)pPlot->pGP->nWidth;
  }
  else
  {
    pAxis = &pPlot->YAxis;
    fPixelRange = (SP_FloatT)pPlot->pGP->nHeight;
  }
  return sp_ValueToPixel(pAxis, fPixelRange, fValue);

}



//---------------------------------------------
// Call a callback function for every valid dataset in the plot.
void   SimplePlot::ForEachDataSet(pFnForEachDataSetCallback pCB, void* pUserData)
{
  auto* pPlot = this;
  SP_IntT i;
  SP_IntT iDS;

  if (!(pCB)) return;

  // For each dataset, call the callback function.
  i = iDS = 0;

  // Do until we've processed every registered dataset.
  while ((iDS < pPlot->nDataSets) && (i < SP_MAX_DATASETS))
  {
    SP_DataSetPlotT* pDataSet = &pPlot->DataSets[i];
    if (pDataSet->pDS)
    {
      if (pDataSet->pDS->nID >= 0)
      {
        pCB(pDataSet, pUserData);
        iDS++;
      }
      i++;
    }
  }

}



//---------------------------------------------
// Init the X and Y axis
void SimplePlot::CopyAxis(SPAxisPickerT nAxis, SP_AxisT* pSrcAxis)
{
  auto* pPlot = this;
  SP_AxisT* pDestAxis = (nAxis == SP_XAXIS) ? &pPlot->XAxis : &pPlot->YAxis;
  memcpy(pDestAxis, pSrcAxis, sizeof(SP_AxisT));
  SetAxisScaleType(nAxis, pDestAxis->nScaleType);

}


//---------------------------------------------
// Set the axis label.
void SimplePlot::SetAxisLabel(SPAxisPickerT nAxis, char* szNewLabel)
{
  auto* pPlot = this;
  SP_AxisT* pDestAxis = (nAxis == SP_XAXIS) ? &pPlot->XAxis : &pPlot->YAxis;
  memcpy(pDestAxis->szLabel, szNewLabel, SP_LABEL_LEN - 1);
  pDestAxis->szLabel[SP_LABEL_LEN - 1] = 0;
  pPlot->bDirtyFlag = 1;
}



//---------------------------------------------
// Set the axis scale type.
void SimplePlot::SetAxisScaleType(SPAxisPickerT nAxis, SP_ScaleTypeT nScaleType)
{
  auto* pPlot = this;
  SP_AxisT* pDestAxis = (nAxis == SP_XAXIS) ? &pPlot->XAxis : &pPlot->YAxis;
  pDestAxis->nScaleType = nScaleType;

  // Do some checking.
  if (pDestAxis->fMax < pDestAxis->fMin)
  {
    SP_FloatT temp = pDestAxis->fMax;
    pDestAxis->fMax = pDestAxis->fMin;
    pDestAxis->fMin = temp;
  }
  if (pDestAxis->nScaleType == SP_SCALETYPE_LOG)
  {
    if (pDestAxis->MajorGrid.fUnit <= 0)
    {
      pDestAxis->MajorGrid.fUnit = 10.0;
    }
    if (pDestAxis->fMin <= 0)
    {
      pDestAxis->fMin = 0.00001;
    }
  }
  pPlot->bDirtyFlag = 1;
}



//---------------------------------------------
// Set the axis scale type.
void SimplePlot::SetAxisRange(SPAxisPickerT nAxis, SP_FloatT fMin, SP_FloatT fMax)
{
  auto* pPlot = this;
  SP_AxisT* pDestAxis = (nAxis == SP_XAXIS) ? &pPlot->XAxis : &pPlot->YAxis;
  pDestAxis->fMin = fMin;
  pDestAxis->fMax = fMax;
  SetAxisScaleType(nAxis, pDestAxis->nScaleType);
}


//---------------------------------------------
void    SimplePlot::SetGridEnable(SPAxisPickerT nAxis, SP_IntT nEnableMajorGrid, SP_IntT nEnableMinorGrid)
{
  auto* pPlot = this;
  SP_AxisT* pDestAxis = (nAxis == SP_XAXIS) ? &pPlot->XAxis : &pPlot->YAxis;
  pDestAxis->MajorGrid.nEn = nEnableMajorGrid;
  pDestAxis->MinorGrid.nEn = nEnableMinorGrid;
  pPlot->bDirtyFlag = 1;
}



//---------------------------------------------
void    SimplePlot::SetGridUnits(SPAxisPickerT nAxis, SP_FloatT fMajorUnits, SP_FloatT fMinorUnits)
{
  auto* pPlot = this;
  SP_AxisT* pDestAxis = (nAxis == SP_XAXIS) ? &pPlot->XAxis : &pPlot->YAxis;
  pDestAxis->MajorGrid.fUnit = fMajorUnits;
  pDestAxis->MinorGrid.fUnit = fMinorUnits;
  pPlot->bDirtyFlag = 1;
}



//---------------------------------------------
void    SimplePlot::SetGridPen(SPAxisPickerT nAxis, SP_PenT* pMajorPen, SP_PenT* pMinorPen)
{
  auto* pPlot = this;
  SP_AxisT* pDestAxis = (nAxis == SP_XAXIS) ? &pPlot->XAxis : &pPlot->YAxis;
  if (pMajorPen)
  {
    memcpy(&pDestAxis->MajorGrid.Pen, pMajorPen, sizeof(SP_PenT));
  }
  if (pMinorPen)
  {
    memcpy(&pDestAxis->MinorGrid.Pen, pMinorPen, sizeof(SP_PenT));
  }
  pPlot->bDirtyFlag = 1;
}


//---------------------------------------------
void SimplePlot::SetGridCallback(
  pFnGridlineCallback GridlineCallback,
  void* pUserData)
{
  auto* pPlot = this;
  pPlot->GridlineCallback = GridlineCallback;
  pPlot->pUserData = pUserData;
}



//---------------------------------------------
// Set the "plot is dirty" flag.
void SimplePlot::SetDirtyFlag()
{
  auto* pPlot = this;
  pPlot->bDirtyFlag = 1;
}


static const SP_PenT defaultPen = {
  1,
  SP_PATTERN_SOLID
};

//---------------------------------------------
// Add a waveform
void  SimplePlot::AddWaveform(SP_DataSetT* pDataSet, const SP_PenT* pPen)
{
  pPen = (pPen) ? pPen : &defaultPen;
  auto* pPlot = this;
  if (pPlot->nDataSets < SP_MAX_DATASETS)
  {
    SP_DataSetPlotT* pNewPlot = &pPlot->DataSets[pPlot->nDataSets];
    pNewPlot->pDS = pDataSet;
    memcpy(&pNewPlot->Pen, pPen, sizeof(SP_PenT));
    pPlot->nDataSets++;
  }
}



//---------------------------------------------
// Remove a waveform
void  SimplePlot::RemoveWaveform(SP_IntT nID)
{
  auto* pPlot = this;
  int i = 0;
  int nDone = 0;
  while (!nDone && (i < SP_MAX_DATASETS))
  {
    SP_DataSetPlotT* pNewDataSet = &pPlot->DataSets[i];

    if ((pNewDataSet->pDS) && (pNewDataSet->pDS->nID == nID))
    {
      nDone = 1;

      // Shift the rest of the plots down
      for (; i < (pPlot->nDataSets - 1); i++)
      {
        memcpy(&pPlot->DataSets[i], &pPlot->DataSets[i + 1], sizeof(SP_DataSetPlotT));
      }
      pPlot->nDataSets--;

      // Set the ID of the newly unused dataset to -1.
      pPlot->DataSets[pPlot->nDataSets].pDS = nullptr;
    }
    else
    {
      i++;
    }
  }

}




//---------------------------------------------
// Redraw the plot
void  SimplePlot::ReDraw()
{
  auto* pPlot = this;
  SP_GfxApiT* pGP = pPlot->pGP;

  if (fillWithBackgroundColor) {
    SP_BoundingBoxT BoundingBox =
    {
        0, 0, pGP->nWidth, pGP->nHeight
    };
    // Draw the background colour
    pGP->FillRegion(pGP, &BoundingBox, pPlot->nBackgroundColour);
  }

  if (drawGridlines) {
    // Draw the gridlines
    sp_DrawGridlines(pPlot);
  }

  LOG_ASSERT(this->XAxis.fMax > this->XAxis.fMin);
  LOG_ASSERT(this->YAxis.fMax > this->YAxis.fMin);

  // Draw the plot data
  ForEachDataSet(sp_ReDrawCallbackC, (void*)pPlot);
}




//---------------------------------------------
// Initialize an instance of a "simple plot" device.
// Structure must be pre-allocated, and, if not pre-initialized,
// initialized to 0.
SP_IntT SimplePlot::Init(SP_GfxApiT* pGP, SP_ColourT nBackgroundColour)
{

  auto* pPlot = this;

  // Initialize dataset IDs to -1.
  for (int i = 0; i < SP_MAX_DATASETS; i++)
  {
    SP_DataSetPlotT* pDS = &pPlot->DataSets[i];
    pDS->pDS = nullptr;    // -1 indicates an invalid dataset.
  }

  if (!(pGP)) return SPLOT_ERROR;

  if (sp_GfxApiInit(pGP) > SPLOT_ERROR)
  {
    pPlot->pGP = pGP;
    pPlot->bDirtyFlag = 1;
    pPlot->nBackgroundColour = nBackgroundColour;
    return SPLOT_SUCCESS;
  }
  else
  {
    return SPLOT_ERROR;
  }

}




//---------------------------------------------
// Draw a line on the canvas using the native setpixel function.
static void sp_DrawLine(SP_GfxApiT* pGP, SP_IntT nStartX, SP_IntT nStartY, SP_IntT nEndX, SP_IntT nEndY, SP_PenT* pPen)
{
  SP_IntT nDirection;
  SP_FloatT fM, fB;

  // nDone becomes 1 when finished.
  SP_IntT nDone = 0;

  // Calculate width
  SP_IntT nWidth = nEndX - nStartX;

  // Calculate height
  SP_IntT nHeight = nEndY - nStartY;

  // Make positive
  SP_IntT nAbsWidth = sp_Abs(nWidth) + 1;
  SP_IntT nAbsHeight = sp_Abs(nHeight) + 1;

  nWidth = (nWidth < 0) ? -nAbsWidth : nAbsWidth;
  nHeight = (nHeight < 0) ? -nAbsHeight : nAbsHeight;

  // If width is greater than height, create a Y pixel for every X pixel.
  if (nAbsWidth >= nAbsHeight)
  {

    // Start from StartX
    SP_IntT nX = nStartX;

    // Calculate the direction
    nDirection = sp_Sign(nWidth);

    // Calculate the slope of the line (M) in Y = MX+B
    fM = (SP_FloatT)nHeight / (SP_FloatT)nWidth;

    // Calculate B in Y = MX+B
    fB = (SP_FloatT)nStartY - fM * (SP_FloatT)nStartX;
    do
    {

      // Calculate Y for this value of X
      SP_FloatT fY = fM * (SP_FloatT)nX + fB;

      // Only draw a pixel if the pattern tells us to.
      if (pPen->nPattern & 0x01)
      {

        // Round Y to an integer.
        SP_IntT nY = sp_Round(fY);

        // Check range... Just in case.
        if ((nX >= 0) && (nX <= pGP->nWidth - 1) && (nY >= 0) && (nY <= pGP->nHeight - 1))
        {
          pGP->SetPixel(pGP, nX, nY, pPen->nColour);
        }
      }

      // Shift pattern left.
      pPen->nPattern = ((pPen->nPattern >> 31) & 0x01) | (pPen->nPattern << 1);
      if (nX != nEndX)
      {
        nX += nDirection;
      }
      else
      {
        nDone = 1;
      }
    } while (!nDone);
  }
  else
  {

    // Start from StartX
    SP_IntT nY = nStartY;

    // Calculate direction
    nDirection = sp_Sign(nHeight);

    // Calculate vars for Y=MX+B equation.
    fM = (SP_FloatT)nWidth / (SP_FloatT)nHeight;
    fB = (SP_FloatT)nStartX - fM * (SP_FloatT)nStartY;
    do
    {

      // Generate a new X value for this Y value.
      SP_FloatT fX = fM * (SP_FloatT)nY + fB;

      // If the pattern tells us to draw, then draw.
      if (pPen->nPattern & 0x01)
      {

        // Round X.
        SP_IntT nX = sp_Round(fX);

        // Check range before drawing.
        if ((nX >= 0) && (nX <= pGP->nWidth - 1) && (nY >= 0) && (nY <= pGP->nHeight - 1))
        {
          pGP->SetPixel(pGP, nX, nY, pPen->nColour);
        }
      }

      // Shift pattern left.
      pPen->nPattern = ((pPen->nPattern >> 31) & 0x01) | (pPen->nPattern << 1);

      if (nY != nEndY)
      {
        nY += nDirection;
      }
      else
      {
        nDone = 1;
      }
    } while (!nDone);
  }
}


//---------------------------------------------
// Fills the region specified in nStartX, nStartY, nEndX, nEndY
static void sp_FillRegion(SP_GfxApiT* pGP, SP_BoundingBoxT* pBoundingBox, SP_ColourT nColour)
{
  SP_IntT x;

  SP_IntT nEndX = pBoundingBox->nX + pBoundingBox->nWidth - 1;
  SP_IntT nEndY = pBoundingBox->nY + pBoundingBox->nHeight - 1;
  SP_PenT Pen = { nColour, SP_PATTERN_SOLID };

  // Fill the region by drawing vertical lines.
  for (x = pBoundingBox->nX; x <= nEndX; x++)
  {
    pGP->DrawLine(pGP, x, pBoundingBox->nY, x, nEndY, &Pen);
  }
}

//---------------------------------------------
static SP_IntT sp_GfxApiInit(SP_GfxApiT* pGP)
{
  if (!(pGP)) return SPLOT_ERROR;

  // Check mandatory functions
  if (!(pGP->SetPixel)) return SPLOT_ERROR;

  // Fill in optional functions if they don't exist.
  if (!(pGP->DrawLine))   pGP->DrawLine = sp_DrawLine;
  if (!(pGP->FillRegion)) pGP->FillRegion = sp_FillRegion;

  return SPLOT_SUCCESS;
}


//---------------------------------------------
// Private...
static SP_IntT     sp_ValueToPixel(SP_AxisT* pAxis, SP_FloatT fPixelRange, SP_FloatT fValue)
{
  if (fValue < pAxis->fMin)
  {
    return -1;
  }
  else if (fValue > pAxis->fMax)
  {
    return -1;
  }
  else if (pAxis->nScaleType == SP_SCALETYPE_LIN)
  {
    const SP_FloatT fRealRange = pAxis->fMax - pAxis->fMin;
    LOG_ASSERT(fRealRange > 0);
    SP_FloatT fRatio = (fValue - pAxis->fMin) / fRealRange;
    return (SP_IntT)(fRatio * fPixelRange);
  }
  else
  {
    const SP_FloatT fRealRange = SP_LOGB(pAxis->MajorGrid.fUnit, pAxis->fMax) - SP_LOGB(pAxis->MajorGrid.fUnit, pAxis->fMin);
    LOG_ASSERT(fRealRange > 0);
    SP_FloatT fRatio = (SP_LOGB(pAxis->MajorGrid.fUnit, fValue) - SP_LOGB(pAxis->MajorGrid.fUnit, pAxis->fMin)) / fRealRange;
    return (SP_IntT)(fRatio * fPixelRange);
  }

}


static void    sp_ForEachGridLine(SimplePlot* pPlot, SPAxisPickerT nAxis, SPGridPickerT nGrid, pFnForEachGridLineCallback pCB, void* pUserData)
{
  SP_AxisT* pAxis = (nAxis == SP_XAXIS) ? &pPlot->XAxis : &pPlot->YAxis;
  SP_GridDataT* pGrid = (nGrid == SP_MAJORGRID) ? &pAxis->MajorGrid : &pAxis->MinorGrid;

  SP_ASSERT(pPlot);
  if (!(pCB)) return;

  // For each gridline, call the callback function and pass back some useful data.
  // What are we doing here...
  if (pAxis->nScaleType == SP_SCALETYPE_LIN)
  {

    // Figure out where the grids should start.  We assume that the grid always starts at zero.
    SP_FloatT fGridValue = pGrid->fUnit * SP_CEIL(pAxis->fMin / pGrid->fUnit);
    while (fGridValue < pAxis->fMax)
    {
      pCB(pPlot, nAxis, nGrid, fGridValue, pUserData);
      fGridValue += pGrid->fUnit;
    }
  }
  else
  {

    // Calculate the lowest major grid unit that is an even logarithmic multiple of
    // the power indicated by the grid unit.
    //SP_FloatT fLog10GridUnit = SP_LOGB(pAxis->MajorGrid.fUnit, pAxis->MajorGrid.fUnit);
    SP_FloatT fMajorGridValue = SP_FLOOR(SP_LOGB(pAxis->MajorGrid.fUnit, pAxis->fMin));//fLog10GridUnit;
    fMajorGridValue = SP_POW(pAxis->MajorGrid.fUnit, fMajorGridValue);//fLog10GridUnit * fMajorGridValue);

    if (nGrid == SP_MAJORGRID)
    {
      // Find the first grid value that is actually within the range.
      while (fMajorGridValue < pAxis->fMin)
      {
        fMajorGridValue *= pAxis->MajorGrid.fUnit;
      }

      // Call the callback for each major grid.
      while (fMajorGridValue <= pAxis->fMax)
      {
        pCB(pPlot, nAxis, nGrid, fMajorGridValue, pUserData);
        fMajorGridValue *= pAxis->MajorGrid.fUnit;
      }
    }
    else
    {
      // Call the callback for each minor grid value grid.
      while (fMajorGridValue <= pAxis->fMax)
      {
        SP_FloatT fNextMajorGridValue = fMajorGridValue * pAxis->MajorGrid.fUnit;
        SP_FloatT fMinorGridUnit = (fNextMajorGridValue * pAxis->MinorGrid.fUnit);
        SP_FloatT fMinorGridValue = fMinorGridUnit;
        while (fMinorGridValue < fNextMajorGridValue)
        {
          pCB(pPlot, nAxis, nGrid, fMinorGridValue, pUserData);
          fMinorGridValue += fMinorGridUnit;
        }
        fMajorGridValue = fNextMajorGridValue;
      }
    }
  }
}

static void sp_DrawGridlinesCallback(SimplePlot* pPlot, SPAxisPickerT nAxis, SPGridPickerT nGrid, SP_FloatT fGridValue, void* pUserData)
{

  sp_DrawGridlinesDataT* pData = (sp_DrawGridlinesDataT*)pUserData;
  SP_AxisT* pAxis = (nAxis == SP_XAXIS) ? &pPlot->XAxis : &pPlot->YAxis;
  SP_GridDataT* pGrid = (nGrid == SP_MAJORGRID) ? &pAxis->MajorGrid : &pAxis->MinorGrid;
  int nGridPixel = sp_ValueToPixel(pAxis, (SP_FloatT)pData->nPixelRange, fGridValue);

  if (nAxis == SP_XAXIS)
  {
    pPlot->pGP->DrawLine(pPlot->pGP, nGridPixel, 0, nGridPixel, pData->nLineLength - 1, &pGrid->Pen);
  }
  else
  {
    pPlot->pGP->DrawLine(pPlot->pGP, 0, nGridPixel, pData->nLineLength - 1, nGridPixel, &pGrid->Pen);
  }

  // If registered, call the application's gridline callback too.
  if (pPlot->GridlineCallback)
  {
    pPlot->GridlineCallback(
      pPlot->pUserData,
      nAxis,
      nGrid,
      nGridPixel,
      fGridValue);
  }

}


//---------------------------------------------
// Draw the gridlines.
static void sp_DrawGridlines(SimplePlot* pPlot)
{
  int                     nGrid;
  //  fstream                 fGridFile;
  sp_DrawGridlinesDataT   GridData;

  // Do the X Axis
  GridData.nLineLength = pPlot->pGP->nHeight;
  GridData.nPixelRange = pPlot->pGP->nWidth - 1;
  for (nGrid = SP_MINORGRID; nGrid <= SP_MAJORGRID; nGrid++)
  {
    SP_GridDataT* pGrid = (nGrid == SP_MAJORGRID) ? &pPlot->XAxis.MajorGrid : &pPlot->XAxis.MinorGrid;
    if (pGrid->nEn)
    {
      sp_ForEachGridLine(pPlot, SP_XAXIS, (SPGridPickerT)nGrid, sp_DrawGridlinesCallback, &GridData);
    }
  }

  // Do the Y Axis
  GridData.nLineLength = pPlot->pGP->nWidth;
  GridData.nPixelRange = pPlot->pGP->nHeight - 1;
  for (nGrid = SP_MINORGRID; nGrid <= SP_MAJORGRID; nGrid++)
  {
    SP_GridDataT* pGrid = (nGrid == SP_MAJORGRID) ? &pPlot->YAxis.MajorGrid : &pPlot->YAxis.MinorGrid;
    if (pGrid->nEn)
    {
      sp_ForEachGridLine(pPlot, SP_YAXIS, (SPGridPickerT)nGrid, sp_DrawGridlinesCallback, &GridData);
    }
  }

}

//---------------------------------------------
// Redraw the dataset.
void SimplePlot::sp_ReDrawCallbackC(SP_DataSetPlotT* pDataSetPlot, void* pUserData)
{
  SimplePlot* pPlot = (SimplePlot*)pUserData;
  pPlot->sp_ReDrawCallback(pDataSetPlot);
}

void SimplePlot::sp_ReDrawCallback(SP_DataSetPlotT* pDataSetPlot)
{
  SimplePlot* pPlot = this;
  SP_IntT                 nSuccess;
  SP_FloatT               fXValue, fYValue;
  SP_IntT                 nThisXPixel, nLastXPixel, nThisYPixel, nLastYPixel;
  
  SP_GfxApiT* pGP = pPlot->pGP;
  SP_DataSetT* pDS = pDataSetPlot->pDS;
  SP_IntT                 nXAxisPixelRange = pGP->nWidth-1;
  SP_IntT                 nYAxisPixelRange = pGP->nHeight-1;

  SP_FloatT               fMinX = MIN(pDS->fMinX, pPlot->XAxis.fMin);
  SP_FloatT               fMaxX = MAX(pDS->fMaxX, pPlot->XAxis.fMax);

  SP_IntT nStartXPixel = sp_ValueToPixel(&pPlot->XAxis, (SP_FloatT)nXAxisPixelRange, fMinX);
  SP_IntT nEndXPixel = sp_ValueToPixel(&pPlot->XAxis, (SP_FloatT)nXAxisPixelRange, fMaxX);  

  if ((nStartXPixel < 0) || (nEndXPixel < 0))
  {
    return;
  }
  else
  {

    // Get initial conditions
    nLastXPixel = nStartXPixel;
    nThisXPixel = (drawGridlines) ? nLastXPixel + 1 : nLastXPixel;
    nSuccess = (pDS->GetYValue(pDS, fMinX, &fYValue) == SPLOT_SUCCESS);

    if (nSuccess)
    {
      // Initial Y condition
      nLastYPixel = nThisYPixel = sp_ValueToPixel(&pPlot->YAxis, (SP_FloatT)nYAxisPixelRange, fYValue);

      // Loop through, getting the Y values for the X values corresponding to each X pixel.
      while ((nSuccess) && (nThisXPixel <= nEndXPixel))
      {
        fXValue = sp_PixelToValue(&pPlot->XAxis, nXAxisPixelRange, nThisXPixel);
        nSuccess = (pDS->GetYValue(pDS, fXValue, &fYValue) == SPLOT_SUCCESS);
        if (nSuccess)
        {
          nLastYPixel = nThisYPixel;
          nThisYPixel = sp_ValueToPixel(&pPlot->YAxis, (SP_FloatT)nYAxisPixelRange, fYValue);
          if (nThisYPixel >= 0)
          {
            if (nLastYPixel >= 0)
            {
              pGP->DrawLine(pGP, nLastXPixel, nLastYPixel, nThisXPixel, nThisYPixel, &pDataSetPlot->Pen);
            }
            else
            {
              pGP->SetPixel(pGP, nThisXPixel, nThisYPixel, pDataSetPlot->Pen.nColour);
            }
          }
          nLastXPixel = nThisXPixel;
          nThisXPixel++;
        }
      }

      nThisXPixel = nEndXPixel;

      // Draw the final line to the value represented by the maximum X range.
      if (nSuccess)
      {
        // Draw the last line.
        nSuccess = (pDS->GetYValue(pDS, fMaxX, &fYValue) == SPLOT_SUCCESS);
        if (nSuccess)
        {
          nLastYPixel = nThisYPixel;
          nThisYPixel = sp_ValueToPixel(&pPlot->YAxis, (SP_FloatT)nYAxisPixelRange, fYValue);
          if (nThisYPixel >= 0)
          {
            if (nLastYPixel >= 0)
            {
              pGP->DrawLine(pGP, nLastXPixel, nLastYPixel, nThisXPixel, nThisYPixel, &pDataSetPlot->Pen);
            }
            else
            {
              pGP->SetPixel(pGP, nThisXPixel, nThisYPixel, pDataSetPlot->Pen.nColour);
            }
          }
        }
      }
    }
  }
}


//---------------------------------------------
static void sp_MemSwap(void* p1, void* p2, int nSize)
{
  char temp;
  char* pb1 = (char*)p1;
  char* pb2 = (char*)p2;
  while (nSize)
  {
    temp = *pb1;
    *pb1++ = *pb2;
    *pb2++ = temp;
    nSize--;
  }
}

//---------------------------------------------
// Private...
static SP_FloatT  sp_PixelToValue(SP_AxisT* pAxis, SP_IntT nPixelRange, SP_IntT nPixelValue)
{
  if (nPixelValue <= 0)
  {
    return pAxis->fMin;
  }
  else if (nPixelValue >= nPixelRange)
  {
    return pAxis->fMax;
  }
  else if (pAxis->nScaleType == SP_SCALETYPE_LIN)
  {
    SP_FloatT fRealRange = pAxis->fMax - pAxis->fMin;
    SP_FloatT fRatio = (SP_FloatT)nPixelValue / (SP_FloatT)nPixelRange;
    return pAxis->fMin + (fRatio * fRealRange);
  }
  else
  {
    SP_FloatT fRealRange = SP_LOGB(pAxis->MajorGrid.fUnit, pAxis->fMax) - SP_LOGB(pAxis->MajorGrid.fUnit, pAxis->fMin);
    SP_FloatT fLogVal = ((SP_FloatT)nPixelValue * fRealRange / (SP_FloatT)nPixelRange) + SP_LOGB(pAxis->MajorGrid.fUnit, pAxis->fMin);
    SP_FloatT fVal = SP_POW(pAxis->MajorGrid.fUnit, fLogVal);
    return fVal;
  }

}

//---------------------------------------------
static void   sp_IntSwap(SP_IntT* px, SP_IntT* py)
{
  SP_IntT nTemp = *py;
  *py = *px;
  *px = nTemp;
}



//---------------------------------------------
static SP_IntT sp_Abs(SP_IntT x)
{
  if (x < 0) return -x;
  return x;
}


//---------------------------------------------
static SP_IntT sp_Sign(SP_IntT x)
{
  if (x < 0) return -1;
  return 1;
}


//---------------------------------------------
static SP_IntT   sp_Round(SP_FloatT x)
{
  SP_FloatT fTheFloor = SP_FLOOR(x);

  if ((x - fTheFloor) >= 0.5)
  {
    x = fTheFloor + 1.0;
  }
  else
  {
    x = fTheFloor;
  }

  return (SP_IntT)fTheFloor;
}

//---------------------------------------------
static void sp_SortXYPoints(SP_InterpPointDataT* pPoints, SP_IntT nStructSize, SP_IntT nPoints)
{
  int i;
  int nSorted;
  SP_InterpPointDataT* pThisPoint;
  SP_InterpPointDataT* pNextPoint;
  unsigned char* pCharBuf = (unsigned char*)pPoints;

  // HAHA! Bubble sort!
  do
  {
    nSorted = 1;
    for (i = 0; i < (nPoints - 1); i++)
    {
      unsigned char* pThis = &pCharBuf[i * nStructSize];
      unsigned char* pNext = &pCharBuf[(i + 1) * nStructSize];
      pThisPoint = (SP_InterpPointDataT*)pThis;
      pNextPoint = (SP_InterpPointDataT*)pNext;

      SP_ASSERT(pThisPoint->fXi != pNextPoint->fXi);  // Should never have equal points

      if (pThisPoint->fXi > pNextPoint->fXi)
      {
        sp_MemSwap(pThisPoint, pNextPoint, nStructSize);
        nSorted = 0;
      }
    }
  } while (!(nSorted));

}

//---------------------------------------------
// Generic "Add points" function - can operate on any interpolator with the SP_InterpolatorApiT API
void SP_InterpolatorAddPoints(void* pAPI, SP_InterpPointDataT* pfXY, SP_IntT nPoints)
{
  int i;
  SP_InterpolatorApiT* pInterp = (SP_InterpolatorApiT*)pAPI;
  SP_ASSERT(pInterp);
  for (i = 0; i < nPoints; i++)
  {
    pInterp->AddPoint(pInterp, pfXY[i].fXi, pfXY[i].fYi);
  }
}

//---------------------------------------------
// Generic "Add points" function - can operate on any interpolator with the SP_InterpolatorApiT API
void SP_InterpolatorAddXYPoints(void* pAPI, SP_FloatT* pfX, SP_FloatT* pfY, SP_IntT nPoints)
{
  int i;
  SP_InterpolatorApiT* pInterp = (SP_InterpolatorApiT*)pAPI;
  SP_ASSERT(pInterp);
  for (i = 0; i < nPoints; i++)
  {
    pInterp->AddPoint(pInterp, pfX[i], pfY[i]);
  }
}



//---------------------------------------------
void SP_LinInterpAddPoint(void* pInterp, SP_FloatT fX, SP_FloatT fY)
{
  SP_LinInterpPointDataT* pNextPoint;
  SP_LinInterpT* pThis = (SP_LinInterpT*)pInterp;
  SP_ASSERT(pThis);

  if (pThis->nPoints >= pThis->nMaxPoints) return;
  pNextPoint = &pThis->pPoints[pThis->nPoints];
  pNextPoint->fXi = fX;
  pNextPoint->fYi = fY;
  pThis->nPoints++;
}

//---------------------------------------------
void SP_LinInterpClearPoints(void* pInterp)
{
  SP_LinInterpT* pThis = (SP_LinInterpT*)pInterp;
  SP_ASSERT(pThis);

  pThis->nPoints = 0;
}

//---------------------------------------------
void sp_LinInterpCalcSpline(SP_LinInterpPointDataT* pPoint, SP_LinInterpPointDataT* pNextPoint)
{
  pPoint->fM = (pNextPoint->fYi - pPoint->fYi) / (pNextPoint->fXi - pPoint->fXi);
  pPoint->fB = pPoint->fYi - (pPoint->fM * pPoint->fXi);
}

//---------------------------------------------
void SP_LinInterpRecalc(void* pInterp)
{
  int                     i;
  SP_LinInterpT* pThis = (SP_LinInterpT*)pInterp;
  SP_ASSERT(pThis);

  // Sort the points first, in case they were added out of order.
  sp_SortXYPoints((SP_InterpPointDataT*)pThis->pPoints, sizeof(SP_LinInterpPointDataT), pThis->nPoints);

  for (i = 1; i < pThis->nPoints; i++)
  {
    sp_LinInterpCalcSpline(&pThis->pPoints[i - 1], &pThis->pPoints[i]);
  }

  if (pThis->nPoints)
  {
    pThis->DS.fMinX = pThis->pPoints[0].fXi;
    pThis->DS.fMaxX = pThis->pPoints[pThis->nPoints - 1].fXi;
  }
}

//---------------------------------------------
SP_IntT SP_LinInterpInitGetYValue(SP_DataSetT* pDS, SP_FloatT fX, SP_FloatT* pfY)
{
  int i;
  SP_LinInterpT* pThis = (SP_LinInterpT*)pDS;
  SP_LinInterpPointDataT* pThisPoint = pThis->pPoints;
  SP_LinInterpPointDataT* pNextPoint = &pThis->pPoints[1];
  SP_ASSERT(pThis);

  if (fX < pThis->DS.fMinX) return SPLOT_ERROR;
  if (fX > pThis->DS.fMaxX) return SPLOT_ERROR;

  i = 0;
  do {
    if ((fX <= pNextPoint->fXi) && (fX >= pThisPoint->fXi))
    {
      *pfY = (pThisPoint->fM * fX + pThisPoint->fB);
      return SPLOT_SUCCESS;
    }
    else
    {
      pNextPoint++; pThisPoint++; i++;
    }
  } while (i < pThis->nPoints);
  return SPLOT_ERROR;
}

//---------------------------------------------
void SP_LinInterpInit(SP_LinInterpT* pLinInterp, SP_LinInterpPointDataT* pPointBuf, SP_IntT nMaxPoints)
{
  if (!(pLinInterp)) return;
  memset(pLinInterp, 0, sizeof(SP_LinInterpT));

  // Set up function pointers.
  pLinInterp->AddPoint = SP_LinInterpAddPoint;
  pLinInterp->ClearPoints = SP_LinInterpClearPoints;
  pLinInterp->Recalc = SP_LinInterpRecalc;
  pLinInterp->DS.GetYValue = SP_LinInterpInitGetYValue;

  // Set up the point buffers.
  if (!(pPointBuf)) return;
  pLinInterp->pPoints = pPointBuf;
  pLinInterp->nMaxPoints = nMaxPoints;
  pLinInterp->nPoints = 0;

}