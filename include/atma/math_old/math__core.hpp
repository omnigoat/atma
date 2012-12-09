//=====================================================================
//
//  The Atma Math Core
//  --------------------
//    Like any good 3D graphics enthusiast, I had to write my own maths
//    library instead of using someone else's. Turns out it was very
//    good for learning the difference between G++ and MSVC. Anyway,
//    this file is mandated to be included by all atma::math header
//    files.
//
//=====================================================================
#ifndef ATMA_MATH__CORE_HPP
#define ATMA_MATH__CORE_HPP
//=====================================================================
#include <atma/atma__core.h>

//=====================================================================
//
//  Namespace Defines
//  --------------------
//    These are here for the sole purpose of making our namespace
//    declarations easier for us.
//
//=====================================================================	
#define ATMA_MATH_BEGIN namespace atma { namespace math {
#define ATMA_MATH_CLOSE } } // namespace atma::math

//=====================================================================
//
//  Beginning of math namespace
//  -----------------------------
//    As this is the "top" math file, this file is responsible for
//    laying out all the constants (such as PI), and other similar
//    "global" mathsy-things.
//
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================

	//=====================================================================
	// PI and trig stuff
	//=====================================================================
	//const double PI = 3.1415926535897932384626433832795028841971693993751058209749445923078164;
	//const double RAD = PI / 180.0;
	//const double DEG = 180.0 / PI;
	
	const float PI = 3.141592653589793238462f;
	const float RAD = PI / 180.0f;
	const float DEG = 180.0f / PI;
	
	inline bool Compare(const float& lhs, const float& rhs, float delta = 0.05f)
	{
		return (lhs > rhs - delta && lhs < rhs + delta);
	}
	
	//=====================================================================
	//
	//  Constant maths library
	//  ------------------------
	//    This maths library involves the use of templated structures to
	//    allow for compile-time functions, such as exponential or factorial
	//    functions to be resolved at compile time (for example, template
	//    functions).
	//
	//=====================================================================
	
		//=====================================================================
		// const_pow
		// -----------
		//   Allows for a compile-time "to the power of" function
		//=====================================================================
		template <size_t B, size_t E> struct const_pow
		{
			static const size_t value = const_pow<B, E - 1>::value * B;
		};
		
		template <size_t B> struct const_pow<B, 1>
		{
			static const size_t value = B;
		};
		
		template <size_t B> struct const_pow<B, 0>
		{
			static const size_t value = 1;
		};
	
		//=====================================================================
		// const_factorial
		// -----------------
		//   Allows for a compile-time factorial function
		//=====================================================================
		template <size_t B> struct const_factorial
		{
			static const size_t value = const_factorial<B - 1>::value * B;
		};
		
		template <> struct const_factorial<1>
		{
			static const size_t value = 1;
		};
		
		template <> struct const_factorial<0>
		{
			static const size_t value = 1;
		};
		
	//=====================================================================
	// Whether or not we're a row-major matrix or column-major
	//=====================================================================
	struct matrix_majority
	{
		static const matrix_majority& row()
		{
			static matrix_majority t(1);
			return t;
		}
		
		static const matrix_majority& column()
		{
			static matrix_majority t(0);
			return t;
		}
	
		bool operator == (const matrix_majority& rhs)
		{
			return mm == rhs.mm;
		}

	private:		
		matrix_majority(size_t n)
		 : mm(n)
		{
		}

		size_t mm;
	};
	
	//=====================================================================
	// Type of matrix on creation
	//=====================================================================
	struct matrix_type
	{
		static const matrix_type& zero()
		{
			static matrix_type t(0);
			return t;
		}
		
		static const matrix_type& identity()
		{
			static matrix_type t(1);
			return t;
		}
		
		bool operator == (const matrix_type& rhs) const
		{
			return type == rhs.type;
		}
		
	private:
		matrix_type(size_t n)
		 : type(n)
		{
		}
		
		size_t type;
	};
	
	enum AE_MATRIX_TYPE
	{
		ZERO,
		IDENTITY,
		MIRROR,
		OTHER
	};
	/*
	
	//=====================================================================
	// Rectangle class
	//=====================================================================
	template <typename E = float>
	struct SRectangle
	{
	public:
		ARect(const E& left = 0, const E& top = 0, const E& right = 0, const E& bottom = 0)
		: Left(left), Top(top), Right(right), Bottom(bottom)
		{
		}
		
		E Left, Top, Right, Bottom;
		
		E getHeight() { return Bottom - Top; }
		E getWidth() { return Right - Left; }
	};
*/
	
	enum Color_Representation
	{
		RGB,
		CMKY,
	};
	/*
	struct Color
	{
		unsigned long long color;
		
		Color(unsigned long long color)
		: color(color)
		{
		}
		
		Color(unsigned long color)
		: color ( 
					( unsigned long long((color & 0xff000000) >> 24) * 0x101) << 48 |
					( unsigned long long((color & 0xff0000) >> 16) * 0x101) << 32 |
					( unsigned long long((color & 0xff00) >> 8) * 0x101) << 16 |
					( unsigned long long((color & 0xff) ) * 0x101)
		        )
		{
		}
		
		Color(float r, float g, float b, float a)
		: color (
					unsigned long long(r * 0xffff) << 48 |
					unsigned long long(g * 0xffff) << 32 |
					unsigned long long(b * 0xffff) << 16 |
					unsigned long long(a * 0xffff)
		        )
		{
		}
		
		operator unsigned long long()
		{
			return color;
		}
	};
	*/
	
	struct Color
	{
		float r, g, b, a;
	};
	
	
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
