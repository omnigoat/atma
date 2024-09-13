
#define DOCTEST_CONFIG_IMPLEMENT
#include <atma/unit_test.hpp>



//import atma.meta;

#include <atma/functor.hpp>
#include <atma/assert.hpp>
#include <array>
#include <memory>

import atma.memory;

namespace test
{
	struct Counter
	{
		int i = 0;
	};

	static Counter cc;

	auto dragon_count = atma::functor_list_t
	{
		atma::functor_list_fwds_t<Counter>{},

		[](Counter& c, auto&& r) requires requires { { ++c.i }; } { ++c.i; return r.dragon_count(); },
		[](Counter& c, auto&& r) { --c.i; }
	};
}

struct dragon { int dragon_count() { return 4; } };
int blam()
{
	dragon d;
	test::dragon_count(d);
	test::dragon_count(d);
	test::dragon_count(d);
	return test::dragon_count.template forwarded_functor<0>().i;
}

namespace test
{
	template <typename Range>
	concept memory_concept = requires(Range r)
	{
		{ std::data(r) };
	};

	template <typename Range>
	concept bounded_memory_concept = memory_concept<Range> && requires(Range r)
	{
		{ std::size(r) };
	};

	template <typename F>
	inline constexpr auto _memory_range_delegate_ = atma::functor_list_t
	{
		// forward the functor F to our implementations
		atma::functor_list_fwds_t<F>(),
		
		[](auto const& operation, memory_concept auto&& dest, memory_concept auto&& src, size_t sz)
		{
			constexpr bool dest_is_bounded = atma::bounded_memory_concept<decltype(dest)>;
			constexpr bool src_is_bounded = atma::bounded_memory_concept<decltype(src)>;

			ATMA_ASSERT(sz != atma::unbounded_memory_size);

			if constexpr (dest_is_bounded && src_is_bounded)
				ATMA_ASSERT(dest.size() == src.size());

			if constexpr (dest_is_bounded)
				ATMA_ASSERT(dest.size() == sz);

			if constexpr (src_is_bounded)
				ATMA_ASSERT(src.size() == sz);

			operation(
				atma::get_allocator(dest),
				atma::get_allocator(src),
				std::data(dest),
				std::data(src),
				sz);
		},

		[]<bounded_memory_concept Dest, bounded_memory_concept Src>(auto const& operation, Dest&& dest, Src&& src)
		{
			ATMA_ASSERT(std::size(dest) == std::size(src));

			operation(
				atma::get_allocator(dest),
				atma::get_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto const& operation, bounded_memory_concept auto&& dest, memory_concept auto&& src)
		{
			operation(
				atma::get_allocator(dest),
				atma::get_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto const& operation, memory_concept auto&& dest, bounded_memory_concept auto&& src)
		{
			operation(
				atma::get_allocator(dest),
				atma::get_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(src));
		},
	};

	inline constexpr auto _memory_copy_ = atma::functor_list_t
	{
		[] (auto&&, auto&&, auto* px, auto* py, size_t size)
		requires std::is_trivial_v<std::remove_reference_t<decltype(*px)>>
		{
			//static_assert(atma::actually_false_v<decltype(*px)>,
			//	"calling memory-copy (so ultimately ::memcpy) on a non-trivial type");
		},

		[](auto&&, auto&&, auto* px, auto* py, size_t size)
		{
			size_t const size_bytes = size * sizeof(*px);
			::memcpy(px, py, size_bytes);
		}
	};

	inline constexpr auto memory_copy = ::test::_memory_range_delegate_<decltype(_memory_copy_)>;

}

int go()
{
	std::array<int, 4> array{1, 2, 3, 4};
	std::array<int, 4> array2;

	std::vector<int> numbers{1, 2, 3, 4};

	//test::get_allocator(numbers);


	test::memory_copy(
		atma::xfer_dest(array2.data()),
		atma::xfer_src(array.data()),
		4);

	return 4;
}

int main(int argc, char** argv)
{
	return blam();
	//return doctest::Context(argc, argv).run();
	//return go();
}

