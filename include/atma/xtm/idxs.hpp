#pragma once

namespace atma { namespace xtm {

	namespace detail
	{
		template <int, int, int...> struct idxs_range_impl;
		template <template <int> class, typename> struct idxs_map_impl;
	}


	//
	//  idxs_t
	//  --------
	//    a structure that stores a variadic list of ints
	//
	template <int... idxs>
	struct idxs_t
	{
	};




	//
	//  idxs_list_t
	//  idxs_range_t
	//  --------------
	//    creates an idxs_t<> between @begin and @end (or up to @count)
	//
	//      idxs_list_t<4> === idxs_t<0, 1, 2, 3>
	//      idxs_range_t<5, 9> === idxs_t<5, 6, 7, 8>
	//
	template <int count>
	using idxs_list_t = typename detail::idxs_range_impl<0, count>::type;

	template <int begin, int end>
	using idxs_range_t = typename detail::idxs_range_impl<begin, end>::type;

	namespace detail
	{
		// idxs_range_impl
		template <int begin, int end, int... idxs>
		struct idxs_range_impl
		{
			using type = typename idxs_range_impl<begin + 1, end, idxs..., begin>::type;
		};

		template <int t, int... idxs>
		struct idxs_range_impl<t, t, idxs...>
		{
			using type = idxs_t<idxs...>;
		};
	}



#if 0
	//
	//  idxs_to_tuple
	//  -------------------
	//    
	//
	template <template <int> class f, typename idxs>
	using idxs_map_t = typename detail::idxs_map_impl<f, idxs>::type;

	namespace detail
	{
		// idxs_map_impl
		template <template <int> class f, int... is>
		struct idxs_map_impl<f, idxs_t<is...>>
		{
			using type = std::tuple<typename f<is>::type...>;
		};
	}
#endif

}}

