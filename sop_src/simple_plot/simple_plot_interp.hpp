#ifndef SIMPLE_PLOT_INTERP_HPP
#define SIMPLE_PLOT_INTERP_HPP

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
void SP_InterpolatorAddPoints(void* pAPI, SP_InterpPointDataT* pfXY, SP_IntT nPoints);

//---------------------------------------------
// Generic "Add points" function - can operate on any interpolator with the SP_InterpolatorApiT API
void SP_InterpolatorAddXYPoints(void* pAPI, SP_FloatT* pfX, SP_FloatT* pfY, SP_IntT nPoints);

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
		SP_LinInterpPointDataT* pPoints;
	SP_IntT					nPoints;
	SP_IntT					nMaxPoints;
} SP_LinInterpT;

void SP_LinInterpInit(SP_LinInterpT* pLinInterp, SP_LinInterpPointDataT* pPointBuf, SP_IntT nMaxPoints);

#endif