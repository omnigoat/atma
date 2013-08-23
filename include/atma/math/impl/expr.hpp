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
#ifdef ATMA_MATH_USE_SSE
		//auto xmmd() const -> decltype(std::declval<OPER>().xmmd()) {
			//return static_cast<OPER const*>(this)->xmmd();
		//}
#else
		auto operator [](unsigned int i) const
		-> decltype(std::declval<OPER>()[i])
		{
			return static_cast<OPER const*>(this)->operator[](i);
		}
#endif
	};

//=====================================================================
} // namespace impl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
