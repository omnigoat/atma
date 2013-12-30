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

	template <typename C, typename R, typename... Params>
	struct function_traits<R(C::*)(Params...) const>
	{
		typedef R result_type;
		
		enum { arity = sizeof...(Params) };
		
		typedef std::tuple<Params...> tupled_params_type;

		template <size_t i>
		struct arg {
			typedef typename std::tuple_element<i, std::tuple<Params...>>::type type;
		};
	};

	template <typename C, typename R, typename... Params>
	struct function_traits<R(C::*)(Params...)>
	{
		typedef R result_type;

		enum { arity = sizeof...(Params) };

		typedef std::tuple<Params...> tupled_params_type;

		template <size_t i>
		struct arg
		{
			typedef typename std::tuple_element<i, std::tuple<Params...>>::type type;
		};
	};

	template <typename R, typename... Params>
	struct function_traits<R(*)(Params...)>
	{
		typedef R result_type;
		
		enum { arity = sizeof...(Params) };
		
		typedef std::tuple<Params...> tupled_params_type;

		template <size_t i>
		struct arg
		{
			typedef typename std::tuple_element<i, std::tuple<Params...>>::type type;
		};
	};

//=====================================================================
} // namespace xtm
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================



