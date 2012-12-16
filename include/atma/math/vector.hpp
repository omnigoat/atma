//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_VECTOR_HPP
#define ATMA_MATH_VECTOR_HPP
//=====================================================================
#include <type_traits>
#include <atma/math/expr_tmpl/expression.hpp>
#include <atma/math/expr_tmpl/elementwise_opers.hpp>
#include <atma/math/expr_tmpl/operators.hpp>
//=====================================================================
namespace atma {
//=====================================================================
template <unsigned int E, typename T> struct vector;
//=====================================================================
	

	template <typename T>
	struct vector<3, T>
	{
		typedef T element_type;

		// constructors
		vector();
		vector(const T& x, const T& y, const T& z);
		vector(const vector& rhs);
		
		vector& operator = (const vector& rhs);

		const T& operator [] (int i) const {
			return *(&x_ + i);
		}

		vector& operator += (const vector& rhs);
		vector& operator -= (const vector& rhs);
		vector& operator *= (const T& rhs);
		vector& operator /= (const T& rhs);

		template <class OP>
		vector& operator = (const expr_tmpl::expr<vector<3, T>, OP>& e) {
			x_ = e[0];
			y_ = e[1];
			z_ = e[2];
			return *this;
		}

		const T& x() const { return x_; }
		const T& y() const { return y_; }
		const T& z() const { return z_; }

		vector& set(const T& x, const T& y, const T& z);
		vector& set_x(const T& x);
		vector& set_y(const T& y);
		vector& set_z(const T& z);

		
	private:
		T x_, y_, z_;
	};


	//=====================================================================
	// implementation
	//=====================================================================
	template <typename T>
	vector<3, T>::vector()
	 : x_(), y_(), z_()
	{
	}

	template <typename T>
	vector<3, T>::vector(const T& x, const T& y, const T& z)
	 : x_(x), y_(y), z_(z)
	{
	}

	template <typename T>
	vector<3, T>::vector(const vector<3, T>& rhs)
	 : x_(rhs.x), y_(rhs.y), z_(rhs.z)
	{
	}

	template <typename T>
	vector<3, T>& vector<3, T>::operator = (const vector<3, T>& rhs) {
		x_ = rhs.x_;
		y_ = rhs.y_;
		z_ = rhs.z_;
		return *this;
	}

	template <typename T>
	vector<3, T>& vector<3, T>::operator += (const vector<3, T>& rhs) {
		x_ += rhs.x_;
		y_ += rhs.y_;
		z_ += rhs.z_;
		return *this;
	}

	template <typename T>
	vector<3, T>& vector<3, T>::operator -= (const vector<3, T>& rhs) {
		x_ -= rhs.x_;
		y_ -= rhs.y_;
		z_ -= rhs.z_;
		return *this;
	}

	template <typename T>
	vector<3, T>& vector<3, T>::operator *= (const T& rhs) {
		x_ *= rhs;
		y_ *= rhs;
		z_ *= rhs;
		return *this;
	}

	template <typename T>
	vector<3, T>& vector<3, T>::operator /= (const T& rhs) {
		x_ /= rhs;
		y_ /= rhs;
		z_ /= rhs;
		return *this;
	}

	template <typename T>
	auto vector<3, T>::set(const T& x, const T& y, const T& z) -> vector<3, T>&
	{
		x_ = x;
		y_ = y;
		z_ = z;
		return *this;
	}

	template <typename T>
	vector<3, T>& vector<3, T>::set_x(const T& x)
	{
		x_ = x;
		return *this;
	}

	template <typename T>
	vector<3, T>& vector<3, T>::set_y(const T& y)
	{
		y_ = y;
		return *this;
	}

	template <typename T>
	vector<3, T>& vector<3, T>::set_z(const T& z)
	{
		z_ = z;
		return *this;
	}


	typedef vector<3, float> vector3f;

	ATMA_MATH_OPERATOR_TX_TX(operator +, elementwise_add_oper, vector3f, vector3f, vector3f)
	ATMA_MATH_OPERATOR_TX_TX(operator -, elementwise_sub_oper, vector3f, vector3f, vector3f)
	ATMA_MATH_OPERATOR_T_TX(operator *, elementwise_mul_oper, float, vector3f, vector3f)
	ATMA_MATH_OPERATOR_TX_T(operator *, elementwise_mul_oper, vector3f, float, vector3f)
	ATMA_MATH_OPERATOR_TX_T(operator /, elementwise_div_oper, vector3f, float, vector3f)
	
	ATMA_MATH_OPERATOR_TX_TX(dot_product, vector_dotproduct_oper, vector3f, vector3f, vector3f)

//=====================================================================
} // namespace atma
//=====================================================================
#endif
//=====================================================================
