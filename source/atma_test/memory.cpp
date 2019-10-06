#include <atma/unit_test.hpp>
#include <atma/memory.hpp>
#include <atma/string.hpp>
#include <atma/functor.hpp>

#include <numeric>

#define CHECK_MEMORY_II_m(r, v, i, elem) \
	CHECK_EQ(((decltype(v)::pointer)v)[i], elem);

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

SCENARIO_OF("memory/basic_memory_t", "basic_memory_t construction")
{
	GIVEN("an empty allocator")
	{
		using empty_allocator = atma::aligned_allocator_t<int>;
		using storage_t = std::vector<int>;
		using int_memory_t = atma::basic_memory_t<int, empty_allocator>;

		THEN("basic_memory_t can be default-constructed")
		{
			int_memory_t memory;

			CHECK(sizeof(memory) == sizeof(int*));
			CHECK((int*)memory == nullptr);
		}

		THEN("basic_memory_t can constructed from a pointer & allocator")
		{
			storage_t store{1, 2, 3, 4};
			auto memory = int_memory_t{store.data(), empty_allocator()};

			CHECK(sizeof(memory) == sizeof(int*));
			CHECK((int*)memory == store.data());
			CHECK_MEMORY(memory, 1, 2, 3, 4);
		}

		THEN("basic_memory_t has working deduction guides")
		{
			int* data = nullptr;
			auto m1 = atma::basic_memory_t{data};
			auto m2 = atma::basic_memory_t{data, empty_allocator()};
		}

		THEN("basic_memory_t can be assigned")
		{
			int_memory_t memory;
			storage_t store{1, 2, 3, 4};

			memory = store.data();

			CHECK((int*)memory == store.data());
			CHECK_MEMORY(memory, 1, 2, 3, 4);
		}

		THEN("basic_memory_t can be indexed")
		{
			storage_t store{1, 2, 3, 4};
			int_memory_t memory{store.data()};

			CHECK(memory[0] == 1);
			CHECK(memory[1] == 2);
			CHECK(memory[2] == 3);
			CHECK(memory[3] == 4);
		}

		THEN("basic_memory_t can fake pointer arithmetic")
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

//TYPE_TO_STRING(atma::dest_memxfer_range_t<int>);
//TYPE_TO_STRING(atma::src_memxfer_range_t<int>);

SCENARIO_TEMPLATE("a memory-xfer-range is contructed ", range_type, atma::dest_bounded_memxfer_range_t<int, std::allocator<int>>, atma::src_bounded_memxfer_range_t<int, std::allocator<int>>)
{
	GIVEN("the types int & std::allocator<int>")
	{
		THEN("it is default constructible")
		{
			range_type d;
			ATMA_UNUSED(d);
		}

		THEN("it constructs from pointer and size")
		{
			int const numsize = 4;
			int numbers[numsize] = { 1, 2, 3, 4 };

			range_type d(numbers, numsize);
			
			CHECK(d.empty() == false);
			CHECK(d.size() == numsize);
			CHECK(d.begin() == numbers);
			CHECK(d.end() == numbers + numsize);
		}

		THEN("it constructs from pointer, offset, and size")
		{
			int const offset = 2;
			int const numsize = 4;
			int numbers[numsize] = { 1, 2, 3, 4 };

			int const range_size = numsize - offset;
			range_type d(numbers, offset, range_size);

			CHECK(d.empty() == false);
			CHECK(d.size() == range_size);
			CHECK(d.begin() == numbers + offset);
			CHECK(d.end() == numbers + numsize);
		}

		THEN("it constructs from generic range (vector)")
		{
			auto numbers = std::vector<int>{1, 2, 3, 4};

			range_type d(numbers);

			CHECK(d.empty() == false);
			CHECK(d.size() == numbers.size());
			CHECK(d.begin() == numbers.data());
			CHECK(d.end() == numbers.data() + numbers.size());
			CHECK(d.begin() == &*numbers.begin());
		}

		THEN("it constructs from vector and size")
		{
			auto numbers = std::vector<int>{ 1, 2, 3, 4 };

			range_type d(numbers, 3);

			CHECK(d.empty() == false);
			CHECK(d.size() == 3);
			CHECK(d.begin() == numbers.data());
			CHECK(d.end() == numbers.data() + 3);
			CHECK(d.begin() == &*numbers.begin());
		}

		THEN("it constructs from vector, offset, and size")
		{
			auto numbers = std::vector<int>{ 1, 2, 3, 4 };

			range_type d(numbers, 1, 3);

			CHECK(d.empty() == false);
			CHECK(d.size() == 3);
			CHECK(d.begin() == numbers.data() + 1);
			CHECK(d.end() == numbers.data() + 4);
			CHECK(d.begin() == &*numbers.begin() + 1);
		}

		THEN("it constructs from a basic_memory_t & size")
		{
			auto numbers = std::vector<int>{ 1, 2, 3, 4 };
			auto mem = atma::basic_memory_t<int>(numbers.data());

			range_type d(mem, numbers.size());

			CHECK(d.empty() == false);
			CHECK(d.size() == numbers.size());
			CHECK(d.begin() == mem.data());
			CHECK(d.end() == mem.data() + numbers.size());
			CHECK(d.begin() == numbers.data());
		}

		THEN("it constructs from a basic_memory_t, offset, and size")
		{
			auto numbers = std::vector<int>{ 1, 2, 3, 4 };
			auto mem = atma::basic_memory_t<int>(numbers.data());

			range_type d(mem + 2, numbers.size() - 2);

			CHECK(d.empty() == false);
			CHECK(d.size() == 2);
			CHECK(d.begin() == mem.data() + 2);
			CHECK(d.end() == mem.data() + numbers.size());
			CHECK(d.begin() == numbers.data() + 2);
		}
	}
}

struct dragon_t
{
	dragon_t() = default;

	dragon_t(atma::string const& name, int age)
		: name(name), age(age)
	{}

	dragon_t(dragon_t const&) = default;
	dragon_t(dragon_t&& rhs)
		: name(std::move(rhs.name))
		, age(rhs.age)
	{
		rhs.age = 0;
	}

	~dragon_t()
	{
		name.clear();
		age = 0;
	}

	atma::string name;
	int age = 0;

	bool operator == (dragon_t const& rhs) const
	{
		return name == rhs.name && age == rhs.age;
	}
};

std::ostream& operator << (std::ostream& stream, dragon_t const& dragon)
{
	return stream << "dragon{" << dragon.name << ", " << dragon.age << '}';
}

TYPE_TO_STRING(dragon_t);

SCENARIO_OF("memory/operations", "range_construct is called")
{
	GIVEN("an empty allocator and the dragon-type")
	{
		using value_type = dragon_t;
		using allocator_type = atma::aligned_allocator_t<dragon_t>;
		using memory_t = atma::basic_memory_t<value_type, allocator_type>;

		static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

		dragon_t const empty_dragon;
		dragon_t const oliver{"oliver", 33};

		GIVEN("memory pointing to an lvalue vector")
		{
			auto dest_storage = std::vector<value_type>(6, empty_dragon);
			auto dest_memory = memory_t(dest_storage.data());

			THEN("range_construct can construct the whole range with a direct constructor")
			{
				atma::memory::range_construct(
					atma::dest_range(dest_memory, dest_storage.size()),
					"oliver", 33);

				CHECK_MEMORY(dest_memory,
					oliver, oliver, oliver,
					oliver, oliver, oliver);
			}

			THEN("a partial-range can be constructed via a direct constructor")
			{
				atma::memory::range_construct(
					atma::dest_range(dest_memory, 4),
					"oliver", 33);

				CHECK_MEMORY(dest_memory,
					oliver, oliver, oliver, oliver,
					empty_dragon, empty_dragon);
			}

			THEN("a partial-range can be constructed via a direct constructor")
			{
				atma::memory::range_construct(
					atma::dest_range(dest_memory + 1, 4),
					"oliver", 33);

				CHECK_MEMORY(dest_memory,
					empty_dragon,
					oliver, oliver, oliver, oliver,
					empty_dragon);
			}

			THEN("a partial-range can be constructed via the copy-constructor")
			{
				atma::memory::range_construct(
					atma::dest_range(dest_memory + 1, 4),
					oliver);

				CHECK_MEMORY(dest_memory,
					empty_dragon,
					oliver, oliver, oliver, oliver,
					empty_dragon);
			}
		}
	}
}

SCENARIO_OF("memory/operations", "range_copy_construct is called")
{
	GIVEN("an empty allocator and the dragon-type")
	{
		using value_type = dragon_t;
		using allocator_type = atma::aligned_allocator_t<dragon_t>;
		using memory_t = atma::basic_memory_t<value_type, allocator_type>;

		static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

		dragon_t const empty_dragon;
		dragon_t const oliver{"oliver", 33};
		dragon_t const henry{"henry", 24};
		dragon_t const marcie{"marcie", 27};
		dragon_t const rachael{"rachael", 19};

		GIVEN("destination memory pointing to an lvalue vector")
		{
			auto dest_storage = std::vector<value_type>(6, empty_dragon);
			auto dest_memory = memory_t(dest_storage.data());

			GIVEN("source memory pointing to a const-lvalue vector")
			{
				auto const src_storage = std::vector<value_type>{oliver, henry, marcie, rachael};

				THEN("range_copy_construct can copy-construct the beginning of the range")
				{
					atma::memory2::range_copy_construct(
						atma::dest_range(dest_memory, 4),
						atma::src_range(src_storage),
						4);

					atma::memory2::range_copy_construct(
						atma::dest_range(dest_storage, 4),
						atma::src_range(src_storage));

					atma::memory::range_copy_construct(
						atma::dest_range(dest_memory, 4),
						atma::src_range(src_storage));

					CHECK_MEMORY(dest_memory,
						oliver, henry, marcie, rachael,
						empty_dragon, empty_dragon);
				}

				THEN("range_copy_construct can copy-construct the middle of the range")
				{
					atma::memory::range_copy_construct(
						atma::dest_range(dest_memory + 1, 4),
						atma::src_range(src_storage));

					CHECK_MEMORY(dest_memory,
						empty_dragon,
						oliver, henry, marcie, rachael,
						empty_dragon);
				}

				THEN("range_copy_construct can copy-construct bits of both ranges")
				{
					atma::memory::range_copy_construct(
						atma::dest_range(dest_memory + 4, 2),
						atma::src_range(src_storage, 2, 2));

					CHECK_MEMORY(dest_memory,
						empty_dragon, empty_dragon, empty_dragon, empty_dragon,
						marcie, rachael);
				}

				THEN("")
				{
					auto dest2_storage = std::vector<value_type>(4, empty_dragon);

					//static_assert(atma::concepts::models<atma::assignable_concept, typename decltype(dest2_storage)::value_type>::value);
					//static_assert(atma::concepts::models_v<atma::dest_memory_range_concept, decltype(dest2_storage)>);

					atma::memory::range_copy_construct(dest2_storage, src_storage);
				}
			}
		}
	}
}


SCENARIO_OF("memory/operations", "range_move_construct is called")
{
	GIVEN("an empty allocator and the dragon-type")
	{
		using value_type = dragon_t;
		using allocator_type = atma::aligned_allocator_t<dragon_t>;
		using memory_t = atma::basic_memory_t<value_type, allocator_type>;

		static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

		dragon_t const empty_dragon;
		dragon_t const oliver{"oliver", 33};
		dragon_t const henry{"henry", 24};
		dragon_t const marcie{"marcie", 27};
		dragon_t const rachael{"rachael", 19};

		GIVEN("destination memory pointing to an lvalue vector")
		{
			auto dest_storage = std::vector<value_type>(6, empty_dragon);
			auto dest_memory = memory_t(dest_storage.data());

			GIVEN("source memory pointing to an lvalue vector")
			{
				auto src_storage = std::vector<value_type>{oliver, henry, marcie, rachael};

				THEN("range_move_construct can move part of the range")
				{
					atma::memory::range_move_construct(
						atma::dest_range(dest_memory, 4),
						atma::src_range(src_storage.begin(), src_storage.end()));

					CHECK_MEMORY(dest_memory,
						oliver, henry, marcie, rachael,
						empty_dragon, empty_dragon);

					CHECK_VECTOR(src_storage,
						empty_dragon, empty_dragon, empty_dragon, empty_dragon);
				}

				THEN("range_move_construct can move part of the range")
				{
					atma::memory::range_move_construct(
						atma::dest_range(dest_memory, 2),
						atma::src_range(src_storage, 0, 2));

					CHECK_MEMORY(dest_memory,
						oliver, henry,
						empty_dragon, empty_dragon, empty_dragon, empty_dragon);

					CHECK_VECTOR(src_storage,
						empty_dragon, empty_dragon,
						marcie, rachael);
				}
			}
		}
	}
}

SCENARIO_OF("memory/operations", "destruct is called")
{
	GIVEN("a memory-range of instantiated dragons")
	{
		using value_type = dragon_t;
		using allocator_type = atma::aligned_allocator_t<dragon_t>;
		using memory_t = atma::basic_memory_t<value_type, allocator_type>;

		static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

		dragon_t const empty_dragon;
		dragon_t const oliver{"oliver", 33};
		dragon_t const henry{"henry", 24};
		dragon_t const marcie{"marcie", 27};
		dragon_t const rachael{"rachael", 19};

		auto dest_storage = std::vector<value_type>{oliver, henry, marcie, rachael};
		auto dest_memory = memory_t(dest_storage.data());

		THEN("destruct calls the destructor of the whole range")
		{
			atma::memory::range_destruct(
				atma::dest_range(dest_memory, 4));

			CHECK_VECTOR(dest_storage,
				empty_dragon, empty_dragon, empty_dragon, empty_dragon);
		}
	}
}

SCENARIO_OF("memory/operations", "memcpy or memmove is called")
{
	GIVEN("a memory-range of instantiated integers")
	{
		using value_type = int;
		using allocator_type = atma::aligned_allocator_t<value_type>;
		using memory_t = atma::basic_memory_t<value_type, allocator_type>;

		static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

		auto dest_storage = std::vector<value_type>{1, 2, 3, 4};
		auto dest_memory = memory_t(dest_storage.data());

		//atma::memory::range_construct<int, allocator_type>(dest_memory, 0);

		GIVEN("source memory pointing to an lvalue vector")
		{
			auto src_storage = std::vector<value_type>{5, 6, 7, 8};

			THEN("memcpy performs correctly")
			{
				atma::memory::memcpy(
					atma::dest_range(dest_memory, 2),
					atma::src_range(src_storage, 2, 2));

				CHECK_VECTOR(dest_storage,
					7, 8, 3, 4);
			}

			THEN("memmove performs overlapping regions correctly")
			{
				atma::memory::memmove(
					atma::dest_range(dest_memory, 2),
					atma::src_range(dest_memory + 1, 2));

				CHECK_VECTOR(dest_storage,
					2, 3, 3, 4);
			}
		}
	}
}

