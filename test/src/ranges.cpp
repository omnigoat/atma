#include <atma/unit_test.hpp>

#include <atma/algorithm/filter.hpp>
#include <atma/algorithm/map.hpp>

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
	auto operator ()(std::string x) const -> int { return 1; }
	auto operator ()(int x) const -> int { return x - 1; }
} const dec;

constexpr struct square_t {
	constexpr auto operator ()(int x) const -> int { return x * x; }
} const square;

constexpr struct mult_t {
	constexpr auto operator ()(int x, int y) const -> int { return x * y; }
} const mult;

#define ATMA_DEFINE_FUNCTOR(name, code) \
	namespace { \
	struct name##_t { \
		code \
	} const name; }


ATMA_DEFINE_FUNCTOR(things, 
	template <typename C>
	auto operator ()(C&& xs) const -> decltype(auto) { 
		return atma::map([](int x){return x + 1;}, xs);
	}
);



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

int times2(int x) { return x * 2; }

template <typename F, typename G, typename A>
auto compos(F f, G g, A a) -> decltype(auto)
{
	return f(g(a));
}

//#error stop




void test()
{
	//auto b = atma::bind_t<inc_t, std::tuple<atma::placeholder_t<0>>>{inc, std::make_tuple(arg1)};
	auto b = atma::bind(inc, arg1);
	auto r = b(4);
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


#endif
struct filter_fnt 
{
	//template <typename F, typename C>
	//constexpr auto operator()(F&& f, C&& c) const -> decltype(auto)
	//{
	//	return atma::filter(std::forward<F>(f), std::forward<C>(c));
	//}

	template <typename F>
	constexpr auto operator ()(F&& f) const -> int
	{
		return 4;
	}
};

struct map_fnt
{
	template <typename F>
	auto operator ()(F&& f) const -> decltype(auto)
	{
		return 4;
	}
};


SCENARIO("ranges can be filtered", "[ranges/filter_t]")
{
	GIVEN("a vector of numbers")
	{
		auto numbers = atma::vector<int>{1, 2, 3, 4};
		auto is_even = [](int i) { return i % 2 == 0; };
		auto plus_10 = [](int i) { return i + 10; };

		//auto rmap = atma::map(plus_10) <<= numbers;
		//auto rmapv = atma::vector<int>{rmap.begin(), rmap.end()};

		auto works = atma::map(plus_10) % atma::filter(is_even);
		auto wrange = works(numbers);
		auto wrangev = atma::vector<int>{wrange.begin(), wrange.end()};

		//atma::detail::valid_bindings_t<std::tuple<decltype(arg1), decltype(arg3)>> lulz;

		//auto bi = atma::bind(&mult_t::operator (), mult, 5, arg2, arg1);
		//auto bi2 = atma::curry(&decltype(bi)::operator (), bi);
		//bi()
		//bi("lulz", 4);
		test();
		auto b1 = atma::curry(&times2);
		//auto b2 = atma::curry(&times2);
		//auto br = b1 % b2;
		//auto rbr = br(4);

		atma::function<int(int)> finc{inc};
		auto fr = finc(4);
		//static_assert(atma::detail::bindings_count_tx<std::tuple<decltype(arg1)>>::value == 1, "oh 1");
		//static_assert(atma::detail::bindings_count_tx<std::tuple<int, decltype(arg1), char, decltype(arg2)>>::value == 2, "oh");

		//using kk = typename atma::detail::resultant_args_t<std::tuple<char, float, int>, std::tuple<decltype(arg2), decltype(arg1), decltype(arg1)>>::type;
		//kk ki;
		//static_assert(atma::function_traits<inc_t>::arity == 2, "bad arity");
		//filter_fnt filter;
		
		auto something = atma::bind(mult, 3, arg1) % (b1 % inc % square % dec);
		//std::string lsdkjf = decltype(something)();
		auto r = b1 % inc % square << 5;

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
			auto filter2 = atma::filter(is_even);
			auto result = filter2(numbers);
			auto resultv = atma::vector<int>{result.begin(), result.end()};

			CHECK(resultv.size() == 2);
			CHECK(resultv[0] == 2);
			CHECK(resultv[1] == 4);
		}
	}
}
