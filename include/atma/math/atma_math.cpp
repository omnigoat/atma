//=====================================================================
//
//  ATMA Math Definitions
//  -----------------------
//    This file is predominately used to define certain things, mainly
//    the externally declared variables in atma_math__settings.h.
//
//=====================================================================
#include <boost/shared_ptr.hpp>
//=====================================================================
#include <atma_math.h>
#include <atma_math_settings.h>
//=====================================================================
using namespace atma::math;

//=====================================================================
// Definitions for atma_math__settings.h
// --------------------------------------
//  The following are the definitions for the externally declared 
//  variables in atma_math__settings.h
//=====================================================================
// default: row
AE_MATRIX_MAJORITY atma::math::Setting_MatrixMajority = ROW;

// default: d3d styles
SVector<3> atma::math::Setting_LocalAxisX = AxisRight;
SVector<3> atma::math::Setting_LocalAxisY = AxisUp;
SVector<3> atma::math::Setting_LocalAxisZ = AxisForwards;

// default: d3d styles
SVector<3> atma::math::Setting_RemoteAxisX = AxisRight;
SVector<3> atma::math::Setting_RemoteAxisY = AxisUp;
SVector<3> atma::math::Setting_RemoteAxisZ = AxisForwards;

SMatrix<4> atma::math::Setting_AxisMappingMatrix = SMatrix<4>(IDENTITY);

// default: opengl styles
float atma::math::Setting_ProjectionMappingNear = -1;
float atma::math::Setting_ProjectionMappingFar = 1;


//=====================================================================

//=====================================================================
float atma::math::RandomFloat(float lower_bound, float upper_bound, float precision)
{
	// rand() only goes up to 32675 or whatever, so 5 is maximum
	if (precision > 4) precision = 4;
	
	float f = (rand() / float(RAND_MAX)) * (upper_bound - lower_bound);
	return lower_bound + f;
}

