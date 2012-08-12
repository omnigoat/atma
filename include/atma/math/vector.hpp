//=====================================================================
//
//  vector
//  ---------
//    This is a templated 3D vector class. It's designed to be
//    specialised if required, but that's not necessary (say, if you
//    have need for a 72 dimension vector). It takes the two template
//    arguments of E and T, where E is the number of elements, and T
//    is the type of those elements.
//
//    As it stands, g++ can't handle having the definitions of the
//    methods in the class itself, so we only declare them in the class
//    and define them later on. Further down the file, we have a series
//    of helpful vector functions. These do not need to be specialised
//    for specialised versions of vector.
//
//=====================================================================
#ifndef ATMA_MATH_VECTOR_HPP
#define ATMA_MATH_VECTOR_HPP
//=====================================================================
#include <cmath>
//=====================================================================
#include "math__core.hpp"
#include "et/expr.hpp"
#include "et/binary_expr.hpp"
#include "vector2.hpp"
#include "vector3.hpp"
#include "vector4.hpp"
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================

	//=====================================================================
	// Vector of E dimensions of type T
	//=====================================================================
	template <size_t E, typename T = float> struct vector
	{
		typedef float length_type;
		
		//=====================================================================
		// Constructor: Default
		//=====================================================================
		vector()
		 : m_elements()
		{
		}
		
		//=====================================================================
		// Access
		//=====================================================================
		inline T& operator ()(size_t i)
		{
			return m_elements[i];
		}
		
		inline const T& operator ()(size_t i) const
		{
			return m_elements[i];
		}
		
		//=====================================================================
		// Self-Addition
		//=====================================================================
		inline vector<E, T>& operator += (const vector<E, T>& rhs)
		{
			for (size_t i = 0; i < E; ++i) (*this)(i) += rhs(i);
			return *this;
		}
		
		//=====================================================================
		// Self-Subration
		//=====================================================================
		inline vector<E, T>& operator -= (const vector<E, T>& rhs)
		{
			for (size_t i = 0; i < E; ++i) (*this)(i) -= rhs(i);
			return *this;
		}
		
		//=====================================================================
		// Self-Multiplication
		//=====================================================================
		template <typename Y> vector<E, T>& operator *= (const Y& rhs)
		{
			for (size_t i = 0; i < E; ++i) (*this)(i) *= rhs;
			return *this;
		}
		
		//=====================================================================
		// Self-Division
		//=====================================================================
		template <typename Y> vector<E, T>& operator /= (const Y& rhs)
		{
			for (size_t i = 0; i < E; ++i) (*this)(i) /= rhs;
			return *this;
		}

	protected:
		// the elements in our vector
		T m_elements[E];
	};
	
	

	//=====================================================================
	//
	//                S T A N D A R D    O P E R A T O R S
	//              ----------------------------------------
	//
	//  These operators are pretty standard for vectors, so we allow them
	//  to be only written once (here), and used for a vector of any size.
	//
	//=====================================================================
	/*
	//========================================================================
	// Addition
	//========================================================================
	template <size_t E, typename T>
	et::expr< et::binary_expr<vector<E, T>, vector<E, T>, et::add_oper> > operator +
		(const vector<E, T>& lhs, const vector<E, T>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::add_oper>(lhs, rhs) );
	}

	template <size_t E, typename T, typename LHS_EXPR>
	et::expr< et::binary_expr<et::expr<LHS_EXPR>, vector<E, T>, et::add_oper> > operator +
		(const et::expr<LHS_EXPR>& lhs, const vector<E, T>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::add_oper>(lhs, rhs) );
	}

	template <size_t E, typename T, typename RHS_EXPR>
	et::expr< et::binary_expr<vector<E, T>, et::expr<RHS_EXPR>, et::add_oper> > operator +
		(const vector<E, T>& lhs, const et::expr<RHS_EXPR>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::add_oper>(lhs, rhs) );
	}

	template <typename LHS_EXPR, typename RHS_EXPR>
	et::expr< et::binary_expr<et::expr<LHS_EXPR>, et::expr<RHS_EXPR>, et::add_oper> > operator + 
		(const et::expr<LHS_EXPR>& lhs, const et::expr<RHS_EXPR>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::add_oper>(lhs, rhs) );
	}


	//========================================================================
	// Subtraction
	//========================================================================
	template <size_t E, typename T>
	et::expr< et::binary_expr<vector<E, T>, vector<E, T>, et::sub_oper> > operator -
		(const vector<E, T>& lhs, const vector<E, T>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::sub_oper>(lhs, rhs) );
	}

	template <size_t E, typename T, typename LHS_EXPR>
	et::expr< et::binary_expr<et::expr<LHS_EXPR>, vector<E, T>, et::sub_oper> > operator -
		(const et::expr<LHS_EXPR>& lhs, const vector<E, T>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::sub_oper>(lhs, rhs) );
	}

	template <size_t E, typename T, typename RHS_EXPR>
	et::expr< et::binary_expr<vector<E, T>, et::expr<RHS_EXPR>, et::sub_oper> > operator -
		(const vector<E, T>& lhs, const et::expr<RHS_EXPR>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::sub_oper>(lhs, rhs) );
	}

	template <typename LHS_EXPR, typename RHS_EXPR>
	et::expr< et::binary_expr<et::expr<LHS_EXPR>, et::expr<RHS_EXPR>, et::sub_oper> > operator - 
		(const et::expr<LHS_EXPR>& lhs, const et::expr<RHS_EXPR>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::sub_oper>(lhs, rhs) );
	}


	//========================================================================
	// Multiplication
	//========================================================================
	template <size_t E, typename T>
	et::expr< et::binary_expr<vector<E, T>, T, et::mult_oper> > operator *
		(const vector<E, T>& lhs, const T& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::mult_oper>(lhs, rhs) );
	}
	
	template <size_t E, typename T>
	et::expr< et::binary_expr<T, vector<E, T>, et::mult_oper> > operator *
		(const T& lhs, const vector<E, T>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::mult_oper>(lhs, rhs) );
	}

	template <typename T, typename LHS_EXPR>
	et::expr< et::binary_expr<et::expr<LHS_EXPR>, T, et::mult_oper> > operator *
		(const et::expr<LHS_EXPR>& lhs, const float& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::mult_oper>(lhs, rhs) );
	}

	template <typename RHS_EXPR>
	et::expr< et::binary_expr<float, et::expr<RHS_EXPR>, et::mult_oper> > operator *
		(const float& lhs, const et::expr<RHS_EXPR>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::mult_oper>(lhs, rhs) );
	}


	//========================================================================
	// Division
	//========================================================================
	template <size_t E, typename T>
	et::expr< et::binary_expr< math::vector<E, T>, T, et::div_oper> > operator /
		(const math::vector<E, T>& lhs, const T& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::div_oper>(lhs, rhs) );
	}

	template <typename LHS_EXPR>
	et::expr< et::binary_expr<et::expr<LHS_EXPR>, float, et::div_oper> > operator /
		(const et::expr<LHS_EXPR>& lhs, const float& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::div_oper>(lhs, rhs) );
	}



	
	*/
	
	
	/*
	//=====================================================================
	// Addition: Vector
	//=====================================================================
	template <size_t E, typename T>
	inline const vector<E, T> operator + (const vector<E, T>& lhs, const vector<E, T>& rhs)
	{
		vector<E, T> v = lhs;
		v += rhs;
		return v;
	}
	
	//=====================================================================
	// Subraction: Vector
	//=====================================================================
	template <size_t E, typename T>
	inline const vector<E, T> operator - (const vector<E, T>& lhs, const vector<E, T>& rhs)
	{
		vector<E, T> v = lhs;
		v -= rhs;
		return v;
	}

	//=====================================================================
	// Multiplication: Scalar
	//=====================================================================
	template <size_t E, typename T, typename Y> 
	const vector<E, T> operator * (const vector<E, T>& lhs, const Y& rhs)
	{
		vector<E, T> v = lhs;
		v *= rhs;
		return v;
	}
	
	template <size_t E, typename T, typename Y>
	const vector<E, T> operator * (const Y& lhs, const vector<E, T>& rhs)
	{
		vector<E, T> v = rhs;
		v *= lhs;
		return v;
	}

	//=====================================================================
	// Division: Scalar
	//=====================================================================
	template <size_t E, typename T, typename Y>
	const vector<E, T> operator / (const vector<E, T>& lhs, const Y& rhs)
	{
		vector<E, T> v = lhs;
		v /= rhs;
		return v;
	}
	
	template <size_t E, typename T, typename Y>
	const vector<E, T> operator / (const Y& lhs, const vector<E, T>& rhs)
	{
		vector<E, T> v = rhs;
		v /= lhs;
		return v;
	}
*/
	//=====================================================================
	// Equality
	//=====================================================================
	template <size_t E, typename T>
	inline bool operator == (const vector<E, T>& lhs, const vector<E, T>& rhs)
	{
		for (size_t i = 0; i < E; ++i)
		{
			if (lhs(i) != rhs(i)) return false;
		}
		return true;
	}
	
	//=====================================================================
	// Inequality
	//=====================================================================
	template <size_t E, typename T>
	inline bool operator != (const vector<E, T>& lhs, const vector<E, T>& rhs)
	{
		for (size_t i = 0; i < E; ++i)
		{
			if (lhs(i) != rhs(i)) return true;
		}
		return false;
	}




	//=====================================================================
	//
	//               H E L P F U L     F U N C T I O N S
	//             ---------------------------------------
	//
	//  These functions here operate on any vector and are the usual
	//  suspects for 3D graphical workings. They rely on the implementation
	//  used for the vectors.
	//
	//=====================================================================

	//=====================================================================
	// Dot-product
	//=====================================================================
	template <size_t E, typename T>
	float DotProduct(const vector<E, T>& v1, const vector<E, T>& v2)
	{
		float result = 0;
		for (size_t i = 0; i < E; ++i) result += float(v1(i) * v2(i));
		return result;
	}
	
	//=====================================================================
	// Normalisation
	//=====================================================================
	template <size_t E, typename T>
	vector<E, T> Normalize(const vector<E, T>& v)
	{
		float m = Magnitude(v);
		if (m == 1) return v;
		return v / m;
	}
	
	//=====================================================================
	// Magnitude
	//=====================================================================
	template <size_t E, typename T>
	float Magnitude(const vector<E, T>& v1)
	{
		float f = 0;
		for (size_t i = 0; i < E; ++i) f += v1(i) * v1(i);
		return std::sqrt(f);
	}
	
	//=====================================================================
	// Returns the angle between two vectors
	//=====================================================================
	template <size_t E, typename T>
	float Angle(const vector<E, T>& lhs, const vector<E, T>& rhs)
	{
		float m = Magnitude(lhs) * Magnitude(rhs);
		if (m == 0.0f) return 0.0f;
		return acos(DotProduct(lhs, rhs) / m);
	}
	
	//=====================================================================
	// Returns the midpoint between two vectors
	//=====================================================================
	template <size_t E, typename T>
	vector<E, T> Midpoint(const vector<E, T>& v1, const vector<E, T>& v2)
	{
		return (v1 + v2) * 0.5f;
	}
	
	
	
	
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
