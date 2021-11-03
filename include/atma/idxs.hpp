#pragma once

#include <utility>


//
//  idxs_t
//  --------
//
namespace atma
{
	template <size_t... Idxs>
	using idxs_t = std::index_sequence<Idxs...>;
}


//
//  idxs_list_t
//  idxs_range_t
//  --------------
//    creates an idxs_t<> of [begin, end), interpolated by step
//
//      idxs_list_t<4> === idxs_t<0, 1, 2, 3>
//      idxs_range_t<5, 9> === idxs_t<5, 6, 7, 8>
//      idxs_range_t<7, 3, -1> === idxs_t<7, 6, 5, 4>
//      idxs_range_t<7, 3, -2> === idxs_t<7, 5>
//      idxs_range_t<7, 3, -3> === such compilation fail
//
namespace atma::detail
{
	template <size_t Begin, size_t End, int Step, size_t... Idxs>
	struct idxs_range_t_
	{
		static_assert(Step != 0, "bad arguments to idxs_range_tx");
		static_assert(Step > 0 || Begin > End, "bad arguments to idxs_range_tx");
		static_assert(Step < 0 || Begin < End, "bad arguments to idxs_range_tx");

		using type = typename idxs_range_t_<size_t(Begin + Step), End, Step, Idxs..., Begin>::type;
	};
	
	template <size_t Terminal, int Step, size_t... Idxs>
	struct idxs_range_t_<Terminal, Terminal, Step, Idxs...>
	{
		using type = idxs_t<Idxs...>;
	};
}

namespace atma
{
	// forward to std::make_index_sequence for compilation performance
	template <size_t Count>
	using idxs_list_t = std::make_index_sequence<Count>;

	template <size_t Begin, size_t End, int Step = 1>
	using idxs_range_t = typename detail::idxs_range_t_<Begin, End, Step>::type;
}
