//=====================================================================
//
//  vector<2, T>
//  ---------------
//    Specialisation for two dimensional vertices. Allows us to give 
//    them the .x, .y properties to make it useful to us.
//
//    This was rewritten on 20060629 to get rid of the "anonymous union
//    and struct" trick (which is non-standard), and make use of "SNK's
//    trick" (snk_kid from www.gamedev.net fame). It's merely a 
//    standard C++ way of achieving the same thing.
//
//=====================================================================
// We are a specialisation, so we want the user to include vector.hpp,
// not vectorX.hpp - so if they do, we do it the proper way for them.
//=====================================================================
// if vector.hpp wasn't included first
#ifndef ATMA_MATH_VECTOR_HPP
// throw an error
#error vector2.hpp should not be directly included. Include vector.hpp.
//=====================================================================
#else
//=====================================================================
#ifndef ATMA_MATH_VECTOR2_HPP
#define ATMA_MATH_VECTOR2_HPP
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
template <size_t E, typename T> struct vector;
//=====================================================================

	template <typename T> struct vector<2, T>
	{
		//=====================================================================
		// Constructor
		//=====================================================================
		vector(T x = 0, T y = 0) : x(x), y(y)
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
		
		//=====================================================================
		// Self-Addition
		//=====================================================================
		inline vector<2, T>& operator += (const vector<2, T>& rhs)
		{
			for (size_t i = 0; i < 2; ++i) (*this)(i) += rhs(i);
			return *this;
		}
		
		//=====================================================================
		// Self-Subration
		//=====================================================================
		inline vector<2, T>& operator -= (const vector<2, T>& rhs)
		{
			for (size_t i = 0; i < 2; ++i) (*this)(i) -= rhs(i);
			return *this;
		}
		
		//=====================================================================
		// Self-Multiplication
		//=====================================================================
		template <typename Y> vector<2, T>& operator *= (const Y& rhs)
		{
			for (size_t i = 0; i < 2; ++i) (*this)(i) *= rhs;
			return *this;
		}
		
		//=====================================================================
		// Self-Division
		//=====================================================================
		template <typename Y> vector<2, T>& operator /= (const Y& rhs)
		{
			for (size_t i = 0; i < 2; ++i) (*this)(i) /= rhs;
			return *this;
		}
		
	private:
		// SNK's trick, with the variable named in his honour
		typedef T vector<2, T>::* const vec[2];
		static const vec mSNK;
			
	public:
		T x, y;
	};

//=====================================================================
// static member initialisation
//=====================================================================
template<typename T>
const typename vector<2, T>::vec vector<2, T>::mSNK 
	= { &vector<2, T>::x, &vector<2, T>::y };

//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
#endif
//=====================================================================

