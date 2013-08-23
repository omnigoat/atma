//=====================================================================
//
//  vector<4, T>
//  ---------------
//    Specialisation for three dimensional vertices. Allows us to give 
//    them the .x, .y, .z and .w properties to make it useful to us.
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
#error vector4.hpp should not be directly included. Include vector.hpp.
//=====================================================================
#else
//=====================================================================
#ifndef ATMA_MATH_VECTOR4_HPP
#define ATMA_MATH_VECTOR4_HPP
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
template <size_t E, typename T> struct vector;
//=====================================================================

	template <typename T> struct vector<4, T>
	{
		//=====================================================================
		// Constructor
		//=====================================================================
		vector(T x = 0, T y = 0, T z = 0, T w = 0)
		 : x(x), y(y), z(z), w(w)
		{
		}
		
		vector(const vector<3, T>& v, T w)
		 : x(v.x), y(v.y), z(v.z), w(w)
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
		inline vector<4, T>& operator += (const vector<4, T>& rhs)
		{
			for (size_t i = 0; i < 4; ++i) (*this)(i) += rhs(i);
			return *this;
		}
		
		//=====================================================================
		// Self-Subration
		//=====================================================================
		inline vector<4, T>& operator -= (const vector<4, T>& rhs)
		{
			for (size_t i = 0; i < 4; ++i) (*this)(i) -= rhs(i);
			return *this;
		}
		
		//=====================================================================
		// Self-Multiplication
		//=====================================================================
		template <typename Y> vector<4, T>& operator *= (const Y& rhs)
		{
			for (size_t i = 0; i < 4; ++i) (*this)(i) *= rhs;
			return *this;
		}
		
		//=====================================================================
		// Self-Division
		//=====================================================================
		template <typename Y> vector<4, T>& operator /= (const Y& rhs)
		{
			for (size_t i = 0; i < 4; ++i) (*this)(i) /= rhs;
			return *this;
		}
		
	private:
		// SNK's trick, with the variable named in his honour
		typedef T vector<4, T>::* const vec[4];
		static const vec mSNK;
			
	public:
		T x, y, z, w;
	};

	template <typename T> float * VectorXYZW(const vector<4, T>& v)
	{
		static float s[4];
		s[0] = v.x; s[1] = v.y; s[2] = v.z; s[3] = v.w;
		return s;
	}
		
	template <typename T> float * VectorXYZW(const vector<3, T>& v, T w = 0)
	{
		static float s[4];
		s[0] = v.x; s[1] = v.y; s[2] = v.z; s[3] = w;
		return s;
	}

//=====================================================================
// static member initialisation
//=====================================================================
template<typename T>
const typename vector<4, T>::vec vector<4, T>::mSNK 
	= { &vector<4, T>::x, &vector<4, T>::y, &vector<4, T>::z, &vector<4, T>::w };
	
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
#endif
//=====================================================================


