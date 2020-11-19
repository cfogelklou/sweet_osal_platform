/**

  Code that I wrote many many moons ago.
	Could be useful now.
	Does not match coding style as it was originally a C/h module.
*/
#ifndef __SIMPLEPLOT_H__
#define __SIMPLEPLOT_H__ 1



#define SPLOT_SUCCESS   0
#define SPLOT_ERROR	-1


#define SP_MAX_DATASETS 16	// Maximum number of datasets in a plot
#define SP_LABEL_LEN	64	// Length of a label in characters

#define SP_PATTERN_SOLID 0xffffffff


typedef double 			SP_FloatT;
typedef unsigned int	SP_UIntT;
typedef int				SP_IntT;
typedef char			SP_Int8T;
typedef unsigned char	SP_UInt8T;

typedef SP_UIntT 		SP_ColourT;
typedef SP_UIntT 		SP_PatternT;

#include <math.h>

// logarithm
//#define SP_LOG10(x)		log10(x)
//#define SP_POW10(x)		pow(10.0,x)

// logarithm of an arbitrary base
#define SP_LOGB(b,x)	(log10(x)/log10(b))
#define SP_POW(b,x)		pow((b), (x))

#define SP_CEIL(x)		ceil(x)
#define SP_FLOOR(x)		floor(x)
//#endif

//---------------------------------------------
// Pen type - used to draw lines.
typedef struct SP_PenTag
{
	SP_ColourT 	nColour;
	SP_PatternT	nPattern;
} SP_PenT;

typedef struct SP_BoundingBoxTag
{
	SP_IntT		nX;
	SP_IntT		nY;
	SP_IntT		nWidth;
	SP_IntT		nHeight;
} SP_BoundingBoxT;

//---------------------------------------------
// Graphics port information - you must at least fill in the SetPixel,
// GetWidth, and GetHeight functions to allow the graphics port to work.
// DrawPenLine and FillRegion will be filled in for you if they are initialized to NULL.
// If they exist, use your platform's optimized DrawPenLine and FillRegion functions.
// pUserData is a user defined pointer that will be passed back to all
// of the functions, when called.
struct SP_GfxApiTag;

typedef void 	(*pFnSetPixel)	(struct SP_GfxApiTag *pGP, SP_IntT nX,  SP_IntT nY, SP_ColourT nColour);
typedef void    (*pFnDrawLine)	(struct SP_GfxApiTag *pGP, SP_IntT nX1, SP_IntT nY1, SP_IntT nX2, SP_IntT nY2, SP_PenT *pPen);
typedef void 	(*pFnFillRegion)(struct SP_GfxApiTag *pGP, SP_BoundingBoxT *pBoundingBox, SP_ColourT nColour);

typedef struct SP_GfxApiTag
{
	SP_IntT					nWidth;		// Current canvas width, in pixels.  Used for all drawing functions.
	SP_IntT					nHeight;	// Current canvas height, in pixels.  Used for all drawing functions.
	pFnSetPixel				SetPixel;	// Application-provided SetPixel function
	pFnDrawLine				DrawLine;	// Optional application-provided DrawLine function
	pFnFillRegion			FillRegion;	// Optional application-provided FillRegion function
	void 				*	pUserData;	// Application-defined pointer to be passed back to all of the above functions.
} SP_GfxApiT;



//---------------------------------------------
// Information for a data set.
struct SP_DataSetTag;

// Function used by the plot library to obtain each Y value for a corresponding X value.
// Must return SPLOT_SUCCESS to continue plotting.
typedef SP_IntT (*pFnGetYValue)(struct SP_DataSetTag *pDS, SP_FloatT fX, SP_FloatT *pfY);

typedef struct SP_DataSetTag
{
	SP_Int8T		nID;							// Unique ID for the dataset - used for removal.
	SP_Int8T		szDataSetLabel[SP_LABEL_LEN];	// Label for the data set.
	pFnGetYValue	GetYValue;						// Get the Y value for a given X
	SP_FloatT		fMinX;							// Don't bother trying to get any data points below fMinX
	SP_FloatT		fMaxX;							// Don't bother trying to get any data points above fMinY
    void           *pUserData;                      // For application use.
} SP_DataSetT;


//---------------------------------------------
// Information stored by the simple plot engine so it knows how to draw each plot
struct SP_DataSetPlotTag;

typedef struct SP_DataSetPlotTag
{
	SP_DataSetT		*pDS;		// Pointer to the actual dataset info.
	SP_PenT			Pen;
} SP_DataSetPlotT;


//---------------------------------------------
// Information needed for each axis
typedef enum
{
	SP_SCALETYPE_LIN = 0,
	SP_SCALETYPE_LOG
} SP_ScaleTypeT;

typedef struct GridDataTag
{
	SP_IntT			nEn;	// Nonzero to draw the grid
	SP_FloatT		fUnit;	// Indicates the grid separation.  (When logarithmic, set to 1.0 for major, 0.1 for minor, to generate a typical logarithmic graph.)
	SP_PenT			Pen;	// Pen to use to draw the grid
} SP_GridDataT;

struct SP_AxisTag;

typedef struct SP_AxisTag
{
	SP_Int8T			szLabel[SP_LABEL_LEN];
	SP_ScaleTypeT		nScaleType;
	SP_FloatT			fMin;       // Plot starts at fMin
	SP_FloatT			fMax;       // Plot ends at fMax
	SP_GridDataT		MajorGrid;	// Major grid info
	SP_GridDataT		MinorGrid;	// Minor grid info
} SP_AxisT;

//---------------------------------------------
// Information needed for the entire plot
struct SP_PlotTag;

typedef enum
{
	SP_XAXIS	= 0,
	SP_YAXIS
} SPAxisPickerT;

typedef enum
{
	SP_MINORGRID = 0,
	SP_MAJORGRID
} SPGridPickerT;

// Callback called when the grid is redrawn - allows the
// application to label the grids.
typedef void (*pFnGridlineCallback)(
        void *pUserData,
        SPAxisPickerT axis,
        SPGridPickerT gridtype,
        SP_IntT   nPixel,
        SP_FloatT fValue );

typedef struct SP_PlotTag
{
	SP_Int8T			szPlotLabel[SP_LABEL_LEN];
	SP_GfxApiT *		pGP;							// Used for drawing
	SP_ColourT 			nBackgroundColour;				// Background colour
	SP_DataSetPlotT	 	DataSets[SP_MAX_DATASETS];		// Array of data sets
	SP_IntT				nDataSets;						// Current number of data sets
	SP_AxisT			XAxis;							// Information for the X axis
	SP_AxisT			YAxis;							// Information for the Y axis
	SP_IntT				bDirtyFlag;						// Dirty flag
	pFnGridlineCallback GridlineCallback;              // To be called for each gridline.
	void                *pUserData;
} SP_PlotT;


#if defined( __cplusplus )
extern "C" {
#endif

//---------------------------------------------
SP_IntT SP_Init(SP_PlotT *pPlot, SP_GfxApiT *pGP, SP_ColourT nBackgroundColour);

//---------------------------------------------
SP_FloatT	SP_PixelToValue(SP_PlotT *pPlot,  SPAxisPickerT nAxis, SP_IntT nPixelValue);

//---------------------------------------------
SP_IntT		SP_ValueToPixel(SP_PlotT *pPlot,  SPAxisPickerT nAxis, SP_FloatT fValue);

//---------------------------------------------
typedef void (*pFnForEachDataSetCallback)(SP_DataSetPlotT *pDataSetPlot, void *pUserData);
void	SP_ForEachDataSet(SP_PlotT *pPlot, pFnForEachDataSetCallback pCB, void *pUserData);

//---------------------------------------------
void 	SP_CopyAxis(SP_PlotT *pPlot, SPAxisPickerT nAxis, SP_AxisT *pSrcAxis);

//---------------------------------------------
void 	SP_SetAxisLabel(SP_PlotT *pPlot, SPAxisPickerT nAxis, char *szNewLabel);

//---------------------------------------------
void 	SP_SetAxisScaleType(SP_PlotT *pPlot, SPAxisPickerT nAxis, SP_ScaleTypeT nScaleType);

//---------------------------------------------
void 	SP_SetAxisRange(SP_PlotT *pPlot, SPAxisPickerT nAxis, SP_FloatT fMin, SP_FloatT fMax);

//---------------------------------------------
void 	SP_SetGridEnable(SP_PlotT *pPlot, SPAxisPickerT nAxis, SP_IntT nEnableMajorGrid, SP_IntT nEnableMinorGrid);

//---------------------------------------------
void 	SP_SetGridUnits(SP_PlotT *pPlot, SPAxisPickerT nAxis, SP_FloatT nMajorUnits, SP_FloatT nMinorUnits);

//---------------------------------------------
void 	SP_SetGridPen(SP_PlotT *pPlot, SPAxisPickerT nAxis, SP_PenT *pMajorPen, SP_PenT *pMinorPen);

//---------------------------------------------
void 	SP_SetGridCallback(SP_PlotT *pPlot,
        pFnGridlineCallback GridlineCallback,
        void                *pUserData );

//---------------------------------------------
void 	SP_SetDirtyFlag(SP_PlotT *pPlot);

//---------------------------------------------
void 	SP_AddWaveform(SP_PlotT *pPlot, SP_DataSetT *pDataSet, SP_PenT *pPen);

//---------------------------------------------
void 	SP_RemoveWaveform(SP_PlotT *pPlot, SP_IntT nID);

//---------------------------------------------
void 	SP_ReDraw(SP_PlotT *pPlot);

//---------------------------------------------
// Members present in all interpolators
struct SP_InterpolatorTag;

#define SP_INTERPOLATOR_BASE_MEMBERS	\
	SP_DataSetT		DS;																\
	void 			(*AddPoint)		(void *pInterp, SP_FloatT fX, SP_FloatT fY);	\
	void 			(*ClearPoints)	(void *pInterp);						 		\
	void 			(*Recalc)		(void *pInterp);

typedef struct SP_InterpolatorApiTag
{
	SP_INTERPOLATOR_BASE_MEMBERS
} SP_InterpolatorApiT;

#define SP_INTERPOLATOR_DATA_BASE_MEMBERS	\
	SP_FloatT fXi;							\
	SP_FloatT fYi;

typedef struct SP_InterpPointDataTag
{
	SP_INTERPOLATOR_DATA_BASE_MEMBERS
} SP_InterpPointDataT;

//---------------------------------------------
// Generic "Add points" function - can operate on any interpolator with the SP_InterpolatorApiT API
void SP_InterpolatorAddPoints(void *pAPI, SP_InterpPointDataT *pfXY, SP_IntT nPoints);

//---------------------------------------------
// Generic "Add points" function - can operate on any interpolator with the SP_InterpolatorApiT API
void SP_InterpolatorAddXYPoints(void *pAPI, SP_FloatT *pfX, SP_FloatT *pfY, SP_IntT nPoints);

//---------------------------------------------
// Linear interpolator
typedef struct SP_LinInterpPointDataTag
{
	SP_INTERPOLATOR_DATA_BASE_MEMBERS
	SP_FloatT fM;
	SP_FloatT fB;
} SP_LinInterpPointDataT;

typedef struct SP_LinInterpTag
{
	SP_INTERPOLATOR_BASE_MEMBERS
	SP_LinInterpPointDataT *pPoints;
	SP_IntT					nPoints;
	SP_IntT					nMaxPoints;
} SP_LinInterpT;

void SP_LinInterpInit(SP_LinInterpT *pLinInterp, SP_LinInterpPointDataT *pPointBuf, SP_IntT nMaxPoints);

#if defined( __cplusplus )
}
#endif

#endif // #define __SIMPLEPLOT_H__

