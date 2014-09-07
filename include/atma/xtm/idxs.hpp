#pragma once

namespace atma { namespace xtm {

	template <uint... idxs>
	struct idxs_t
	{
	};


	namespace detail
	{
		// idxs_range_impl
		template <uint begin, uint end, uint... idxs>
		struct idxs_range_impl
		{
			using type = typename idxs_range_impl<begin + 1, end, begin, idxs...>::type;
		};

		template <uint t, uint... idxs>
		struct idxs_range_impl<t, t, idxs...>
		{
			using type = idxs_t<idxs...>;
		};


		// idxs_map_impl
		template <template <uint> class f, typename idxs>
		struct idxs_map_impl;

		template <template <uint> class f, uint... is>
		struct idxs_map_impl<f, idxs_t<is...>>
		{
			using type = std::tuple<typename f<is>::type...>;
		};
	}


	template <uint begin, uint end>
	using idxs_range_t = typename detail::idxs_range_impl<begin, end>::type;

	template <template <uint> class f, typename idxs>
	using idxs_map_t = typename detail::idxs_map_impl<f, idxs>::type;

}}

