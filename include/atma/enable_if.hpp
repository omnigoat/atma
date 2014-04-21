#pragma once

namespace atma
{
	namespace detail {
		struct enabler_t {};

		template <typename V, typename... Values>
		struct all_t
		{
			static bool const value = V::value && all_t<Values...>::value;
		};

		template <typename V>
		struct all_t<V>
		{
			static bool const value = V::value;
		};
	}

	//
	// with variadics, allows for *way* cleaner code utilising enable_if.
	// works for all methods/functions/constructors, with other template
	// arguments or solo. it takes a variadic number of integral_constants,
	// and-ing their values together.
	//
	// usage (works for constructors/methods/functions):
	//
	//   struct dragon
	//   {
	//       template <typename FN, atma::enable_if<is_valid_callback<FN>>...>
	//       dragon(FN fn)
	//       {
	//           // ....
	//       }
	//   }
	//
	template <typename... Condition>
	using enable_if = typename std::enable_if<detail::all_t<Condition...>::value, detail::enabler_t>::type;

	template <typename... Condition>
	using disable_if = typename std::enable_if<!detail::all_t<Condition...>::value, detail::enabler_t>::type;
}
