#ifndef ATMA_XTM_FUNCTION_HPP
#define ATMA_XTM_FUNCTION_HPP
//=====================================================================
#include <tuple>
//=====================================================================
namespace atma {
namespace xtm {
//=====================================================================
	
	template <typename T>
	struct function_traits
		: public function_traits<decltype(&T::operator())>
	{
	};
	
	template <typename C, typename R, typename... Args>
	struct function_traits<R(C::*)(Args...) const>
	{
		typedef R result_type;
		
		enum { arity = sizeof...(Args) };
		
		template <size_t i>
		struct arg {
			typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
		};
	};

	template <typename R, typename... Args>
	struct function_traits<R(*)(Args...)>
	{
		typedef R result_type;
		
		enum { arity = sizeof...(Args) };
		
		template <size_t i>
		struct arg
		{
			typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
		};
	};

//=====================================================================
} // namespace xtm
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================



