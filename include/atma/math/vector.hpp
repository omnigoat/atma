//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_VECTOR_HPP
#define ATMA_MATH_VECTOR_HPP
//=====================================================================
#include <type_traits>
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





	
	// this little structure determines, given a value of T, the
	// return-type of its index operator (operator [])
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


	template <typename T>
	struct member_type {
		typedef const T& type;
	};

	template <typename R, typename O>
	struct member_type<expr<R, O>> {
		typedef const expr<R, O> type;
	};

	


	
	template <typename LHS, typename RHS>
	struct elementwise_add_oper
	{
		elementwise_add_oper(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}

		typename element_type_of<LHS>::type
		 operator [](int i) const
		  { return lhs[i] + rhs[i]; }

	private:
		typename member_type<LHS>::type lhs;
		typename member_type<RHS>::type rhs;
	};

	template <typename LHS, typename RHS>
	struct elementwise_sub_oper
	{
		elementwise_sub_oper(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}

		typename element_type_of<LHS>::type
		 operator [](int i) const
		  { return lhs[i] - rhs[i]; }

	private:
		typename member_type<LHS>::type lhs;
		typename member_type<RHS>::type rhs;
	};

	template <typename LHS, typename RHS>
	struct elementwise_mul_oper
	{
		elementwise_mul_oper(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}

		typename element_type_of<LHS>::type
		 operator [](int i) const
		  { return value<LHS>::get(lhs, i) * value<RHS>::get(rhs, i); }

	private:
		typename member_type<LHS>::type lhs;
		typename member_type<RHS>::type rhs;
	};




	template <typename T>
	struct expression_template_traits;

	// expr
	template <class R, class EXPR>
	struct expr;
	
	// binary_expr
	template <typename LHS, typename RHS>
	struct binary_expr;

	// expr again
	template <typename R, typename OPER>
	struct expr
	{
		expr(const OPER& oper)
		 : oper(oper)
		 {
		 }

		typename element_type_of<OPER>::type
		 operator [](int i) const
		  { return oper[i]; }

	private:
		OPER oper;
	};



	typedef vector<3, float> vector3f;

	expr<vector3f, elementwise_add_oper<vector3f, vector3f>> operator + (const vector3f& lhs, const vector3f& rhs) {
		return expr<vector3f, elementwise_add_oper<vector3f, vector3f>>(elementwise_add_oper<vector3f, vector3f>(lhs, rhs));
	}

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
	}

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
