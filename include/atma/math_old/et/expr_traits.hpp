//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_ET_EXPR_TRAITS_HPP
#define ATMA_MATH_ET_EXPR_TRAITS_HPP
//=====================================================================
#include <atma/math/math__core.hpp>
#include <atma/math/vector.hpp>
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
namespace et {
//=====================================================================

	//=====================================================================
	// Forward: Reasoning behind the traits get() methods
	//
	// Since we must accept scalar types (vector * scalar), having a unifed
	// operator () or operator [] ceases to work. That means we must switch,
	// based on type, the method of access. For a scalar type, given the
	// operator (int i), we can simply return the same value. This can be
	// done in traits. Yays.
	//=====================================================================
	namespace detail
	{
		template <typename T> struct is_scalar
		{
			typedef bool value_type;
			static const value_type value = false;
		};
		
		template <>
		struct is_scalar<float>
		{
			typedef bool value_type;
			static const value_type value = true;
		};
		
		
		template <typename T, bool is_scalar>
		struct unary_expr_traits_base
		{
			typedef typename T::component_type component_type;
			typedef T type;
			typedef type& reference_type;
			typedef const type& const_reference_type;
		};
		
		template <typename T>
		struct unary_expr_traits_base<T, true>
		{
			typedef T component_type;
			typedef T type;
			typedef type& reference_type;
			typedef const type& const_reference_type;
		};
	}
	
	
	//=====================================================================
	// 
	//=====================================================================
	template <typename T>
	struct unary_expr_traits
	{
	private:
		typedef detail::unary_expr_traits_base<T, detail::is_scalar<T>::value> traits_base;
		
	public:
		typedef typename traits_base::component_type component_type;
		typedef typename traits_base::type type;
		typedef typename traits_base::reference_type reference_type;
		typedef typename traits_base::const_reference_type const_reference_type;
	};
	
	
	// a binary expression of: T1 oper T2 -> R
	template <typename R, typename T1, typename T2>
	struct binary_expr_traits
	{
	private:
		typedef unary_expr_traits<R> result_traits_base;
		typedef unary_expr_traits<T1> lhs_traits_base;
		typedef unary_expr_traits<T2> rhs_traits_base;
		
	public:
		// what type is return by the expression
		typedef typename result_traits_base::type result_type;
		typedef typename result_traits_base::component_type result_component_type;
		
		// information about the LHS type
		typedef typename lhs_traits_base::component_type lhs_component_type;
		typedef typename lhs_traits_base::type lhs_type;
		typedef typename lhs_traits_base::reference_type lhs_reference_type;
		typedef typename lhs_traits_base::const_reference_type lhs_const_reference_type;
		
		static lhs_component_type get_lhs(const T1& t, int i)
		{
			return get_lhs_< detail::is_scalar<T1>::value >(t, i);
		}
		
		// information about the RHS type
		typedef typename rhs_traits_base::component_type rhs_component_type;
		typedef typename rhs_traits_base::type rhs_type;
		typedef typename rhs_traits_base::reference_type rhs_reference_type;
		typedef typename rhs_traits_base::const_reference_type rhs_const_reference_type;
		
		static rhs_component_type get_rhs(const T2& t, int i)
		{
			return get_rhs_< detail::is_scalar<T2>::value >(t, i);
		}
		
	private:
		template <bool is_scalar>
		static lhs_component_type  get_lhs_(const T1& t, int i)
		{
			return t(i);
		}
		
		template <>
		static lhs_component_type get_lhs_<true>(const T1& t, int i)
		{
			return t;
		}
		
		template <bool is_scalar>
		static rhs_component_type get_rhs_(const T2& t, int i)
		{
			return t(i);
		}
		
		template <>
		static rhs_component_type get_rhs_<true>(const T2& t, int i)
		{
			return t;
		}
	};

//=====================================================================
} // namespace et	
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
