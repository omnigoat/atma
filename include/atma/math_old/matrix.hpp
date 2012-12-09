//=====================================================================
//
//  matrix
//  ---------
//    A templated 3D matrix class. It's not designed to be specialised,
//    as there's no "other way" you'd want to address the data, except
//    by row/colum pairs. Of course, you can specialise if you want for
//    some reason.
//
//    Down below, there's the specialisation 
//
//=====================================================================
#ifndef ATMA_MATH_MATRIX_HPP
#define ATMA_MATH_MATRIX_HPP
//=====================================================================
#include "atma/math/math__core.hpp"
#include "atma/math/vector.hpp"
#include "atma/math/quaternion.hpp"
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
struct quaternion;
//=====================================================================

	//=====================================================================
	// Our matrix with rows R, columns C, and holding type T
	//=====================================================================
	template <size_t R, size_t C = R, typename T = float> 
	struct matrix
	{
		//=====================================================================
		// Constructor: Default
		//=====================================================================
		matrix(matrix_type type = matrix_type::zero(), matrix_majority = matrix_majority::row());
		matrix(T angle, const vector<3, T>& axis);
		matrix(const quaternion& q);
		matrix(const quaternion& rotation, const vector<3>& translation);
		
		//=====================================================================
		// Accessors
		//=====================================================================
		inline T& operator ()(unsigned long i)
		{
			// returns 1 for row-major, 0 for column major
			return m_matrix_majority == matrix_majority::row() ? m_elements[i] : m_elements[ i%R * R + i/C ];
		}
		
		inline const T& operator ()(unsigned long i) const
		{
			return m_matrix_majority == matrix_majority::row() ? m_elements[i] : m_elements[ i%R * R + i/C ];
		}
		
		inline T& operator ()(unsigned long r, unsigned long c)
		{
			return m_elements[r * R + c];
		}

		inline const T& operator ()(unsigned long r, unsigned long c) const
		{
			return m_elements[r * R + c];
		}
		
		inline operator const T * const () const
		{
			return m_elements;
		}
		
		//=====================================================================
		//=====================================================================
		inline void set_row_major()
		{
			m_matrix_majority = matrix_majority::row();
		}
		
		inline void set_column_major()
		{
			m_matrix_majority = matrix_majority::column();
		}
		
		//=====================================================================
		// Division: Scalar
		//=====================================================================
		inline friend matrix<R, C, T> operator / (const matrix<R, C, T>& lhs, float rhs)
		{
			matrix<R, C, T> result;
			for (unsigned long i = 0; i < RC; ++i) result.m_elements[i] = lhs.m_elements[i] / rhs;
			return result;
		}
		
		inline friend matrix<R, C, T> operator / (float lhs, const matrix<R, C, T>& rhs)
		{
			matrix<R, C, T> result;
			for (unsigned long i = 0; i < RC; ++i) result.m_elements[i] = rhs.m_elements[i] / lhs;
			return result;
		}
		
		//=====================================================================
		// Assignment
		//=====================================================================
		inline matrix<R, C, T>& operator = (const matrix<R, C, T>& rhs)
		{
			for (unsigned i = 0; i < RC; ++i) m_elements[i] = rhs.m_elements[i];
			return *this;
		}
		
		//=====================================================================
		// Equality
		//=====================================================================
		friend bool operator == (const matrix<R, C, T>& lhs, const matrix<R, C, T>& rhs)
		{
			for (unsigned long i = 0; i < RC; ++i) 
			{
				if (lhs.m_elements[i] != rhs.m_elements[i]) return false;
			}
			return true;
		}
		
		//=====================================================================
		// Inequality
		//=====================================================================
		friend bool operator != (const matrix<R, C, T>& lhs, const matrix<R, C, T>& rhs)
		{
			for (unsigned long i = 0; i < RC; ++i) 
			{
				if (lhs.m_elements[i] == rhs.m_elements[i]) return false;
			}
			return true;
		}
		
	private:
		//
		size_t RM, CM;
		matrix_majority m_matrix_majority;
		// total number of elements
		static const size_t RC = R * C;
		// our data
		mutable T m_elements[R * C];
	};

	//=====================================================================
	// Constructor: Default
	//=====================================================================
	template <size_t R, size_t C, typename T> 
	matrix<R, C, T>::matrix(matrix_type mt, matrix_majority mm)
	 : m_elements(), RM(R), CM(1), m_matrix_majority(mm)
	{
		// if we're to be an identity matrix, become one
		if ( mt == matrix_type::identity() )
		{
			for (unsigned long i = 0; i < std::min(R, C); ++i) (*this)(i, i) = 1;
		}
	}
	
	//=====================================================================
	// Constructor: From angle and axis
	//=====================================================================
	template <size_t R, size_t C, typename T>
	matrix<R, C, T>::matrix(T angle, const vector<3, T>& the_axis)
	 : m_elements(), RM(R), CM(1)
	{
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
	template <size_t R, size_t C, typename T> 
	matrix<R, C, T>::matrix(const quaternion& q)
	 : m_elements(), RM(R), CM(1)
	{
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

	template <size_t R, size_t C, typename T> 
	matrix<R, C, T>::matrix(const quaternion& q, const vector<3>& v)
	 : m_elements(), RM(R), CM(1)
	{
	
		matrix<R, C, T>& us = *this;

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
	

	
	//=====================================================================
	// Multiplication: Scalar
	//=====================================================================
	template <size_t R, size_t C, typename T> matrix<R, C, T> operator * (const matrix<R, C, T>& lhs, float rhs)
	{
		matrix<R, C, T> result;
		for (unsigned long i = 0; i < R*C; ++i) result(i) = lhs(i) * rhs;
		return result;
	}
	
	template <size_t R, size_t C, typename T> matrix<R, C, T> operator * (float lhs, const matrix<R, C, T>& rhs)
	{
		matrix<R, C, T> result;
		for (unsigned long i = 0; i < R*C; ++i) result(i) = result(i) * lhs;
		return result;
	}
	
	//=====================================================================
	// Mulitplication: Vector
	//=====================================================================
	template <size_t R, size_t C, typename T> vector<R, T> operator * (const matrix<R, C, T>& lhs, const vector<C, T>& rhs)
	{
		vector<R, T> result;
		// i: row, j: column
		for (unsigned long i = 0; i < R; ++i)
		{
			for (unsigned long j = 0; j < C; ++j)
			{
				result(i) += lhs(i, j) * rhs(j);
			}
		}
		return result;
	}
	
	template <size_t R, size_t C, typename T> vector<C, T> operator * (const vector<R, T>& lhs, const matrix<R, C, T>& rhs)
	{
		vector<C, T> result;
		// i: row, j: column
		for (unsigned long j = 0; j < R; ++j)
		{
			for (unsigned long i = 0; i < C; ++i)
			{
				result(j) += rhs(i, j) * lhs(j);
			}
		}
		return result;
	}

	//=====================================================================
	// Mulitplication: Vector - Specialisation for matrix<4> and vector<3>
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
	
	//=====================================================================
	// Mulitplication: Matrix
	//=====================================================================
	template <size_t R, size_t C, typename T> matrix<R, C, T> operator * (const matrix<R, C, T>& lhs, const matrix<R, C, T>& rhs)
	{
		matrix<R, C, T> result;
		
		// i: row, j: column
		for (unsigned long i = 0; i < R; i++)
		{
			for (unsigned long j = 0; j < C; j++)
			{
				for (unsigned long k = 0; k < C; k++)
				{
					result(i, j) += lhs(i, k) * rhs(k, j);
				}
			}
		}
		return result;
	}

	//=====================================================================
	// Multiplication: Vector with RM/CM
	//=====================================================================
	
	/*
	template <typename T>
	vector<3, T> multiply(const vector<3, T>& lhs, const matrix<4, 4, T>& rhs, AE_MATRIX_MAJORITY mm = ROW)
	{
		return mm == ROW ? rhs * lhs : lhs * rhs;
	}
	*/
	template <size_t E, typename T>
	matrix<E, E, T> multiply(const matrix<E, E, T>& base, const matrix<E, E, T>& modifier, matrix_majority mm = matrix_majority::row )
	{
		return mm == matrix_majority::row() ? modifier * base : base * modifier;
	}
	
	
	/*
	
	PROBABLY NOT NEEDED
	
	//=====================================================================
	// Converts from our matrix to another of type M
	//=====================================================================
	template <class M> struct matrixConverter
	{
		template <unsigned long R, unsigned long C, class T>
		bool convertTo(M& matrix_to, const matrix<R, C, T>& matrix_from)
		{
			return false;
		}
		
		template <unsigned long R, unsigned long C, class T>
		bool convertFrom(matrix<R, C, T>& matrix_to, const M& matrix_from)
		{
			return false;
		}
	};*/
	
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================

