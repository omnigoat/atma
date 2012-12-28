

//=====================================================================
// expression template functions
//=====================================================================
namespace impl
{
	template <unsigned int E, typename T> struct vector_add {};
	template <unsigned int E, typename T> struct vector_sub {};
	template <unsigned int E, typename T> struct vector_mul_post {};
	template <unsigned int E, typename T> struct vector_mul_pre {};
	template <unsigned int E, typename T> struct vector_div {};

	template <unsigned int E, typename T, typename LHS, typename RHS>
	struct expr<vector<E,T>, binary_oper<vector_add<E,T>, LHS, RHS>>
	{
		expr(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}
		
		auto operator [](unsigned int i) const -> T {
			ATMA_ASSERT(i < E);
			return lhs[i] + rhs[i];
		}
	
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	};

	template <unsigned int E, typename T, typename LHS, typename RHS>
	struct expr<vector<E,T>, binary_oper<vector_sub<E,T>, LHS, RHS>>
	{
		expr(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}
		
		auto operator [](unsigned int i) const -> T {
			ATMA_ASSERT(i < E);
			return lhs[i] - rhs[i];
		}
	
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	};


	template <unsigned int E, typename T, typename LHS, typename RHS>
	struct expr<vector<E,T>, binary_oper<vector_mul_post<E,T>, LHS, RHS>>
	{
		expr(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}
		
		auto operator [](unsigned int i) const -> T {
			ATMA_ASSERT(i < E);
			return lhs[i] * rhs;
		}
	
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	};

	template <unsigned int E, typename T, typename LHS, typename RHS>
	struct expr<vector<E,T>, binary_oper<vector_mul_pre<E,T>, LHS, RHS>>
	{
		expr(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}
		
		auto operator [](unsigned int i) const -> T {
			ATMA_ASSERT(i < E);
			return lhs * rhs[i];
		}
	
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	};

	template <unsigned int E, typename T, typename LHS, typename RHS>
	struct expr<vector<E,T>, binary_oper<vector_div<E,T>, LHS, RHS>>
	{
		expr(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}
		
		auto operator [](unsigned int i) const -> T {
			ATMA_ASSERT(i < E);
			return lhs[i] / rhs;
		}
	
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	};

}
	



//=====================================================================
// addition
//=====================================================================
// T + T
template <unsigned int E, typename T>
inline auto operator + (const vector<E,T>& lhs, const vector<E,T>& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_add<E,T>, vector<E,T>, vector<E,T>>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_add<E,T>, vector<E,T>, vector<E,T>>>(lhs, rhs);
}

// T + X
template <unsigned int E, typename T, typename OPER>
inline auto operator + (const vector<E,T>& lhs, const impl::expr<vector<E,T>, OPER>& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_add<E,T>, vector<E,T>, impl::expr<vector<E,T>, OPER>>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_add<E,T>, vector<E,T>, impl::expr<vector<E,T>, OPER>>>(lhs, rhs);
}
	
// X + T
template <unsigned int E, typename T, typename OPER>
inline auto operator + (const impl::expr<vector<E,T>, OPER>& lhs, const vector<E,T>& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_add<E,T>, impl::expr<vector<E,T>, OPER>, vector<E,T>>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_add<E,T>, impl::expr<vector<E,T>, OPER>, vector<E,T>>>(lhs, rhs);
}

// X + X
template <unsigned int E, typename T, typename LHS_OPER, typename RHS_OPER>
inline auto operator + (const impl::expr<vector<E,T>, LHS_OPER>& lhs, const impl::expr<vector<E,T>, RHS_OPER>& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_add<E,T>, impl::expr<vector<E,T>, LHS_OPER>, impl::expr<vector<E,T>, RHS_OPER>>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_add<E,T>, impl::expr<vector<E,T>, LHS_OPER>, impl::expr<vector<E,T>, RHS_OPER>>>(lhs, rhs);
}



//=====================================================================
// subtraction
//=====================================================================
// T - T
template <unsigned int E, typename T>
inline auto operator - (const vector<E,T>& lhs, const vector<E,T>& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_sub<E,T>, vector<E,T>, vector<E,T>>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_sub<E,T>, vector<E,T>, vector<E,T>>>(lhs, rhs);
}

// T - X
template <unsigned int E, typename T, typename OPER>
inline auto operator - (const vector<E,T>& lhs, const impl::expr<vector<E,T>, OPER>& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_sub<E,T>, vector<E,T>, impl::expr<vector<E,T>, OPER>>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_sub<E,T>, vector<E,T>, impl::expr<vector<E,T>, OPER>>>(lhs, rhs);
}
	
// X - T
template <unsigned int E, typename T, typename OPER>
inline auto operator - (const impl::expr<vector<E,T>, OPER>& lhs, const vector<E,T>& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_sub<E,T>, impl::expr<vector<E,T>, OPER>, vector<E,T>>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_sub<E,T>, impl::expr<vector<E,T>, OPER>, vector<E,T>>>(lhs, rhs);
}

// X - X
template <unsigned int E, typename T, typename LHS_OPER, typename RHS_OPER>
inline auto operator - (const impl::expr<vector<E,T>, LHS_OPER>& lhs, const impl::expr<vector<E,T>, RHS_OPER>& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_sub<E,T>, impl::expr<vector<E,T>, LHS_OPER>, impl::expr<vector<E,T>, RHS_OPER>>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_sub<E,T>, impl::expr<vector<E,T>, LHS_OPER>, impl::expr<vector<E,T>, RHS_OPER>>>(lhs, rhs);
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
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_mul_post<E,T>, vector<E,T>, T>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_mul_post<E,T>, vector<E,T>, T>>(lhs, rhs);
}

// X * T
template <unsigned int E, typename T, typename OPER>
inline auto operator * (const impl::expr<vector<E,T>, OPER>& lhs, const T& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_mul_post<E,T>, impl::expr<vector<E,T>, OPER>, T>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_mul_post<E,T>, impl::expr<vector<E,T>, OPER>, T>>(lhs, rhs);
}

//=====================================================================
// pre multiplication
//=====================================================================
// T * T
template <unsigned int E, typename T>
inline auto operator * (const T& lhs, const vector<E,T>& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_mul_pre<E,T>, T, vector<E,T>>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_mul_pre<E,T>, T, vector<E,T>>>(lhs, rhs);
}

// T * X
template <unsigned int E, typename T, typename OPER>
inline auto operator * (const T& lhs, const impl::expr<vector<E,T>, OPER>& rhs) ->
impl::expr<vector<E,T>, impl::binary_oper<impl::vector_mul_pre<E,T>, T, impl::expr<vector<E,T>, OPER>>> {
	return impl::expr<vector<E,T>, impl::binary_oper<impl::vector_mul_pre<E,T>, T, impl::expr<vector<E,T>, OPER>>>(lhs, rhs);
}


