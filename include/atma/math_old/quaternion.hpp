//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#
//#
//#		Name:	quaternion
//#
//#		Desc:	A Quaternion class. Use it wisely.
//#
//#		Gods:	Jonathan "goat" Runting
//#
//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#
#ifndef ATMA_MATH_SQUATERNION
#define ATMA_MATH_SQUATERNION
//=====================================================================
#include <cmath>
//=====================================================================
#include "math__core.hpp"
#include "vector.hpp"
#include "matrix.hpp"
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
template <size_t R, size_t C, typename T> struct matrix;
//=====================================================================

	//=====================================================================
	// quaternion
	//=====================================================================
	struct quaternion
	{
		//=====================================================================
		// Constructor: Default
		//=====================================================================
		quaternion() 
		 : w(1.0f)
		{
		}
		
		//=====================================================================
		// Constructor: Initialisation
		//=====================================================================
		quaternion(float w, float i, float j, float k) 
		 : w(w), v(vector<3>(i, j, k))
		{
		}
		
		quaternion(float w, const vector<3>& v) : w(w), v(v)
		{
		}
		
		//=====================================================================
		// Constructor: Euler Angles
		//=====================================================================
		quaternion(float x_rotation, float y_rotation, float z_rotation);
		
		//=====================================================================
		// Constructor: Rotation Matrix
		//=====================================================================
		quaternion(const matrix<3, 3, float>& rotation_matrix);
		quaternion(const matrix<4, 4, float>& rotation_matrix);
		
		//=====================================================================
		// addition
		//=====================================================================
		inline friend quaternion operator + (const quaternion& lhs, const quaternion& rhs)
		{
			return quaternion(lhs.w + rhs.w, lhs.v + rhs.v);
		}

		//=====================================================================
		// subtraction
		//=====================================================================
		inline friend quaternion operator - (const quaternion& lhs, const quaternion& rhs)
		{
			return quaternion(lhs.w - rhs.w, lhs.v - rhs.v);
		}
		
		//=====================================================================
		// Self-Addition
		//=====================================================================
		inline quaternion operator += (const quaternion& rhs)
		{
			w += rhs.w;	v += rhs.v;
			return *this;
		}
		
		//=====================================================================
		// Self-Subtraction
		//=====================================================================
		inline quaternion operator -= (const quaternion& rhs)
		{
			w -= rhs.w; v -= rhs.v;
			return *this;
		}
		
		//=====================================================================
		// Self-Multiplication
		//=====================================================================
		inline quaternion operator *= (const quaternion& rhs)
		{
			w = w * rhs.w - DotProduct(v, rhs.v);
			v = CrossProduct(v, rhs.v) + (w * rhs.v) + (rhs.w * v);
			return *this;
		}
		
		//=====================================================================
		// Conjugation
		//=====================================================================
		inline friend quaternion Conjugate(const quaternion& q)
		{
			return quaternion(q.w, -q.v.x, -q.v.y, -q.v.z);
		}
		
		//=====================================================================
		// Norm
		//=====================================================================
		inline friend float Norm (const quaternion& q)
		{
			return q.w * q.w + q.v.x * q.v.x + q.v.y * q.v.y + q.v.z * q.v.z;
		}
		
		//=====================================================================
		// Inversion
		//=====================================================================
		inline friend quaternion Inverse(const quaternion& q)
		{
			quaternion q0 = Conjugate(q); float n = Norm(q);
			q0.w /= n; q0.v /= n;
			return q0;
		}
		
		//=====================================================================
		// Normalization
		//=====================================================================
		inline friend quaternion Normalize(const quaternion& q)
		{
			float n = Norm(q);
			if (n == 1) return q;
			n = 1.0f / sqrtf(n);
			return quaternion(q.w * n, q.v * n);
		}
		
		//=====================================================================
		// Converts to Euler angles
		//=====================================================================
		inline friend vector<3> EulerAngles(const quaternion& q)
		{
			float test = q.v.x*q.v.y + q.v.z*q.w;
			
			float heading = 0;
			float attitude = 0;
			float bank = 0;
		
			// singularity at north pole
			if (test > 0.499)
			{
				heading = 2 * atan2(q.v.x, q.w);
				attitude = float(PI / 2);
			}
			// singularity at south pole
			else if (test < -0.499)
			{
				heading = -2 * atan2(q.v.x, q.w);
				attitude = - float(PI / 2);
			}
			// no singularities!
			else
			{
				float xx = q.v.x*q.v.x;
				float yy = q.v.y*q.v.y;
				float zz = q.v.z*q.v.z;
				
				heading  = atan2 (2*q.v.y*q.w - 2*q.v.x*q.v.z, 1 - 2*yy - 2*zz);
				attitude = asin  (2*test);
				bank     = atan2 (2*q.v.x*q.w - 2*q.v.y*q.v.z, 1 - 2*xx - 2*zz);
			}
			
			return vector<3>(heading, attitude, bank);
		}
		
		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		// data
		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		float w;
		vector<3> v;
	};

	//=====================================================================
	// Multiplication by Quaternion
	//=====================================================================
	inline quaternion operator * (const quaternion& lhs, const quaternion& rhs)
	{
		return quaternion( lhs.w * rhs.w - DotProduct(lhs.v, rhs.v), 
					CrossProduct(lhs.v, rhs.v) + (lhs.w * rhs.v) + (rhs.w * lhs.v) );
	}
	
	//=====================================================================
	// Multiplication by Vector
	//=====================================================================
	inline vector<3> operator * (const quaternion& lhs, const vector<3>& rhs)
	{
		quaternion r = lhs * quaternion(1, rhs) * Conjugate(lhs);
		return vector<3>(r.v.x, r.v.y, r.v.z);
	}
	
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
