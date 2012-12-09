//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#
//#
//#		Name: matrix
//#
//#		Desc: A 4  x 4 dimensional matrix. Yeah.
//#
//#		Gods: Jonathan "goat" Runting
//#
//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#
#ifndef ATMA_MATH_SMATRIX4_HPP
#define ATMA_MATH_SMATRIX4_HPP
//=====================================================================
#include <algorithm>
#include <vector>
//=====================================================================
#include "atma/math/atma_math__core.h"
#include "atma/math/settings.h"
#include "matrix.hpp"
#include "vector.hpp"
#include "quaternion.hpp"
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
template <unsigned long E, class T> struct vector;
struct quaternion;
//=====================================================================

	//=====================================================================
	// Our matrix with rows 4, columns 4, and holding type T
	//=====================================================================
	template <typename T> struct matrix<4, 4, T>
	{
	public:
		//=====================================================================
		// Constructor: Default
		//=====================================================================
		matrix(AE_MATRIX_TYPE matrix_type = ZERO)
		{
			if (Setting_MatrixMajority == ROW)
				RM = 4, CM = 1;
			else
				RM = 1, CM = 4;

			// set us all to zero
			for (unsigned long i = 0; i < 16; ++i) m_elements[i] = 0;

			// if we're to be an identity matrix, become one
			if (matrix_type == IDENTITY)
			{
				for (unsigned long i = 0; i < 4; ++i) m_elements[i * 4 + i] = 1;
			}
			// else if we're a mirroring matrix, we need to make the value at 2, 2 a
			// negative one (to mirror the z axis for LH to RH formats)
			else if (matrix_type == MIRROR)
			{
				for (unsigned long i = 0; i < 4; ++i) m_elements[i * 4 + i] = (i == T(2)) ? 1 : T(-1);
			}
		}
		
		//=====================================================================
		// Constructor: From Angle and Axis
		//=====================================================================
		matrix(T angle, const vector<3>& the_axis)
		{
			if (Setting_MatrixMajority == ROW)
				RM = 4, CM = 1;
			else
				RM = 1, CM = 4;
				
			matrix<4, 4, T>& m = *this;
			
			vector<3> axis = Normalize(the_axis);
			
			float c = cos(angle);
			float s = sin(angle);
			float t = 1.0f - c;
			
			float xyt = axis.x * axis.y * t;
			float xzt = axis.x * axis.z * t;
			float yzt = axis.y * axis.z * t;
			float zs = axis.z * s;
			float ys = axis.y * s;
			float xs = axis.x * s;
			
			m(0) = c + axis.x * axis.x * t;
			m(1) = xyt + zs;
			m(2) = xzt - ys;
			
			m(4) = xyt - zs;
			m(5) = c + axis.y * axis.y * t;
			m(6) = yzt + xs;
			
			m(8) = xzt + ys;
			m(9) = yzt - xs;
			m(10) = c + axis.z * axis.z * t;
		}
		
		//=====================================================================
		// Constructor: From Quaternion
		//=====================================================================

		matrix(const quaternion& q);
		matrix(const quaternion& q, const vector<3>& v);
		
		//=====================================================================
		// Accessors
		//=====================================================================
		inline T& operator ()(size_t i)
		{
			return m_elements[i];
		}
		
		inline const T& operator ()(size_t i) const
		{
			return m_elements[i];
		}
		
		inline T& operator ()(size_t r, size_t c)
		{
			return m_elements[r * RM + c * CM];
		}
		
		inline const T& operator ()(size_t r, size_t c) const
		{
			return m_elements[r * RM + c * CM];
		}
		
		inline operator const T * const () const
		{
			return m_elements;
		}
		
		//=====================================================================
		// Division: Scalar
		//=====================================================================
		inline friend matrix<4, 4, T> operator / (const matrix<4, 4, T>& lhs, T rhs)
		{
			matrix<4, 4, T> result;
			for (unsigned long i = 0; i < 16; ++i) result.m_elements[i] = lhs.m_elements[i] / rhs;
			return result;
		}
		
		inline friend matrix<4, 4, T> operator / (T lhs, const matrix<4, 4, T>& rhs)
		{
			matrix<4, 4, T> result;
			for (unsigned long i = 0; i < 16; ++i) result.m_elements[i] = rhs.m_elements[i] / lhs;
			return result;
		}
		
		//=====================================================================
		// Assignment
		//=====================================================================
		inline matrix<4, 4, T>& operator = (const matrix<4, 4, T>& rhs)
		{
			for (unsigned i = 0; i < 16; ++i) m_elements[i] = rhs.m_elements[i];
			return *this;
		}
		
		//=====================================================================
		// Equality
		//=====================================================================
		inline friend bool operator == (const matrix<4, 4, T>& lhs, const matrix<4, 4, T>& rhs)
		{
			for (unsigned long i = 0; i < 16; ++i) 
			{
				if (lhs.m_elements[i] != rhs.m_elements[i]) return false;
			}
			return true;
		}
		
		//=====================================================================
		// Inequality
		//=====================================================================
		inline friend bool operator != (const matrix<4, 4, T>& lhs, const matrix<4, 4, T>& rhs)
		{
			for (unsigned long i = 0; i < 16; ++i)
			{
				if (lhs.m_elements[i] != rhs.m_elements[i]) return true;
			}
			return false;
		}
	
	private:
		// row-multiplier and column-multiplier
		size_t RM, CM;
		// our data
		mutable T m_elements[16];
	};

	//=====================================================================
	// Mulitplication: Vector
	//=====================================================================
	template <typename T> vector<3, T> operator * (const matrix<4, 4, T>& lhs, const vector<3, T>& rhs)
	{
		vector<3, T> result;
		// i: row, j: column
		for (unsigned long i = 0; i < 3; ++i)
		{
			for (unsigned long j = 0; j < 3; ++j)
			{
				result(i) += lhs(i, j) * rhs(j);
			}
		}
		return result;
	}
	
	template <typename T> vector<3, T> operator * (const vector<3, T>& lhs, const matrix<4, 4, T>& rhs)
	{
		vector<3, T> result;
		// i: row, j: column
		for (unsigned long j = 0; j < 3; ++j)
		{
			for (unsigned long i = 0; i < 3; ++i)
			{
				result(j) += rhs(i, j) * lhs(j);
			}
		}
		return result;
	}

	
	template <typename T> matrix<4, 4, T> operator * (const matrix<4, 4, T>& lhs, const matrix<4, 4, T>& rhs)
	{
		matrix<4, 4, T> result;
		
		// i: row, j: column
		for (unsigned long i = 0; i < 4; i++)
		{
			for (unsigned long j = 0; j < 4; j++)
			{
				for (unsigned long k = 0; k < 4; k++)
				{
					result(i, j) += lhs(i, k) * rhs(k, j);
				}
			}
		}
		return result;
	}
	/*
	//=====================================================================
	// Constructor: From Quaternion
	//=====================================================================
	template <typename T> matrix<4, 4, T>::matrix(const quaternion& q)
	{

		if (Setting_MatrixMajority == ROW)
			RM = 4, CM = 1;
		else
			RM = 1, CM = 4;
		
		matrix& us = *this;
		
		float xx	= q.v.x * q.v.x;
		float xy	= q.v.x * q.v.y;
		float xz	= q.v.x * q.v.z;
		float xw	= q.v.x * q.w;
		float yy	= q.v.y * q.v.y;
		float yz	= q.v.y * q.v.z;
		float yw	= q.v.y * q.w;
		float zz	= q.v.z * q.v.z;
		float zw	= q.v.z * q.w;
		
		us(0)		= 1.0f - 2.0f * ( yy + zz );
		us(1)		= 2.0f * ( xy + zw );
		us(2)		= 2.0f * ( xz - yw );
		us(3)		= 0;
		
		us(4)		= 2.0f * ( xy - zw );
		us(5)		= 1.0f - 2.0f * ( xx + zz );
		us(6)		= 2.0f * ( yz + xw );
		us(7)		= 0;
		
		us(8)		= 2.0f * ( xz + yw );
		us(9)		= 2.0f * ( yz - xw );
		us(10)		= 1.0f - 2.0f * ( xx + yy );
		us(11)		= 0;
		
		us(12)		= 0;
		us(13)		= 0;
		us(14)		= 0;
		us(15)		= 1.0f;
	}

	//=====================================================================
	// Constructor: From Quaternion and Vector
	//=====================================================================
	template <typename T>
	matrix<4, 4, T>::matrix(const quaternion& q, const vector<3>& v)
	{
		if (Setting_MatrixMajority == ROW)
			RM = 4, CM = 1;
		else
			RM = 1, CM = 4;
		
		matrix<4, 4, T>& us = *this;

		float xx	= q.v.x * q.v.x;
		float xy	= q.v.x * q.v.y;
		float xz	= q.v.x * q.v.z;
		float xw	= q.v.x * q.w;
		float yy	= q.v.y * q.v.y;
		float yz	= q.v.y * q.v.z;
		float yw	= q.v.y * q.w;
		float zz	= q.v.z * q.v.z;
		float zw	= q.v.z * q.w;
		
		us(0)		= 1.0f - 2.0f * ( yy + zz );
		us(1)		= 2.0f * ( xy + zw );
		us(2)		= 2.0f * ( xz - yw );
		us(3)		= 0;
		
		us(4)		= 2.0f * ( xy - zw );
		us(5)		= 1.0f - 2.0f * ( xx + zz );
		us(6)		= 2.0f * ( yz + xw );
		us(7)		= 0;
		
		us(8)		= 2.0f * ( xz + yw );
		us(9)		= 2.0f * ( yz - xw );
		us(10)		= 1.0f - 2.0f * ( xx + yy );
		us(11)		= 0;
		
		us(12)		= v.x;
		us(13)		= v.y;
		us(14)		= v.z;
		us(15)		= 1.0f;
	}
*/

//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================

