//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_MATRIX_HPP
#define ATMA_MATH_MATRIX_HPP
//=====================================================================
#include <array>
#include <numeric>
#include <type_traits>
#include <initializer_list>
#include <atma/assert.hpp>
#include <atma/math/impl/expression.hpp>
#include <atma/math/impl/operators.hpp>
#include <atma/math/impl/binary_operator.hpp>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	
	template <unsigned int M, unsigned int N, typename T>
	struct matrix
	{
		const T& get(int m, int n) const;
		void set(int m, int n, const T&);

	private:
		std::array<T, M*N> elements_;
	};
	

	template <unsigned int M, unsigned int N, typename T>
	const T& matrix::get(int m, int n) const {
		return elements_[m * N + n];
	}

	template <unsigned int M, unsigned int N, typename T>
	void matrix::set(int m, int n, const T& t) {
		elements_[m * N + n] = t;
	}
	
	

//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
