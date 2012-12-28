//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_EXPR_TMPL_EXPR_HPP
#define ATMA_MATH_EXPR_TMPL_EXPR_HPP
//=====================================================================
namespace atma {
namespace math {
namespace expr_tmpl {
//=====================================================================

	template <typename R, typename OPER>
	struct expr
	{
		template <typename... Args>
		expr(const Args&... args)
		 : oper(args...)
		  {}
 
		auto operator [](int i) const
		 -> decltype(std::declval<OPER>()[i])
		  { return oper[i]; }

		operator R() const
		 { return cast<R>()(*this); }

	private:
		OPER oper;
	};

	template <typename T>
	struct cast {
		template <typename OPER>
		T operator ()(const expr<T, OPER>& o) const {
			return o[0];
		}
	};

//=====================================================================
} // namespace expr_tmpl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
