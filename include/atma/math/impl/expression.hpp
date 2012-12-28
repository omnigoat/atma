//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_impl_EXPR_HPP
#define ATMA_MATH_impl_EXPR_HPP
//=====================================================================
namespace atma {
namespace math {
namespace impl {
//=====================================================================

	template <typename R, typename OPER>
	struct expr
	{
		template <typename... Args>
		expr(const Args&... args)
		 : oper(args...)
		  {}
 
		auto operator [](unsigned int i) const
		 -> decltype(std::declval<OPER>()[i])
		  { return oper[i]; }

	private:
		OPER oper;
	};

//=====================================================================
} // namespace impl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
