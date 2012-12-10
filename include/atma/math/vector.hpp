//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_VECTOR_HPP
#define ATMA_MATH_VECTOR_HPP
//=====================================================================
#include <type_traits>
#include <atma/math/expr_tmpl/elementwise_opers.hpp>
#include <atma/math/expr_tmpl/expression.hpp>
//=====================================================================
namespace atma {
//=====================================================================
template <unsigned int E, typename T> struct vector;
//=====================================================================
	template <class OP, class EXPR>
	struct expr;

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

		template <class OP, class EXPR>
		vector& operator = (const expr<OP, EXPR>& e) {
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
	vector<3, T>& vector<3, T>::set(const T& x, const T& y, const T& z)
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

	#define ATMA_MATH_OPERATOR_T_T(fn, name, rt, lhst, rhst) \
	expr_tmpl::expr<rt, expr_tmpl::##name##<lhst, rhst>>##fn##(const lhst##& lhs, const rhst##& rhs) { \
		return expr_tmpl::expr<rt, expr_tmpl::##name##<lhst, rhst>>(expr_tmpl::##name##<lhst, rhst>(lhs, rhs)); \
	}
	
	#define ATMA_MATH_OPERATOR_T_X(fn, name, rt, lhst, rhst) \
	template <typename RHS_OPER> \
	expr_tmpl::expr<rt, expr_tmpl::##name##<lhst, expr<rhst, RHS_OPER>>>##fn##(const lhst##& lhs, const expr<rhst, RHS_OPER>& rhs) { \
		return expr_tmpl::expr<rt, expr_tmpl::##name##<lhst, expr<rhst, RHS_OPER>>>(expr_tmpl::##name##<lhst, expr<rhst, RHS_OPER>>(lhs, rhs)); \
	}

	#define ATMA_MATH_OPERATOR_X_T(fn, name, rt, lhst, rhst) \
	template <typename LHS_OPER> \
	expr_tmpl::expr<rt, expr_tmpl::##name##<expr<lhst, LHS_OPER>, rhst>>##fn##(const expr<lhst, LHS_OPER>& lhs, const rhst##& rhs) { \
		return expr_tmpl::expr<rt, expr_tmpl::##name##<expr<lhst, LHS_OPER>, rhst>>(expr_tmpl::##name##<expr<lhst, LHS_OPER>, rhst>(lhs, rhs)); \
	}
	
	#define ATMA_MATH_OPERATOR_X_X(fn, name, rt, lhst, rhst) \
	template <typename LHS_OPER, typename RHS_OPER> \
	expr_tmpl::expr<rt, expr_tmpl::##name##<expr<lhst, LHS_OPER>, expr<rhst, RHS_OPER>>> \
	fn##(const expr<lhst, LHS_OPER>& lhs, const expr<rhst, RHS_OPER>##& rhs) { \
		return expr_tmpl::expr<rt, expr_tmpl::##name##<expr<lhst, LHS_OPER>, expr<rhst, RHS_OPER>>> \
		(expr_tmpl::##name##<expr<lhst, LHS_OPER>, expr<rhst, RHS_OPER>>(lhs, rhs)); \
	}

	#define ATMA_MATH_OPERATOR_T_TX(fn, name, rt, lhst, rhst) \
		ATMA_MATH_OPERATOR_T_T(fn, name, rt, lhst, rhst) \
		ATMA_MATH_OPERATOR_T_X(fn, name, rt, lhst, rhst)

	#define ATMA_MATH_OPERATOR_TX_T(fn, name, rt, lhst, rhst) \
		ATMA_MATH_OPERATOR_T_T(fn, name, rt, lhst, rhst) \
		ATMA_MATH_OPERATOR_X_T(fn, name, rt, lhst, rhst)

	#define ATMA_MATH_OPERATOR_TX_TX(fn, name, rt, lhst, rhst) \
		ATMA_MATH_OPERATOR_T_T(fn, name, rt, lhst, rhst) \
		ATMA_MATH_OPERATOR_T_X(fn, name, rt, lhst, rhst) \
		ATMA_MATH_OPERATOR_X_T(fn, name, rt, lhst, rhst) \
		ATMA_MATH_OPERATOR_X_X(fn, name, rt, lhst, rhst)


	//ATMA_MATH_OPERATOR(elementwise_sub_oper, vector3f, vector3f)
	//ATMA_MATH_OPERATOR_XT_T(elementwise_mul_oper, vector3f, float)
	//ATMA_MATH_OPERATOR_T_XT(elementwise_mul_oper, float, vector3f)

	//#include <atma/math/expr_tmpl/make_operators

	typedef vector<3, float> vector3f;

	/*expr<vector3f, elementwise_add_oper<vector3f, vector3f>> operator + (const vector3f& lhs, const vector3f& rhs) {
		return expr<vector3f, elementwise_add_oper<vector3f, vector3f>>(elementwise_add_oper<vector3f, vector3f>(lhs, rhs));
	}*/
	ATMA_MATH_OPERATOR_TX_TX((operator +), elementwise_add_oper, vector3f, vector3f, vector3f)



	//ATMA_MATH_OPERATOR_T_X((operator +), elementwise_add_oper, vector3f, vector3f, vector3f)
	//ATMA_MATH_OPERATOR_X_T((operator +), elementwise_add_oper, vector3f, vector3f, vector3f)
	//ATMA_MATH_OPERATOR_X_X((operator +), elementwise_add_oper, vector3f, vector3f, vector3f)

/*
	template <typename RHS_OPER>
	expr<vector3f, elementwise_add_oper<vector3f, expr<vector3f, RHS_OPER>>> operator + (const vector3f& lhs, const expr<vector3f, RHS_OPER>& rhs) {
		return expr<vector3f, elementwise_add_oper<vector3f, expr<vector3f, RHS_OPER>>>(lhs, rhs);
	}

	expr<vector3f, elementwise_mul_oper<vector3f, float>> operator * (const vector3f& lhs, const float& rhs) {
		return expr<vector3f, elementwise_mul_oper<vector3f, float>>(elementwise_mul_oper<vector3f, float>(lhs, rhs));
	}

	expr<vector3f, elementwise_mul_oper<float, vector3f>> operator * (const float& lhs, const vector3f& rhs) {
		return expr<vector3f, elementwise_mul_oper<float, vector3f>>(elementwise_mul_oper<float, vector3f>(lhs, rhs));
	}

	template <typename RHS_OPER>
	expr<vector3f, elementwise_mul_oper<float, expr<vector3f, RHS_OPER>>> operator * (const float& lhs, const expr<vector3f, RHS_OPER>& rhs) {
		return expr<vector3f, elementwise_mul_oper<float, expr<vector3f, RHS_OPER>>>
			(elementwise_mul_oper<float, expr<vector3f, RHS_OPER>>(lhs, rhs));
	}*/

	/*
	template <typename T, template <typename, typename> class OP, class EXPR>
	expr<add_oper<vector<3, T>>, binary_expr<vector<3, T>, expr<OP<vector<3, T>>, EXPR>>>
	operator + (const vector<3, T>& lhs, const expr<OP<vector<3, T>>, EXPR>& rhs) {
		return expr<add_oper<vector<3, T>>, binary_expr<vector<3, T>, expr<OP<T>, EXPR>>>(lhs, rhs);
	}

	template <typename T, template <typename> class OP, class EXPR>
	expr<add_oper<T>, binary_expr<vector<3, T>, expr<OP<T>, EXPR>>>
	operator + (const expr<OP<T>, EXPR>& lhs, const vector3f& rhs) {
		return expr<add_oper<vector<3, T>>, binary_expr<vector<3, T>, expr<OP<T>, EXPR>>>(lhs, rhs);
	}

	expr<sub_oper<float>, binary_expr<vector3f, vector3f>> operator - (const vector3f& lhs, const vector3f& rhs) {
		return expr<sub_oper<float>, binary_expr<vector3f, vector3f>>(lhs, rhs);
	}
	expr<vector3f, binary_expr<OP, LHS, RHS>>

	template <typename T, template <typename> class OP, class EXPR>
	expr<sub_oper<T>, binary_expr<vector<3, T>, expr<OP<T>, EXPR>>>
	operator - (const vector<3, T>& lhs, const expr<OP<T>, EXPR>& rhs) {
		return expr<sub_oper<T>, binary_expr<vector<3, T>, expr<OP<T>, EXPR>>>(lhs, rhs);
	}

	

	// subtraction between two binary expressions
	template <
	 template <typename> class L_OP, typename L_LHS, typename L_RHS,
	 template <typename> class R_OP, typename R_LHS, typename R_RHS
	>
	binary_expr<
	 sub_oper<float>,
	 binary_expr<L_OP<float>, L_LHS, L_RHS>,
	 binary_expr<R_OP<float>, R_LHS, R_RHS>
	>
	operator - (const binary_expr<L_OP<float>, L_LHS, L_RHS>& lhs, const binary_expr<R_OP<float>, R_LHS, R_RHS>& rhs) {
		return binary_expr<sub_oper<float>, binary_expr<L_OP<float>, L_LHS, L_RHS>, binary_expr<R_OP<float>, R_LHS, R_RHS>>(lhs, rhs);
	}

	expr<sub_oper<float>, binary_expr<vector3f, vector3f>>
	*/

//=====================================================================
} // namespace atma
//=====================================================================
#endif
//=====================================================================
