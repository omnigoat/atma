//=====================================================================
//
//  matrix<3, 3, T>
//  ---------
//    A specialisation of the matrix class for the 
//=====================================================================
#ifndef ATMA_MATH_MATRIX3_HPP
#define ATMA_MATH_MATRIX3_HPP
#error shouldn't be 
//=====================================================================
#include "atma_math__core.h"
#include "matrix.hpp"
#include "quaternion.hpp"
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
template <size_t R, size_t C, class T> struct matrix;
struct quaternion;
//=====================================================================

	//=====================================================================
	// Our matrix with rows 3, columns 3, and holding type T
	//=====================================================================
	template <class T> struct matrix<3, 3, T>
	{
	public:
		//=====================================================================
		// Constructor: Default
		//=====================================================================
		matrix(AE_MATRIX_TYPE _matrix_type = ZERO)
		{
			// set us all to zero
			for (unsigned long i = 0; i < 9; ++i) m_elements[i] = 0;
			// if we're a square matrix, and we're to be set to an IDENTITY
			// matrix, then do it.
			if (_matrix_type == IDENTITY)
			{
				for (unsigned long i = 0; i < 3; ++i) m_elements[i * 3 + i] = 1;
			}
		}
		
		//=====================================================================
		// Constructor: From Quaternion
		//=====================================================================
/*
		matrix(const quaternion& q)
		{
			float xx	= q.v.x * q.v.x;
			float xy	= q.v.x * q.v.y;
			float xz	= q.v.x * q.v.z;
			float xw	= q.v.x * q.w;
			float yy	= q.v.y * q.v.y;
			float yz	= q.v.y * q.v.z;
			float yw	= q.v.y * q.w;
			float zz	= q.v.z * q.v.z;
			float zw	= q.v.z * q.w;
			
			m_elements[0]		= 1.0f - 2.0f * ( yy + zz );
			m_elements[3]		= 2.0f * ( xy + zw );
			m_elements[6]		= 2.0f * ( xz - yw );
			
			m_elements[1]		= 2.0f * ( xy - zw );
			m_elements[4]		= 1.0f - 2.0f * ( xx + zz );
			m_elements[7]		= 2.0f * ( yz + xw );
			
			m_elements[2]		= 2.0f * ( xz + yw );
			m_elements[5]		= 2.0f * ( yz - xw );
			m_elements[8]		= 1.0f - 2.0f * ( xx + yy );
		}
		*/
		//=====================================================================
		// Accessors
		//=====================================================================
		inline T& operator ()(unsigned long i)
		{
			return m_elements[i];
		}
		
		inline const T& operator ()(unsigned long i) const
		{
			return m_elements[i];
		}
		
		inline T& operator ()(unsigned long r, unsigned long c)
		{
			return m_elements[c * 3 + r];
		}
		
		inline const T& operator ()(unsigned long r, unsigned long c) const
		{
			return m_elements[c * 3 + r];
		}
		
		inline operator T * ()
		{
			return m_elements;
		}
		
		//=====================================================================
		// Division: Scalar
		//=====================================================================
		inline friend matrix<3, 3, T> operator / (const matrix<3, 3, T>& lhs, float rhs)
		{
			matrix<3, 3, T> result;
			for (unsigned long i = 0; i < 9; ++i) result.m_elements[i] = lhs.m_elements[i] / rhs;
			return result;
		}
		
		inline friend matrix<3, 3, T> operator / (float lhs, const matrix<3, 3, T>& rhs)
		{
			matrix<3, 3, T> result;
			for (unsigned long i = 0; i < 9; ++i) result.m_elements[i] = rhs.m_elements[i] / lhs;
			return result;
		}
		
		//=====================================================================
		// Assignment
		//=====================================================================
		inline matrix<3, 3, T>& operator = (const matrix<3, 3, T>& rhs)
		{
			for (unsigned i = 0; i < 9; ++i) m_elements[i] = rhs.m_elements[i];
			return *this;
		}
		
		//=====================================================================
		// Equality
		//=====================================================================
		friend bool operator == (const matrix<3, 3, T>& lhs, const matrix<3, 3, T>& rhs)
		{
			for (unsigned long i = 0; i < 9; ++i) 
			{
				if (lhs.m_elements[i] != rhs.m_elements[i]) return false;
			}
			return true;
		}
		
		//=====================================================================
		// Inequality
		//=====================================================================
		friend bool operator != (const matrix<3, 3, T>& lhs, const matrix<3, 3, T>& rhs)
		{
			for (unsigned long i = 0; i < 9; ++i) 
			{
				if (lhs.m_elements[i] == rhs.m_elements[i]) return false;
			}
			return true;
		}
		
	private:
		// our data
		T m_elements[9];
	};


//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================

