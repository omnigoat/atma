#ifndef ATMA_MATH
#define ATMA_MATH
//=====================================================================
#include <cmath>
#include <vector>
//=====================================================================
//#include <atma_ASettingsManager.h>
//=====================================================================
//#include "atma/math/atma_.h"
//#include "atma/math/settings.h"
//=====================================================================
#include "atma/math/vector.hpp"
#include "atma/math/matrix.hpp"
#include "atma/math/quaternion.hpp"
#include "atma/silk/atma_silk_SColor.h"
//=====================================================================
#include "atma/math/calculateNormals.h" // ??
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
	
	//=====================================================================
	// Compare two floating point variables, allowing for loss of
	// accuracy
	//=====================================================================
	inline bool Compare(const float& lhs, const float& rhs, float delta = 0.05f)
	{
		return (lhs > rhs - delta && lhs < rhs + delta);
	}
	
	//=====================================================================
	// create random floating-point number
	//=====================================================================
	float RandomFloat(float lower_bound, float upper_bound, float precision = 4);
	
	
	// why this function? I don't know
	template <class T>
	inline vector<3, T> calculateInterpolatedNormal(const std::vector<vector<3, T> >& normals)
	{
		vector<3, T> v;
		for (typename std::vector<vector<3, T> >::const_iterator normal = normals.begin(); normal != normals.end(); ++normal)
		{
			v += *normal;
		}
		
		return Normalize(v);
	}
	/*
	//=====================================================================
	// Transforms our local axies into the "remote" or API defined ones
	//  - Technically it just creates the matrix to do so
	//=====================================================================
	inline void generateAxisMappingMatrix()
	{
		vector<3> changed_y = Setting_LocalAxisY;
		vector<3> changed_z = Setting_LocalAxisZ;
		
		matrix<4> result(IDENTITY), x(IDENTITY), y(IDENTITY), z(IDENTITY);
		
		// create a rotation matrix from just the angle between the two x
		// axis and a vector perpendicular to the two
		if (Setting_LocalAxisX != Setting_RemoteAxisX)
		{
			x = matrix<4>( Angle(Setting_LocalAxisX, Setting_RemoteAxisX), 
							CrossProduct(Setting_LocalAxisX, Setting_RemoteAxisX) );
		
			changed_y = (Setting_MatrixMajority == ROW) ? x * Setting_LocalAxisY : Setting_LocalAxisY * x;
			changed_z = (Setting_MatrixMajority == ROW) ? x * Setting_LocalAxisZ : Setting_LocalAxisZ * x;
		}
		
		if (Setting_LocalAxisY != Setting_RemoteAxisY)
		{
			y = matrix<4>(Angle(changed_y, Setting_RemoteAxisY), 
								CrossProduct(changed_y, Setting_RemoteAxisY));
			
			changed_z = (Setting_MatrixMajority == ROW) ? y * changed_z : changed_z * y;
		}
		
		if (Setting_LocalAxisZ != Setting_RemoteAxisZ)
		{
			if (Compare(Angle(changed_z, Setting_RemoteAxisZ), 180.0f * float(RAD))) z(2, 2) = -1;
		}
		
		Setting_AxisMappingMatrix = (Setting_MatrixMajority == ROW) ? z * y * x : x * y * z;
	}
	
	//=====================================================================
	// Create a Perspective Projection Matrix
	//=====================================================================
	template <typename T> 
	inline matrix<4, 4, T> createPerspectiveMatrix(T r, T l, T t, T b, T n, T f)
	{
		// mapping
		//  - different APIs may map their near and far planes to different values, so
		//    we have to accomodate that. reducing the formulas gives only two additional
		//    multiplications to the matrix code below, and it's a hell-a lot better than
		//    creating a whole new matrix and multiplying by that, so we do it this way.
		T map_n = T(Setting_ProjectionMappingNear);
		T map_f = T(Setting_ProjectionMappingFar);
		float fmndt = (map_f - map_n) / 2;
		float fpndt = (map_f + map_n) / 2;

		// finally, create the matrix.
		matrix<4, 4, T> mtx;
		mtx(0) = 2 * n / (r - l);
		mtx(8) = (l + r) / (l - r);
		mtx(5) = 2 * n / (t - b);
		mtx(9) = (t + b) / (b - t);
		mtx(11) = 1;
		mtx(10) = (f + n) / (f - n)    * fmndt + fpndt;
		mtx(14) = 2 * n * f / (n - f)  * fmndt;
		
		// hallejuiah! praise be to Krishna!
		return (Setting_MatrixMajority == ROW) ? Setting_AxisMappingMatrix * mtx : mtx * Setting_AxisMappingMatrix;
	}

	//=====================================================================
	// Create a perspective matrix from a FOV
	//=====================================================================
	template <typename T> 
	inline matrix<4, 4, T> createPerspectiveMatrixFOV(T fov, T aspect, T n, T f)
	{
		T hh = fov * T(0.5) * n;
		T hw = hh * aspect;
		return createPerspectiveMatrix(-hw, hw, hh, -hh, n, f);
	}
	
	//=====================================================================
	// Create a LH View Matrix from
	//=====================================================================
	template <class T>
	matrix<4, 4, T> createViewMatrix(const vector<3, T>& eye, const vector<3, T>& target, const vector<3, T>& up)
	{
		vector<3, T> z_axis = Normalize(target - eye);
		vector<3, T> x_axis = Normalize(CrossProduct(up, z_axis));
		vector<3, T> y_axis = CrossProduct(z_axis, x_axis);

		// now create the matrix
		matrix<4, 4, T> mtx;
		
		mtx(0) = x_axis.x;
		mtx(1) = y_axis.x;
		mtx(2) = z_axis.x;
		
		mtx(4) = x_axis.y;
		mtx(5) = y_axis.y;
		mtx(6) = z_axis.y;
		
		mtx(8) = x_axis.z;
		mtx(9) = y_axis.z;
		mtx(10) = z_axis.z;
		
		mtx(12) = -DotProduct(x_axis, eye);
		mtx(13) = -DotProduct(y_axis, eye);
		mtx(14) = -DotProduct(z_axis, eye);
		mtx(15) = 1;
		
		// return the view matrix, allowing for the local-to-remote axis mapping
		return (Setting_MatrixMajority == ROW) ? mtx * Setting_AxisMappingMatrix : Setting_AxisMappingMatrix * mtx;
	}
	
	
	*/
	
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================


