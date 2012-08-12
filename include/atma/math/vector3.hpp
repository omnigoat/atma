//=====================================================================
//
//  vector<3, T>
//  ---------------
//    Specialisation for three dimensional vertices. Allows us to give 
//    them the .x, .y and .z properties to make it useful to us.
//
//    This was rewritten on 20060629 to get rid of the "anonymous union
//    and struct" trick (which is non-standard), and make use of "SNK's
//    trick" (snk_kid from www.gamedev.net fame). It's merely a 
//    standard C++ way of achieving the same thing.
//
//=====================================================================
// if vector.hpp wasn't included first
#ifndef ATMA_MATH_VECTOR_HPP
// throw an error
#error vector3.hpp should not be directly included. Include vector.hpp.
//=====================================================================
#else
//=====================================================================
#ifndef ATMA_MATH_VECTOR3_HPP
#define ATMA_MATH_VECTOR3_HPP

#include <atma/math/et/expr.hpp>
#include <atma/math/et/vector_operator_macros.hpp>

//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
template <size_t E, typename T> struct vector;
//=====================================================================

	template <typename T> struct vector<3, T>
	{
		typedef T component_type;
		typedef vector<3, T> our_type;
		
		//=====================================================================
		// Constructor
		//=====================================================================
		vector()
		 : x(), y(), z()
		{
		}
		
		vector(T x, T y, T z)
		 : x(x), y(y), z(z)
		{
		}
		
		template <typename X>
		vector( const et::expr<vector<3, T>, X>& xr )
		 : x(xr(0)), y(xr(1)), z(xr(2))
		{
		}
		
		
		//=====================================================================
		// Access
		//=====================================================================
		inline T& operator ()(size_t i)
		{
			return this->*mSNK[i];
		}
		
		inline const T& operator ()(size_t i) const
		{
			return this->*mSNK[i];
		}
		
		template <typename X>
		our_type& operator = (const et::expr<our_type, X>& xr)
		{
			x = xr(0);
			y = xr(1);
			z = xr(2);
			return *this;
		}
		
		// addition
		ATMA_MATH_ET_BINARY_TT(our_type, our_type, our_type, +, et::add_oper)
		ATMA_MATH_ET_BINARY_XT(our_type, our_type, our_type, +, et::add_oper)
		ATMA_MATH_ET_BINARY_TX(our_type, our_type, our_type, +, et::add_oper)
		ATMA_MATH_ET_BINARY_XX(our_type, our_type, our_type, +, et::add_oper)
		
		// subtraction
		ATMA_MATH_ET_BINARY_TT(our_type, our_type, our_type, -, et::sub_oper)
		ATMA_MATH_ET_BINARY_XT(our_type, our_type, our_type, -, et::sub_oper)
		ATMA_MATH_ET_BINARY_TX(our_type, our_type, our_type, -, et::sub_oper)
		ATMA_MATH_ET_BINARY_XX(our_type, our_type, our_type, -, et::sub_oper)
		
		// multiplication
		ATMA_MATH_ET_BINARY_TT(our_type, our_type, float, *, et::mul_oper)
		ATMA_MATH_ET_BINARY_XT(our_type, our_type, float, *, et::mul_oper)
		ATMA_MATH_ET_BINARY_TT(our_type, float, our_type, *, et::mul_oper)
		ATMA_MATH_ET_BINARY_TX(our_type, float, our_type, *, et::mul_oper)
		
		// division
		ATMA_MATH_ET_BINARY_TT(our_type, our_type, float, /, et::div_oper)
		ATMA_MATH_ET_BINARY_XT(our_type, our_type, float, /, et::div_oper)
		
		
		
		//=====================================================================
		// Self-Addition
		//=====================================================================
		inline vector<3, T>& operator += (const vector<3, T>& rhs)
		{
			for (size_t i = 0; i < 3; ++i) (*this)(i) += rhs(i);
			return *this;
		}
		
		//=====================================================================
		// Self-Subration
		//=====================================================================
		inline vector<3, T>& operator -= (const vector<3, T>& rhs)
		{
			for (size_t i = 0; i < 3; ++i) (*this)(i) -= rhs(i);
			return *this;
		}
		
		//=====================================================================
		// Self-Multiplication
		//=====================================================================
		template <typename Y> vector<3, T>& operator *= (const Y& rhs)
		{
			for (size_t i = 0; i < 3; ++i) (*this)(i) *= rhs;
			return *this;
		}
		
		//=====================================================================
		// Self-Division
		//=====================================================================
		template <typename Y> vector<3, T>& operator /= (const Y& rhs)
		{
			for (size_t i = 0; i < 3; ++i) (*this)(i) /= rhs;
			return *this;
		}
		
	private:
		// SNK's trick, with the variable named in his honour
		typedef T vector<3, T>::* const vec[3];
		static const vec mSNK;
			
	public:
		T x, y, z;
	};

	//=====================================================================
	// Cross-Product
	//=====================================================================
	template <typename T>
	inline vector<3, T> CrossProduct(const vector<3, T>& v1, const vector<3, T>& v2)
	{
		return vector<3, T> (	v1.y * v2.z - v1.z * v2.y,
								v1.z * v2.x - v1.x * v2.z,
								v1.x * v2.y - v1.y * v2.x );
	}


//=====================================================================
// static member initialisation
//=====================================================================
template<typename T>
const typename vector<3, T>::vec vector<3, T>::mSNK 
	= { &vector<3, T>::x, &vector<3, T>::y, &vector<3, T>::z };
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
#endif
//=====================================================================

