#pragma once


#include <atma/idxs.hpp>
#include <atma/tuple.hpp>


namespace atma {

	//
	//  placeholder_t
	//  ---------------------------
	//    hooray!
	//
	template <int I> struct placeholder_t
	{
		// placeholder_t is an identity function (i.e., it constructs itself)
		using type = placeholder_t<I>;
		static int const value = I;
	};




	//
	//  tuple_placeholder_list_t
	//  tuple_placeholder_range_t
	//  ---------------------------
	//    tuple_placeholder_list_t<2> === std::tuple<placeholder_t<0>, placeholder_t<1>>
	//    tuple_placeholder_range_t<1, 3> === std::tuple<placeholder_t<1>, placeholder_t<2>>
	//
	template <size_t begin, size_t end, int step = 1>
	using tuple_placeholder_range_t =
		typename tuple_idxs_map_t<placeholder_t, idxs_range_t<begin, end, step>>::type;

	template <size_t count>
	using tuple_placeholder_list_t =
		typename tuple_idxs_map_t<placeholder_t, idxs_list_t<count>>::type;




	//
	//  tuple_nonplaceholder_size_t
	//  -----------------------------
	//    when we have a tuple of {thing, thing2, thing3, placeholder_t<0>, placeholder_t<1>...},
	//    we want to count how many things there are before the first placeholder
	//
	template <typename T, int Acc = 0>
	struct tuple_nonplaceholder_size_t;

	template <int Acc>
	struct tuple_nonplaceholder_size_t<std::tuple<>, Acc>
	{
		static int const value = Acc;
	};

	template <typename x, typename... xs, int Acc>
	struct tuple_nonplaceholder_size_t<std::tuple<x, xs...>, Acc>
	{
		static int const value = tuple_nonplaceholder_size_t<std::tuple<xs...>, Acc + 1>::value;
	};

	template <int i, typename... xs, int Acc>
	struct tuple_nonplaceholder_size_t<std::tuple<placeholder_t<i>, xs...>, Acc>
	{
		static int const value = Acc;
	};

}

namespace
{
	atma::placeholder_t<0> const arg1;
	atma::placeholder_t<1> const arg2;
	atma::placeholder_t<2> const arg3;
	atma::placeholder_t<3> const arg4;
	atma::placeholder_t<4> const arg5;
	atma::placeholder_t<5> const arg6;
	atma::placeholder_t<6> const arg7;
	atma::placeholder_t<7> const arg8;
}

