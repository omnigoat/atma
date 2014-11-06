#pragma once
//=====================================================================
#ifndef ATMA_MATH_VECTOR4F_SCOPE
#	error "this file needs to be included solely from vector4f.hpp"
#endif
//=====================================================================
namespace atma { namespace math {

	//=====================================================================
	// addition
	//=====================================================================
	// vector + vector
	inline auto operator + (vector4f const& lhs, vector4f const& rhs)
		-> impl::vector4f_add<vector4f, vector4f>
	{
		return { lhs, rhs };
	}

	// expr + vector
	template <typename OP>
	inline auto operator + (impl::expr<vector4f, OP> const& lhs, vector4f const& rhs)
		-> impl::vector4f_add<impl::expr<vector4f, OP>, vector4f>
	{
		return { lhs, rhs };
	}

	// vector + expr
	template <typename OP>
	inline auto operator + (vector4f const& lhs, impl::expr<vector4f, OP> const& rhs)
		-> impl::vector4f_add<vector4f, impl::expr<vector4f, OP>>
	{
		return { lhs, rhs };
	}

	// expr + expr
	template <typename LOP, typename ROP>
	inline auto operator + (impl::expr<vector4f, LOP> const& lhs, impl::expr<vector4f, ROP> const& rhs)
		-> impl::vector4f_add<impl::expr<vector4f, LOP>, impl::expr<vector4f, ROP>>
	{
		return { lhs, rhs };
	}


	//=====================================================================
	// subtraction
	//=====================================================================
	// vector - vector
	inline auto operator - (vector4f const& lhs, vector4f const& rhs)
		-> impl::vector4f_sub<vector4f, vector4f>
	{
		return { lhs, rhs };
	}

	// expr - vector
	template <typename OP>
	inline auto operator - (impl::expr<vector4f, OP> const& lhs, vector4f const& rhs)
		-> impl::vector4f_sub<impl::expr<vector4f, OP>, vector4f>
	{
		return { lhs, rhs };
	}

	// vector - expr
	template <typename OP>
	inline auto operator - (vector4f const& lhs, impl::expr<vector4f, OP> const& rhs)
		-> impl::vector4f_sub<vector4f, impl::expr<vector4f, OP>>
	{
		return { lhs, rhs };
	}

	// expr - expr
	template <typename LOP, typename ROP>
	inline auto operator - (impl::expr<vector4f, LOP> const& lhs, impl::expr<vector4f, ROP> const& rhs)
		-> impl::vector4f_sub<impl::expr<vector4f, LOP>, impl::expr<vector4f, ROP>>
	{
		return { lhs, rhs };
	}


	//=====================================================================
	// multiplication
	//=====================================================================
	// vector * float
	inline auto operator * (vector4f const& lhs, float rhs)
		-> impl::vector4f_mul<vector4f, float>
	{
		return { lhs, rhs };
	}

	// X * float
	template <typename OPER>
	inline auto operator * (impl::expr<vector4f, OPER> const& lhs, float rhs)
		-> impl::vector4f_mul<impl::expr<vector4f, OPER>, float>
	{
		return { lhs, rhs };
	}

	// float * vector
	inline auto operator * (float lhs, vector4f const& rhs)
		-> impl::vector4f_mul<float, vector4f>
	{
		return { lhs, rhs };
	}

	// float * X
	template <typename OPER>
	inline auto operator * (float lhs, impl::expr<vector4f, OPER> const& rhs)
		-> impl::vector4f_mul<float, impl::expr<vector4f, OPER>>
	{
		return { lhs, rhs };
	}

	
	//=====================================================================
	// division
	//=====================================================================
	// vector / float
	inline auto operator / (vector4f const& lhs, float rhs)
		-> impl::vector4f_div<vector4f, float>
	{
		return { lhs, rhs };
	}

	// X / float
	template <typename OPER>
	inline auto operator / (impl::expr<vector4f, OPER> const& lhs, float rhs)
		-> impl::vector4f_div<impl::expr<vector4f, OPER>, float>
	{
		return { lhs, rhs };
	}


} }
