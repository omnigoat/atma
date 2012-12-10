//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_EXPR_TMPL_UTILITY_HPP
#define ATMA_MATH_EXPR_TMPL_UTILITY_HPP
//=====================================================================
#include <type_traits>
//=====================================================================
namespace atma {
namespace expr_tmpl {
//=====================================================================
	
	
	//=====================================================================
	// type deduction of, given a value of T, the
	// return-type of its index operator (operator [])
	//=====================================================================
	template <typename T, bool II = std::is_arithmetic<T>::value>
	struct element_type_of
	 { typedef T type; };

	template <typename T>
	struct element_type_of<T, false>
	 : element_type_of<decltype(&T::operator[]), false>
	  {};
	
	template <typename R, typename C, typename... Args>
	struct element_type_of<R(C::*)(Args...) const, false>
	 { typedef typename std::remove_reference<R>::type type; };


	//=====================================================================
	// correctly retrieve value from either scalar or vector objects
	//=====================================================================
	template <typename T, bool IA = std::is_arithmetic<T>::value>
	struct value {
		static typename element_type_of<T>::type
		 get(const T& in, unsigned int i)
		  { return in[i]; }
	};

	template <typename T>
	struct value<T, true> {
		static T get(const T& in, unsigned int)
		 { return in; }
	};

	

	//=====================================================================
	// member-type for operators, because we must store @expr by value
	//=====================================================================
	template <typename, typename> struct expr;

	template <typename T>
	struct member_type {
		typedef const T& type;
	};

	template <typename R, typename O>
	struct member_type<expr<R, O>> {
		typedef const expr<R, O> type;
	};

	

//=====================================================================
} // namespace expr_tmpl
} // namespace atma
//=====================================================================
#endif
//=====================================================================
