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
	
	template <typename, typename> struct expr;


	//=====================================================================
	// storage_policy
	// ----------------
	//   stores @expr<> by value, and all other things by reference. "all
	//   other things" will in general be const references to actual things
	//   we wish to compute (vectors, matrices, numbers, etc). these are
	//   stored by reference as they do not fall off the stack until
	//   computations involving them have finished.
	//=====================================================================
	template <typename T>
	struct storage_policy {
		typedef const T& type;
	};

	template <typename R, typename O>
	struct storage_policy<expr<R, O>> {
		typedef const expr<R, O> type;
	};
	

	//=====================================================================
	// has_index_operator
	// --------------------
	//=====================================================================
	template <typename T, bool P = std::is_arithmetic<T>::value>
	struct has_index_operator {
		static const bool value = true;
	};

	template <typename T>
	struct has_index_operator<T, true> {
		static const bool value = false;
	};




	//=====================================================================
	// value_t
	// ---------
	//   provides the correct storage and index operator for any type.
	//=====================================================================
	template <typename T, bool F = has_index_operator<T>::value>
	struct value_t
	{
		value_t(const T& t)
		 : value_(t)
		  {}

		auto operator [](int i) const
		 -> const decltype(std::declval<T>()[i])&
		  { return value_[i]; }

	private:
		typename storage_policy<T>::type value_;
	};

	template <typename T>
	struct value_t<T, false>
	{
		value_t(const T& t)
		 : value_(t)
		  {}

		auto operator [](int i) const
		 -> T
		  { return value_; }

	private:
		typename storage_policy<T>::type value_;
	};


//=====================================================================
} // namespace expr_tmpl
} // namespace atma
//=====================================================================
#endif
//=====================================================================
