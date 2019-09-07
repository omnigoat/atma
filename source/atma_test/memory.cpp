#include <atma/unit_test.hpp>
#include <atma/memory.hpp>
//#include <atma/vector.hpp>

#include <numeric>

#define CHECK_MEMORY_II_m(r, v, i, elem) \
	CHECK(((decltype(v)::pointer)v)[i] == elem);

#define CHECK_MEMORY_II(v, seq) \
	BOOST_PP_SEQ_FOR_EACH_I(CHECK_MEMORY_II_m, v, seq)

#define CHECK_MEMORY(v, ...) \
	CHECK_MEMORY_II(v, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))


SCENARIO_OF("memory/base_memory_t", "base_memory_t EBO")
{
	GIVEN("an empty allocator")
	{
		using empty_allocator = atma::aligned_allocator_t<int>;
		static_assert(std::is_empty_v<empty_allocator>, "empty_allocator must actually be empty");

		THEN("sizeof memory_t is 1")
		{
			atma::detail::base_memory_t<byte, empty_allocator> memory;
			CHECK(sizeof(memory) == 1);
		}
	}
}

SCENARIO_OF("memory/simple_memory_t", "simple_memory_t construction")
{
	GIVEN("an empty allocator")
	{
		using empty_allocator = atma::aligned_allocator_t<int>;
		using storage_t = std::vector<int>;
		using int_memory_t = atma::simple_memory_t<int, empty_allocator>;

		THEN("simple_memory_t can be default-constructed")
		{
			int_memory_t memory;

			CHECK(sizeof(memory) == sizeof(int*));
			CHECK((int*)memory == nullptr);
		}

		THEN("simple_memory_t can constructed from a pointer & allocator")
		{
			storage_t store{1, 2, 3, 4};
			auto memory = int_memory_t{store.data(), empty_allocator()};

			CHECK(sizeof(memory) == sizeof(int*));
			CHECK((int*)memory == store.data());
			CHECK_MEMORY(memory, 1, 2, 3, 4);
		}

		THEN("simple_memory_t has working deduction guides")
		{
			int* data = nullptr;
			auto m1 = atma::simple_memory_t{data};
			auto m2 = atma::simple_memory_t{data, empty_allocator()};
		}

		THEN("simple_memory_t can be assigned")
		{
			int_memory_t memory;
			storage_t store{1, 2, 3, 4};

			memory = store.data();

			CHECK((int*)memory == store.data());
			CHECK_MEMORY(memory, 1, 2, 3, 4);
		}

		THEN("simple_memory_t can be indexed")
		{
			storage_t store{1, 2, 3, 4};
			int_memory_t memory{store.data()};

			CHECK(memory[0] == 1);
			CHECK(memory[1] == 2);
			CHECK(memory[2] == 3);
			CHECK(memory[3] == 4);
		}

		THEN("simple_memory_t can fake pointer arithmetic")
		{
			storage_t store{1, 2, 3, 4};

			auto m1 = int_memory_t{store.data()};
			auto m2 = m1 + 2;
			static_assert(std::is_same_v<decltype(m2), decltype(m1)>);

			CHECK((int*)m2 == ((int*)m1 + 2));
			CHECK_MEMORY(m2, 3, 4);
		}
	}
}

SCENARIO_OF("memory/basic_memory_t", "basic_memory_t behaves nicely")
{
	GIVEN("an empty allocator")
	{
		using empty_allocator = atma::aligned_allocator_t<byte>;
		using memory_t = atma::basic_memory_t<byte, empty_allocator>;

		THEN("basic_memory_t can be default-constructed")
		{
			auto memory = memory_t();
		}

		THEN("basic_memory_t can constructed from a pointer")
		{
			byte* ptr = nullptr;
			auto memory = memory_t(ptr);
		}

		THEN("basic_memory_t can constructed from a pointer")
		{
			byte* ptr = nullptr;
			auto memory = memory_t(ptr);
		}
	}
}

#if 1
SCENARIO_OF("memory/operations", "memory operations behave")
{
	GIVEN("an empty allocator")
	{
		using empty_allocator = atma::aligned_allocator_t<int>;
		using memory_t = atma::basic_memory_t<int, empty_allocator>;

		static_assert(std::is_empty_v<empty_allocator>, "empty_allocator not empty!");

		memory_t m;
		m.allocate(4);


#if 1
		THEN("construct_range works")
		{
			auto dest_storage = std::vector<int>{0, 0, 0, 0, 0, 0};
			auto dest_memory = memory_t(dest_storage.data());

			//atma::memory::construct_range(
			//	atma::dest_range_t{dest_memory, 1, 4},
			//	4);
			//
			//CHECK_MEMORY(dest_memory, 0, 4, 4, 4, 4, 0);

			atma::memory::construct_range(
				atma::dest_range_t{dest_storage, 2},
				4);

			CHECK_MEMORY(dest_memory, 4, 4, 0, 0, 0, 0);
		}

		THEN("copy_construct_range works")
		{
			auto dest_storage = std::vector<int>{0, 0, 0, 0, 0, 0};
			auto src_storage = std::vector<int>{1, 2, 3, 4};

			auto dest_memory = atma::simple_memory_t<int, empty_allocator>(dest_storage.data());
			auto src_memory = atma::simple_memory_t<int, empty_allocator>(src_storage.data());

			atma::memory::copy_construct_range(
				atma::dest_range_t{dest_memory, 1, 4},
				atma::src_range_t(src_memory, 4));

			CHECK_MEMORY(dest_memory, 0, 1, 2, 3, 4, 0);
		}

		THEN("copy_construct_range works - iterator edition")
		{
			auto dest_storage = std::vector<int>{0, 0, 0, 0, 0, 0};
			auto src_storage = std::vector<int>{1, 2, 3};

			auto dest_memory = memory_t(dest_storage.data());

			atma::memory::copy_construct_range(
				atma::dest_range_t{dest_memory, 1, 3},
				atma::src_range_t{src_storage.begin(), src_storage.end()});

			CHECK_MEMORY(dest_memory, 0, 1, 2, 3, 0, 0);
		}
#endif
#if 0
		THEN("move_construct_range works")
		{
			auto dest_storage = std::vector<std::unique_ptr<int>>(2);
			auto src_storage = std::vector<std::unique_ptr<int>>();
			src_storage.emplace_back(std::move(std::make_unique<int>(1)));
			src_storage.emplace_back(std::move(std::make_unique<int>(2)));

			auto dest_memory = atma::simple_memory_t<std::unique_ptr<int>>(dest_storage.data());
			auto src_memory = atma::simple_memory_t<std::unique_ptr<int>>(src_storage.data());

			atma::memory::move_construct_range(
				atma::dest_range_t{dest_memory, 2},
				atma::src_range_t{src_memory, 0, 2});

			CHECK(src_storage[0] == nullptr);
			CHECK(src_storage[1] == nullptr);

			CHECK(*dest_storage[0] == 1);
			CHECK(*dest_storage[1] == 2);
		}

		THEN("move_construct_range works - iterator edition")
		{
			auto dest_storage = std::vector<std::unique_ptr<int>>(2);
			auto src_storage = std::vector<std::unique_ptr<int>>();
			src_storage.emplace_back(std::move(std::make_unique<int>(1)));
			src_storage.emplace_back(std::move(std::make_unique<int>(2)));

			auto dest_memory = atma::simple_memory_t<std::unique_ptr<int>>(dest_storage.data());

			atma::memory::move_construct_range(
				atma::dest_range_t{dest_memory, 2},
				src_storage.begin(), src_storage.end());

			CHECK(src_storage[0] == nullptr);
			CHECK(src_storage[1] == nullptr);

			CHECK(*dest_storage[0] == 1);
			CHECK(*dest_storage[1] == 2);
		}
#endif
	}
}
#endif
