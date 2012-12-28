//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_VECTOR_HPP
#define ATMA_MATH_VECTOR_HPP
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
	
	//=====================================================================
	// forward-declare expression template functions
	//=====================================================================
	namespace impl
	{
		template <unsigned int E, typename T> struct vector_add;
		template <unsigned int E, typename T> struct vector_sub;
		template <unsigned int E, typename T> struct vector_mul_post;
		template <unsigned int E, typename T> struct vector_mul_pre;
		template <unsigned int E, typename T> struct vector_div;
	}



	//=====================================================================
	// vector!
	//=====================================================================
	template <unsigned int E, typename T>
	struct vector
	{
		typedef T element_type;

		// construction
		vector();
		vector(std::initializer_list<T>);
		vector(const vector& rhs);
		
		// assignment
		vector& operator = (const vector& rhs);
		template <class OP>
		vector& operator = (const impl::expr<vector<E, T>, OP>& e);

		// access
		const T& operator [] (unsigned int i) const;
		const T* begin() const { return &elements_[0]; }
		const T* end() const { return &elements_[0] + E; }

		// computation
		auto magnitude_squared() const -> T;
		auto magnitude() const -> T;
		auto normalized() const -> impl::expr<vector<E, T>, impl::binary_oper<impl::vector_div<E,T>, vector<E,T>, T>>;


		// mutation
		vector& operator += (const vector& rhs);
		vector& operator -= (const vector& rhs);
		vector& operator *= (const T& rhs);
		vector& operator /= (const T& rhs);
		vector& set(unsigned int i, const T& n);
		vector& normalize();

	private:
		std::array<T, E> elements_;
	};
	

	//=====================================================================
	// dot-product
	//=====================================================================
	template <unsigned int E, typename T>
	inline auto dot_product(const vector<E,T>& lhs, const vector<E,T>& rhs) -> T
	{
		T result{};
		for (auto i = 0u; i != E; ++i)
			result += lhs[i] * rhs[i];
		return result;
	}

	//=====================================================================
	// cross-product (3-dimensional only)
	//=====================================================================
	template <typename T>
	inline auto cross_product(const vector<3,T>& lhs, const vector<3,T>& rhs) -> vector<3,T>
	{
		return vector<3,T>{
			lhs[1]*rhs[2] - lhs[2]*rhs[1],
			lhs[2]*rhs[0] - lhs[0]*rhs[2],
			lhs[0]*rhs[1] - lhs[1]*rhs[0]
		};
	}

	

	// implementation
	#include "impl/vector_impl.hpp"
	// operators
	#include "impl/vector_opers.hpp"

//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
