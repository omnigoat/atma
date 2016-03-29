#include <atma/unit_test.hpp>
#include <atma/algorithm/filter.hpp>
#include <atma/vector.hpp>

template <typename T>
struct function_object_traits : atma::function_traits<T>
{
};

struct function_object_base_t
{
};

struct function_object_t : function_object_base_t
{
};

template <typename R, typename A>
struct function_object1_t : function_object_base_t
{
	
};

constexpr struct inc_t {
	constexpr auto operator ()(int x) const -> int { return x + 1; }
} const inc;

constexpr struct dec_t {
	constexpr auto operator ()(int x) const -> int { return x - 1; }
} const dec;

constexpr struct square_t {
	constexpr auto operator ()(int x) const -> int { return x * x; }
} const mult;



struct thing
{
	void operator ()(int) {}
	void operator ()(std::string) {}
};

template <bool A, bool... R>
struct all_of_t
{
	constexpr static bool const value = A && all_of_t<R...>::value;
};

template <bool A>
struct all_of_t<A>
{
	constexpr static bool const value = A;
};

template <bool R>
bool const all_of_v = all_of_t<R>::value;

template <int I>
constexpr bool const lulz = true;


int times2(int x) { return x * 2; }

int test()
{
	//static_assert(atma::detail::bindings_count_tx<std::tuple<decltype(arg1)>>::value == 1, "oh 1");
	using k1 = atma::detail::resultant_args_t<
		typename atma::function_traits<decltype(&times2)>::tupled_args_type,
		 std::tuple<decltype(arg1)>>;

	static_assert(std::is_same<k1, std::tuple<int>>::value, "yay");

	auto b = atma::bind(&times2, arg1);
	auto b2 = atma::bind(b, 4);
}

#error stop

template <typename F, typename G,
	typename std::enable_if<
		all_of_t<
			atma::function_traits<F>::arity == 1,
			atma::function_traits<G>::arity == 1,
		std::is_same<typename atma::function_traits<F>::template arg_type<0>, typename atma::function_traits<G>::result_type>::value
		>::value, int
	>::type incorrect_arguments = 0
>
auto operator % (F&& lhs, G&& rhs) -> decltype(auto)
{
	//using ftype = decltype(&std::remove_reference_t<F>::operator ());
	//using gtype = decltype(&std::remove_reference_t<G>::operator ());
	auto ffn = atma::curry(&std::decay_t<F>::operator (), lhs);
	auto gfn = atma::curry(&std::decay_t<G>::operator (), rhs);
	return atma::curry(&atma::detail::composited<decltype(ffn), decltype(gfn), int, int>, ffn, gfn);
}

#if 0
template <typename A, typename B, template <typename, typename> typename R>
struct type2_constructor_fn_t : function_object1_t
{
	template <typename AA>
	auto operator ()(AA&& a) -> decltype(auto)
	{
		return R<A, B>{std::forward<AA>(a), b};
	}

private:
	B b;
};


struct filter_fnt 
{
	template <typename F, typename C>
	constexpr auto operator()(F&& f, C&& c) const -> decltype(auto)
	{
		return atma::filter(std::forward<F>(f), std::forward<C>(c));
	}

	template <typename F>
	constexpr auto operator ()(F&& f) const -> decltype(auto)
	{
		//return atma::filter(f);
		return atma::bind(atma::filter)
	}
};

#endif


auto blam() -> void
{
	//atma::function_object<thing> 
}



SCENARIO("ranges can be filtered", "[ranges/filter_t]")
{
	GIVEN("a vector of numbers")
	{
		auto numbers = atma::vector<int>{1, 2, 3, 4};
		auto is_even = [](int i) { return i % 2 == 0; };

		static_assert(atma::detail::bindings_count_tx<std::tuple<decltype(arg1)>>::value == 1, "oh 1");
		static_assert(atma::detail::bindings_count_tx<std::tuple<int, decltype(arg1), char, decltype(arg2)>>::value == 2, "oh");

		// should be {int, char}
		using k1 = typename atma::detail::original_args_type_t<
			std::tuple<char, float, int>,
			std::tuple<decltype(arg2), float, decltype(arg1)>, 0, 1>::type;

		k1 ki1;

		using kk = typename atma::detail::resultant_args_t<std::tuple<char, float, int>, std::tuple<decltype(arg2), decltype(arg1), decltype(arg1)>>::type;
		kk ki;
		//static_assert(atma::function_traits<inc_t>::arity == 2, "bad arity");
		//filter_fnt filter;
		auto something = dec % (inc % mult);
		auto r = something(4);
		THEN("standard filtering works")
		{
			auto result = atma::filter(is_even, numbers);
			auto resultv = atma::vector<int>{result.begin(), result.end()};

			CHECK(resultv.size() == 2);
			CHECK(resultv[0] == 2);
			CHECK(resultv[1] == 4);
		}

		THEN("lazy binding filtering works")
		{
			auto result = atma::filter(is_even) <<= numbers;
			auto resultv = atma::vector<int>{result.begin(), result.end()};

			CHECK(resultv.size() == 2);
			CHECK(resultv[0] == 2);
			CHECK(resultv[1] == 4);
		}
	}
}
