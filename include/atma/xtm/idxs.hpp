#pragma once

namespace atma { namespace xtm {

	namespace detail
	{
		template <int, int, int, int...> struct idxs_range_tx;
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
	//    creates an idxs_t<> of [@begin, @end)
	//
	//      idxs_list_t<4> === idxs_t<0, 1, 2, 3>
	//      idxs_range_t<5, 9> === idxs_t<5, 6, 7, 8>
	//      idxs_range_t<7, 3, -1> === idxs_t<7, 6, 5, 4>
	//      idxs_range_t<7, 3, -3> === such compilation fail
	//
	namespace detail
	{
		template <int begin, int end, int step, int... idxs>
		struct idxs_range_tx
		{
			static_assert(step != 0, "bad arguments to idxs_range_tx");
			static_assert(step > 0 || begin > end, "bad arguments to idxs_range_tx");
			static_assert(step < 0 || begin < end, "bad arguments to idxs_range_tx");

			using type = typename idxs_range_tx<begin + step, end, step, idxs..., begin>::type;
		};

		template <int t, int step, int... idxs>
		struct idxs_range_tx<t, t, step, idxs...>
		{
			using type = idxs_t<idxs...>;
		};
	}

	template <int count>
	using idxs_list_t = typename detail::idxs_range_tx<0, count, 1>::type;

	template <int begin, int end, int step = 1>
	using idxs_range_t = typename detail::idxs_range_tx<begin, end, step>::type;

}}

