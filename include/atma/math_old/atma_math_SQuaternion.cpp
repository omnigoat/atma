//=====================================================================
//
//	Implementation of the SQuaternion class.
//
//	NOTE: Most implementation in header file.
//
//=====================================================================
#include <cmath>
//=====================================================================
#include <atma/math/SVector.h>
#include <atma/math/SMatrix.h>
#include <atma/math/SQuaternion.h>
//=====================================================================
using namespace atma::math;
//=====================================================================

//=====================================================================
// Constructor: From Euler Angles
//=====================================================================
SQuaternion::SQuaternion(float _x_rotation, float _y_rotation, float _z_rotation)
{
	// temporary values for less calculations
	const float sin_x = sin(_x_rotation * 0.5f);
	const float cos_x = cos(_x_rotation * 0.5f);
	const float sin_y = sin(_y_rotation * 0.5f);
	const float cos_y = cos(_y_rotation * 0.5f);
	const float sin_z = sin(_z_rotation * 0.5f);
	const float cos_z = cos(_z_rotation * 0.5f);
	
	// more temporary values
	const float cos_x_by_cos_y = cos_x * cos_y;
	const float sin_x_by_sin_y = sin_x * sin_y;
	
	// calculate!
	float i =	sin_z	* cos_x_by_cos_y    -	cos_z	* sin_x_by_sin_y;
	float j =	cos_z	* sin_x *  cos_y	+	sin_z	* cos_x *  sin_y;
	float k =	cos_z	* cos_x *  sin_y	-	sin_z	* sin_x *  cos_y;
	w		=	cos_z	* cos_x_by_cos_y    +	sin_z	* sin_x_by_sin_y;
	v		=	SVector<3>(i, j, k);
	
	*this = Normalize(*this);
}

//=====================================================================
// Constructor: From 3x3 Matrix
//=====================================================================
SQuaternion::SQuaternion(const SMatrix<3>& _rotation_matrix)
{
}

//=====================================================================
// Constructor: From 4x4 Matrix
//=====================================================================
SQuaternion::SQuaternion(const SMatrix<4>& _rotation_matrix)
{
	// for easy typing
	const SMatrix<4>& m = _rotation_matrix;
	// the trace of the matrix
	float trace = 1 + m(1, 1) + m(2, 2);
	
	// if the trace is larger than zero, perform an "instant transformation"
	if (trace > 0.0f)
	{
		float S = sqrt(trace) * 2;
		w = 0.25f * trace;
		v = SVector<3>(	( m(1, 2) - m(2, 1) ) / S,
						( m(2, 0) - m(0, 2) ) / S,
						( m(0, 1) - m(1, 0) ) / S		);
	}
	
	// if not, we'll have to find which major diagonal e has the greatest
	// value, and calculate accordingly
	if (m(0, 0) > m(1, 1) && m(0, 0) > m(2, 2)) 
	{
		float S  = sqrt(1 + m(0, 0) - m(1, 1) - m(2, 2)) * 2.0f;
		w = (m(1, 2) - m(2, 1)) / S;
		v = SVector<3>(	0.25f * S,
						(m(1, 0) + m(0, 1)) / S,
						(m(0, 2) + m(2, 0)) / S);
		
	}
	else if (m(1, 1) > m(2, 2))
	{
		float S  = sqrt(1 + m(1, 1) - m(0, 0) - m(2, 2)) * 2.0f;
		w = (m(2, 0) - m(0, 2)) / S;
		v = SVector<3>(	(m(1, 0) + m(0, 1)) / S,
						0.25f * S,
						(m(2, 1) + m(1, 2)) / S);
		
	}
	else
	{
		float S = sqrt(1 + m(2, 2) - m(0, 0) - m(1, 1) ) * 2.0f;
		w = (m(0, 1) - m(1, 0)) / S;
		v = SVector<3>(	(m(0, 2) + m(2, 0)) / S,
						(m(2, 1) + m(1, 2)) / S,
						0.25f * S				);
	}
}



