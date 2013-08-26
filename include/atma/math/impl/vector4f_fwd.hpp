//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_VECTOR4F_FWD_HPP
#define ATMA_MATH_IMPL_VECTOR4F_FWD_HPP
//=====================================================================
#ifndef ATMA_MATH_VECTOR4F_SCOPE
#	error "this file needs to be included solely from vector4f.hpp"
#endif
//=====================================================================
#include <atma/math/impl/element_type_of.hpp>
//=====================================================================
namespace atma {
namespace math {

	struct vector4f;

namespace impl {
//=====================================================================

	// expression templates
	template <typename, typename> struct vector4f_add;
	template <typename, typename> struct vector4f_sub;
	template <typename, typename> struct vector4f_mul;
	template <typename, typename> struct vector4f_div;

	// element-type of vector4f is a float
	template <>
	struct element_type_of<vector4f>
	{
		typedef float type;
	};

//=====================================================================
} // namespace impl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
