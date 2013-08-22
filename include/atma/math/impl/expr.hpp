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
	
	struct vector4f;

	template <typename T>
	struct base_expr {};

#ifdef ATMA_MATH_USE_SSE
	template <>
	struct base_expr<vector4f>
	{
		__m128 xmmd;
	};
#endif


	template <typename R, typename OPER>
	struct expr
	{
		template <typename... Args>
		expr(const Args&... args)
		 : oper_(args...)
		  {}

#ifndef ATMA_MATH_USE_SSE
		auto operator [](unsigned int i) const
		-> decltype(std::declval<OPER>()[i])
		{
			return oper_[i];
		}
#endif

	private:
		OPER oper_;
	};

//=====================================================================
} // namespace impl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
