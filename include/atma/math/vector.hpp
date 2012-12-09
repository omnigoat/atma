//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_VECTOR_HPP
#define ATMA_MATH_VECTOR_HPP
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








	template <typename T, typename R = T, bool is_integral = std::is_integral<T>::value>
	struct value {
		static R get(const T& in, unsigned int i) {
			return in[i];
		}
	};

	template <typename T, typename R>
	struct value<T, R, true> {
		static R get(const T& in, unsigned int) {
			return in;
		}
	};

	#define ATMA_MATH_BINARY_OPER(NAME, OP) \
	template <typename R> \
	struct NAME { \
		typedef R result_type; \
		static R result(const R& lhs, const R& rhs) { \
			return lhs OP rhs; \
		} \
	};

	ATMA_MATH_BINARY_OPER(add_oper, +)
	ATMA_MATH_BINARY_OPER(sub_oper, -)
	ATMA_MATH_BINARY_OPER(mul_oper, *)
	ATMA_MATH_BINARY_OPER(div_oper, /)

	// expr
	template <class OP, class EXPR>
	struct expr;
	
	// binary_expr
	template <typename LHS, typename RHS>
	struct binary_expr;

	// expr again
	template <class OP, typename LHS, typename RHS>
	struct expr<OP, binary_expr<LHS, RHS>>
	{
		expr(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}

		typename OP::result_type operator [](int i) const {
			return OP::result(
				value<LHS, typename OP::result_type>::get(lhs, i),
				value<RHS, typename OP::result_type>::get(rhs, i)
			);
		}
	private:
		const LHS& lhs;
		const RHS& rhs;
	};

	



	typedef vector<3, float> vector3f;

	expr<add_oper<float>, binary_expr<vector3f, vector3f>> operator + (const vector3f& lhs, const vector3f& rhs) {
		return expr<add_oper<float>, binary_expr<vector3f, vector3f>>(lhs, rhs);
	}

	expr<sub_oper<float>, binary_expr<vector3f, vector3f>> operator - (const vector3f& lhs, const vector3f& rhs) {
		return expr<sub_oper<float>, binary_expr<vector3f, vector3f>>(lhs, rhs);
	}

	template <typename T, template <typename> class OP, class EXPR>
	auto operator - (const vector<3, T>& lhs, const expr<OP<T>, EXPR>& rhs)
	-> expr<sub_oper<T>, binary_expr<vector<3, T>, expr<OP<T>, EXPR>>> {
		return expr<sub_oper<T>, binary_expr<vector<3, T>, expr<OP<T>, EXPR>>>(lhs, rhs);
	}

	/*
	binary_expr<sub_oper<float>, vector3f, vector3f> operator - (const vector3f& lhs, const vector3f& rhs) {
		return binary_expr<sub_oper<float>, vector3f, vector3f>(lhs, rhs);
	}

	template <template <typename> class OP, typename R_LHS, typename R_RHS>
	binary_expr<sub_oper<float>, vector3f, binary_expr<OP<float>, R_LHS, R_RHS>>
	operator - (const vector3f& lhs, const binary_expr<OP<float>, R_LHS, R_RHS>& rhs) {
		return binary_expr<sub_oper<float>, vector3f, binary_expr<OP<float>, R_LHS, R_RHS>>(lhs, rhs);
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
