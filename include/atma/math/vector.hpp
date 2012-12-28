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
#include <atma/math/expr_tmpl/expression.hpp>
#include <atma/math/expr_tmpl/elementwise_fns.hpp>
#include <atma/math/expr_tmpl/operators.hpp>
#include <atma/math/expr_tmpl/binary_operator.hpp>
//=====================================================================
namespace atma {
//=====================================================================
	
	template <unsigned int E, typename T>
	struct vector
	{
		typedef T element_type;

		// constructors
		vector();
		vector(std::initializer_list<T>);

		vector(const vector& rhs);
		
		vector& operator = (const vector& rhs);

		const T& operator [] (int i) const;

		const T* begin() const { return &*elements_.begin(); }
		const T* end() const { return &*elements_.end(); }

		vector& operator += (const vector& rhs);
		vector& operator -= (const vector& rhs);
		vector& operator *= (const T& rhs);
		vector& operator /= (const T& rhs);

		template <class OP>
		vector& operator = (const expr_tmpl::expr<vector<E, T>, OP>& e) {
			for (auto i = 0u; i != E; ++i) {
				elements_[i] = e[i];
			}
			return *this;
		}

		vector& set(unsigned int i, const T& n);

	private:
		std::array<T, E> elements_;
	};


	//=====================================================================
	// implementation
	//=====================================================================
	template <unsigned int E, typename T>
	vector<E, T>::vector()
	{
	}

	template <unsigned int E, typename T>
	vector<E, T>::vector(std::initializer_list<T> elements)
	{
		ATMA_ASSERT(elements.size() == E);
		std::copy(elements.begin(), elements.end(), elements_.begin());
	}

	template <unsigned int E, typename T>
	vector<E, T>::vector(const vector<E, T>& rhs)
	 : elements_(rhs.elements_)
	{
	}

	template <unsigned int E, typename T>
	vector<E, T>& vector<E, T>::operator = (const vector<E, T>& rhs) {
		elements_ = rhs.elements_;
		return *this;
	}

	template <unsigned int E, typename T>
	const T& vector<E, T>::operator [](int i) const {
		return elements_[i];
	}

	template <unsigned int E, typename T>
	vector<E, T>& vector<E, T>::operator += (const vector<E, T>& rhs) {
		for (auto i = 0u; i != E; ++i)
			elements_[i] += rhs.elements_[i];
		return *this;
	}

	template <unsigned int E, typename T>
	vector<E, T>& vector<E, T>::operator -= (const vector<E, T>& rhs) {
		for (auto i = 0u; i != E; ++i)
			elements_[i] -= rhs.elements_[i];
		return *this;
	}

	template <unsigned int E, typename T>
	vector<E, T>& vector<E, T>::operator *= (const T& rhs) {
		for (auto i = 0u; i != E; ++i)
			elements_[i] *= rhs;
		return *this;
	}

	template <unsigned int E, typename T>
	vector<E, T>& vector<E, T>::operator /= (const T& rhs) {
		for (auto i = 0u; i != E; ++i)
			elements_[i] /= rhs;
		return *this;
	}

	template <unsigned int E, typename T>
	auto vector<E, T>::set(unsigned int i, const T& n) -> vector<E, T>&
	{
		elements_[i] = n;
		return *this;
	}


	namespace expr_tmpl {
		template <unsigned int E, typename T>
		struct cast<vector<E,T>> {
			template <typename OPER>
			vector<E,T> operator ()(const expr<vector<E,T>, OPER>& o) const {
				vector<E,T> result;
				for (auto i = 0U; i != E; ++i)
					result.set(i, o[i]);
				return result;
			}
		};
	}

	template <unsigned int E, typename T>
	struct vector_mul_post
	{
		auto operator ()(const vector<E,T>& lhs, const T& rhs, int i) const
		 -> decltype(lhs[i]*rhs)
		  { return lhs[i] * rhs; }
	};

	template <unsigned int E, typename T>
	struct vector_mul_pre
	{
		auto operator ()(const T& lhs, const vector<E,T>& rhs, int i) const
		 -> decltype(lhs*rhs[i])
		  { return lhs * rhs[i]; }
	};

	typedef vector<3, float> vector3f;

	template <unsigned int E, typename T>
	inline auto dot_product(const vector<E,T>& lhs, const vector<E,T>& rhs) -> T
	{
		T result{};
		for (auto i = 0u; i != E; ++i)
			result += lhs[i] * rhs[i];
		return result;
	}



	//=====================================================================
	// addition
	//=====================================================================
	// T + T
	template <unsigned int E, typename T>
	inline auto operator + (const vector<E,T>& lhs, const vector<E,T>& rhs)
	 -> expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, vector<E,T>, vector<E,T>>>
	  { return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, vector<E,T>, vector<E,T>>>(lhs, rhs); }

	// T + X
	template <unsigned int E, typename T, typename OPER>
	inline auto operator + (const vector<E,T>& lhs, const expr_tmpl::expr<vector<E,T>, OPER>& rhs)
	 -> expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, vector<E,T>, expr_tmpl::expr<vector<E,T>, OPER>>>
	  { return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, vector<E,T>, expr_tmpl::expr<vector<E,T>, OPER>>>(lhs, rhs); }
	
	// X + T
	template <unsigned int E, typename T, typename OPER>
	inline auto operator + (const expr_tmpl::expr<vector<E,T>, OPER>& lhs, const vector<E,T>& rhs) ->
	expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, expr_tmpl::expr<vector<E,T>, OPER>, vector<E,T>>> {
		return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, expr_tmpl::expr<vector<E,T>, OPER>, vector<E,T>>>(lhs, rhs);
	}

	// X + X
	template <unsigned int E, typename T, typename LHS_OPER, typename RHS_OPER>
	inline auto operator + (const expr_tmpl::expr<vector<E,T>, LHS_OPER>& lhs, const expr_tmpl::expr<vector<E,T>, RHS_OPER>& rhs) ->
	expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, expr_tmpl::expr<vector<E,T>, LHS_OPER>, expr_tmpl::expr<vector<E,T>, RHS_OPER>>> {
		return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, expr_tmpl::expr<vector<E,T>, LHS_OPER>, expr_tmpl::expr<vector<E,T>, RHS_OPER>>>(lhs, rhs);
	}



	//=====================================================================
	// subtraction
	//=====================================================================
	// T - T
	template <unsigned int E, typename T>
	inline auto operator - (const vector<E,T>& lhs, const vector<E,T>& rhs)
	 -> expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_sub, vector<E,T>, vector<E,T>>>
	  { return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_sub, vector<E,T>, vector<E,T>>>(lhs, rhs); }

	// T - X
	template <unsigned int E, typename T, typename OPER>
	inline auto operator - (const vector<E,T>& lhs, const expr_tmpl::expr<vector<E,T>, OPER>& rhs)
	 -> expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_sub, vector<E,T>, expr_tmpl::expr<vector<E,T>, OPER>>>
	  { return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_sub, vector<E,T>, expr_tmpl::expr<vector<E,T>, OPER>>>(lhs, rhs); }
	
	// X - T
	template <unsigned int E, typename T, typename OPER>
	inline auto operator - (const expr_tmpl::expr<vector<E,T>, OPER>& lhs, const vector<E,T>& rhs) ->
	expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_sub, expr_tmpl::expr<vector<E,T>, OPER>, vector<E,T>>> {
		return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_sub, expr_tmpl::expr<vector<E,T>, OPER>, vector<E,T>>>(lhs, rhs);
	}

	// X - X
	template <unsigned int E, typename T, typename LHS_OPER, typename RHS_OPER>
	inline auto operator - (const expr_tmpl::expr<vector<E,T>, LHS_OPER>& lhs, const expr_tmpl::expr<vector<E,T>, RHS_OPER>& rhs) ->
	expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_sub, expr_tmpl::expr<vector<E,T>, LHS_OPER>, expr_tmpl::expr<vector<E,T>, RHS_OPER>>> {
		return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_sub, expr_tmpl::expr<vector<E,T>, LHS_OPER>, expr_tmpl::expr<vector<E,T>, RHS_OPER>>>(lhs, rhs);
	}



	//=====================================================================
	// post multiplication
	//    note: we don't have an expr for the scalar, because any expression
	//          that results in a single element almost definitely has high
	//          computational costs (like dot-products)
	//=====================================================================
	// T * T
	template <unsigned int E, typename T>
	inline auto operator * (const vector<E,T>& lhs, const T& rhs) ->
	expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<vector_mul_post<E,T>, vector<E,T>, T>> {
		return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<vector_mul_post<E,T>, vector<E,T>, T>>(lhs, rhs);
	}

	// X * T
	template <unsigned int E, typename T, typename OPER>
	inline auto operator * (const expr_tmpl::expr<vector<E,T>, OPER>& lhs, const T& rhs) ->
	expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<vector_mul_post<E,T>, expr_tmpl::expr<vector<E,T>, OPER>, T>> {
		expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<vector_mul_post<E,T>, expr_tmpl::expr<vector<E,T>, OPER>, T>>(lhs, rhs);
	}

	//=====================================================================
	// pre multiplication
	//=====================================================================


//=====================================================================
} // namespace atma
//=====================================================================
#endif
//=====================================================================
