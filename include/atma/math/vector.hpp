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
	auto dot_product(const vector<E,T>& lhs, const vector<E,T>& rhs) -> T
	{
		T result{};
		for (auto i = 0u; i != E; ++i)
			result += lhs[i] * rhs[i];
		return result;
	}


	template <unsigned int E, typename T>
	auto operator + (const vector<E,T>& lhs, const vector<E,T>& rhs)
	 -> expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, vector<E,T>, vector<E,T>>>
	  { return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, vector<E,T>, vector<E,T>>>(lhs, rhs); }

	template <unsigned int E, typename T, typename OPER>
	auto operator + (const vector<E,T>& lhs, const expr_tmpl::expr<vector<E,T>, OPER>& rhs)
	 -> expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, vector<E,T>, decltype(rhs)>>
	  { return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<expr_tmpl::elementwise_add, vector<E,T>, decltype(rhs)>>(lhs, rhs); }
	

	template <unsigned int E, typename T, typename OPER>
	auto operator * (const vector<E,T>& lhs, const expr_tmpl::expr<T,OPER>& rhs) ->
	expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<vector_mul_post<E,T>, vector<E,T>, decltype(rhs)>>
	{
		return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<vector_mul_post<E,T>, vector<E,T>, decltype(rhs)>>(lhs, rhs);
	}

	template <unsigned int E, typename T>
	auto operator * (const vector<E,T>& lhs, const T& rhs) ->
	expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<vector_mul_post<E,T>, vector<E,T>, T>>
	{
		return expr_tmpl::expr<vector<E,T>, expr_tmpl::binary_oper<vector_mul_post<E,T>, vector<E,T>, T>>(lhs, rhs);
	}


//=====================================================================
} // namespace atma
//=====================================================================
#endif
//=====================================================================
