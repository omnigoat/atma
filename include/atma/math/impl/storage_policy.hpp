//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_impl_UTILITY_HPP
#define ATMA_MATH_impl_UTILITY_HPP
//=====================================================================
#include <type_traits>
//=====================================================================
namespace atma {
namespace math {
namespace impl {
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
		typedef T const& type;
	};

	template <>
	struct storage_policy<float> {
		typedef float const type;
	};

	template <typename R, typename O>
	struct storage_policy<expr<R, O>> {
		typedef expr<R, O> const type;
	};
	
//=====================================================================
} // namespace impl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
