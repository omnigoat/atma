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

#define SCOPED_BASIC_MEMORY_ALLOCATION_II(allocation_name, memory_name, size) auto allocation_name = memory_name.allocate(size); SCOPE_GUARD([&]{memory_name.deallocate(allocation_name);})
#define SCOPED_BASIC_MEMORY_ALLOCATION(memory_name, size) SCOPED_BASIC_MEMORY_ALLOCATION_II(temp_alloc_##__COUNTER__, memory_name, size)


//
// GENERATE_TEMPLATE_TYPE_COMBINATIONS(type_name, combination_seq)
// -----------------------------------------------------------------
//  takes a template-typename from @type_name, and generates all possible combinations
//  from the given seq. example:
//
//    GENERATE_TEMPLATE_TYPE_COMBINATIONS(dragon_t, (int, float)(string, char*))
//
//        gives:
//          dragon_t<int, string>, dragon_t<int, char*>, dragon_t<float, string>, dragon_t<float, char*>
//
#define ATMA_PP_VARIADIC_EXPAND(...) __VA_ARGS__

#define INTERNAL_TEMPLATE_TYPE_COMBINATIONS_m(r, product) ((BOOST_PP_SEQ_ENUM(product)))
#define INTERNAL_TEMPLATE_TYPE_COMBINATIONS_op(s, data, elem) BOOST_PP_TUPLE_TO_SEQ(elem)

#define GENERATE_COMBINATIONS_OF_TUPLES_m(r, product) BOOST_PP_SEQ_TO_TUPLE(product)
#define GENERATE_COMBINATIONS_OF_TUPLES(tupleseqs) \
	BOOST_PP_SEQ_FOR_EACH_PRODUCT(GENERATE_COMBINATIONS_OF_TUPLES_m, \
			BOOST_PP_SEQ_TRANSFORM(INTERNAL_TEMPLATE_TYPE_COMBINATIONS_op, ~, \
				BOOST_PP_VARIADIC_SEQ_TO_SEQ(tupleseqs)))

#define INTERNAL_TEMPLATE_TYPE_COMBINATIONS_M(r, d, i, x) ((d<BOOST_PP_TUPLE_REM_CTOR(x)>))
#define INTERNAL_TEMPLATE_TYPE_COMBINATIONS(type_name, seq) \
	BOOST_PP_SEQ_FOR_EACH_I(INTERNAL_TEMPLATE_TYPE_COMBINATIONS_M, type_name, \
		BOOST_PP_SEQ_FOR_EACH_PRODUCT(INTERNAL_TEMPLATE_TYPE_COMBINATIONS_m, \
			BOOST_PP_SEQ_TRANSFORM(INTERNAL_TEMPLATE_TYPE_COMBINATIONS_op, ~, \
				BOOST_PP_VARIADIC_SEQ_TO_SEQ(seq))))

#define BLAMMO_m(r,d,x) BOOST_PP_TUPLE_PUSH_FRONT(x, d)
#define BLAMMO(type_name, seq) \
	BOOST_PP_SEQ_FOR_EACH(BLAMMO_m, type_name,\
		BOOST_PP_VARIADIC_SEQ_TO_SEQ(GENERATE_COMBINATIONS_OF_TUPLES(seq)))

#define ASDF_ff(...) BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)
#define ASDF_f(a) BOOST_PP_EXPAND(ASDF_ff a) //BOOST_PP_VARIADIC_TO_SEQ((__VA_ARGS__))
	
	//BOOST_PP_SEQ_FOR_EACH(ASDF_ff, ~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define ASDF_m(r,d,x) BOOST_PP_EXPAND(d x)

#define FOR_EACH_COMBINATION(fn, seq) \
	BOOST_PP_SEQ_FOR_EACH(ASDF_m, fn,\
		BOOST_PP_VARIADIC_SEQ_TO_SEQ(GENERATE_COMBINATIONS_OF_TUPLES(seq)))

#define GENERATE_TEMPLATE_TYPE_COMBINATIONS_M(r, d, i, x) BOOST_PP_COMMA_IF(i) BOOST_PP_CAT(ATMA_PP_VARIADIC_EXPAND, x)
#define GENERATE_TEMPLATE_TYPE_COMBINATIONS(type_name, seq) \
	BOOST_PP_SEQ_FOR_EACH_I(GENERATE_TEMPLATE_TYPE_COMBINATIONS_M, ~, INTERNAL_TEMPLATE_TYPE_COMBINATIONS(type_name, seq))

#define WITH_TEMPLATE_TYPE_COMBINATIONS_M(r,d,i,x) BOOST_PP_CAT(d, x)
#define WITH_TEMPLATE_TYPE_COMBINATIONS(fn, type_name, combination_seq) \
	BOOST_PP_SEQ_FOR_EACH_I(WITH_TEMPLATE_TYPE_COMBINATIONS_M, fn, INTERNAL_TEMPLATE_TYPE_COMBINATIONS(type_name, combination_seq))

	//GENERATE_TEMPLATE_TYPE_COMBINATIONS(type_name, combination_seq)


	


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

dragon_t const empty_dragon;


// allow us to test xfer_dest/xfer_src in one set of tests
template <typename Tag, template <typename...> typename Allocator, typename Value>
struct xfer_maker
{
	using allocator_type = Allocator<Value>;
	using value_type = Value;

	using memxfer_t = atma::memxfer_t<Tag, Value, Allocator<Value>>;
	using bounded_memxfer_t = atma::bounded_memxfer_t<Tag, Value, Allocator<Value>>;

	constexpr static auto make = [](auto&&... args)
	{
		if constexpr (std::is_same_v<Tag, atma::dest_memory_tag_t>)
			return atma::xfer_dest(std::forward<decltype(args)>(args)...);
		else
			return atma::xfer_src(std::forward<decltype(args)>(args)...);
	};
};




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

#define test_memory_tags (atma::dest_memory_tag_t, atma::src_memory_tag_t)
#define test_allocators  (std::allocator, atma::arena_allocator_t)
#define test_value_types (int, dragon_t)

#define xfer_type_combinations test_memory_tags test_allocators test_value_types

// test different allocators & simple/complex types in combination
#define xfer_type_list GENERATE_TEMPLATE_TYPE_COMBINATIONS(xfer_maker, xfer_type_combinations)

#define TYPE_TO_STRING_VARAGS(...) TYPE_TO_STRING(__VA_ARGS__); BOOST_PP_TUPLE_ELEM()
//WITH_TEMPLATE_TYPE_COMBINATIONS(TYPE_TO_STRING_VARAGS, xfer_maker, xfer_type_combinations)


// GENERATE_COMBINATIONS_OF_TUPLES
//GENERATE_COMBINATIONS_OF_TUPLES(xfer_type_combinations)

// GENERATE_TEMPLATE_TYPE_COMBINATIONS
//GENERATE_TEMPLATE_TYPE_COMBINATIONS(xfer_maker, xfer_type_combinations)

// INTERNAL_TEMPLATE_TYPE_COMBINATIONS
//INTERNAL_TEMPLATE_TYPE_COMBINATIONS(xfer_maker, xfer_type_combinations)

// BLAMMO
//BLAMMO(xfer_make, xfer_type_combinations)

// type-to-string all our used types
#define TYPE_ALLOCATORS_TO_STRING(allocator_type, value_type) TYPE_TO_STRING(allocator_type<value_type>);
FOR_EACH_COMBINATION(TYPE_ALLOCATORS_TO_STRING, test_allocators test_value_types)

#if 1
SCENARIO_TEMPLATE("a (dest|src)_memxfer_t is directly constructed", xfer, xfer_type_list)
{
	using value_type     = typename xfer::value_type;
	using allocator_type = typename xfer::allocator_type;
	using memxfer_type   = typename xfer::memxfer_t;
	using storage_type   = atma::vector<value_type, allocator_type>;

	//char buf[256];
	//snprintf(buf, 256, "the type '%s' and allocator '%s'", doctest::detail::type_to_string<value_type>(), doctest::detail::type_to_string<allocator_type>());

	GIVEN("hello")
	{
		allocator_type A;

		auto data = storage_type(4);

		if constexpr (std::is_empty_v<allocator_type>)
		{
			THEN("it is constructible from solely pointer")
			{
				[[maybe_unused]] auto d = memxfer_type(data.data());
			
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == data.data());
			}
		}

#if 0
		THEN("it is constructible from an allocator & pointer")
		{
			[[maybe_unused]] auto d = memxfer_type(A, data.get());

			CHECK(MODELS_ARGS(atma::memory_concept, d));
			CHECK(MODELS_NOT_ARGS(atma::bounded_memory_concept, d));

			CHECK(std::data(d) == d.data());
			CHECK(d.data() == data.get());
		}
#endif
	}

#if 0
	GIVEN("the type 'int' & a non-empty allocator")
	{
		using allocator_type = std::pmr::polymorphic_allocator<int>;
		using storage_type = std::vector<int, allocator_type>;
		using dest_type = atma::memxfer_t<tag_type, int, allocator_type>;

		static_assert(!std::is_empty_v<allocator_type>);

		atma::arena_memory_resource_t MR(64, 64);
		allocator_type A{&MR};

		auto data = storage_type{4, A};

		THEN("it is constructible from an allocator & pointer")
		{
			[[maybe_unused]] auto d = memxfer_type(data.get_allocator(), data.data());

			CHECK(MODELS_ARGS(atma::memory_concept, d));
			CHECK(MODELS_NOT_ARGS(atma::bounded_memory_concept, d));

			CHECK(d.data() == data.data());
			CHECK(d.get_allocator() == data.get_allocator());
		}
	}
#endif

}

template <typename F>
struct scope_guard_t
{
	scope_guard_t(F f)
		: f(f)
	{}

	~scope_guard_t()
	{
		f();
	}

	F f;
};

template <typename F>
scope_guard_t(F f) -> scope_guard_t<F>;

#define SCOPE_GUARD(f) scope_guard_t scope_guard_##__COUNTER__{f};

SCENARIO_TEMPLATE("xfer_dest() or xfer_src() is called", xfer, xfer_type_list)
{
	auto& xfer_make = xfer::make;

	GIVEN("the type 'int' & an empty allocator")
	{
		using allocator_type = atma::aligned_allocator_t<int>;
		static_assert(std::is_empty_v<atma::aligned_allocator_t<int>>);

		AND_GIVEN("a vector of size 4")
		{
			using storage_type = std::vector<int, allocator_type>;
			auto storage = storage_type(4);

			WHEN("we call xfer_make with arguments {allocator, pointer}")
			{
				[[maybe_unused]] auto d = xfer_make(storage.get_allocator(), storage.data());

				THEN("the result-type is conceptually an 'unbounded memory'")
				{
					CHECK(MODELS_ARGS(atma::memory_concept, d));
					CHECK(MODELS_NOT_ARGS(atma::bounded_memory_concept, d));
				}

				THEN("the result has the same allocator")
				{
					CHECK(atma::get_allocator(d) == d.get_allocator());
					CHECK(d.get_allocator() != storage.get_allocator());
				}

				THEN("the result has the same value")
				{
					CHECK(std::data(d) == d.data());
					CHECK(d.data() == storage.data());
				}
			}

			THEN("we can call with arguments {pointer}")
			{
				[[maybe_unused]] auto d = xfer_make(storage.data());

				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_NOT_ARGS(atma::bounded_memory_concept, d));

				CHECK(std::data(d) == d.data());
				CHECK(d.data() == storage.data());
			}

			THEN("we can call with arguments {allocator, pointer, size}")
			{
				[[maybe_unused]] auto d = xfer_make(storage.get_allocator(), storage.data(), storage.size());

				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_ARGS(atma::bounded_memory_concept, d));

				CHECK(atma::get_allocator(d) == d.get_allocator());
				CHECK(d.get_allocator() == storage.get_allocator());
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == storage.data());
				CHECK(std::size(d) == d.size());
				CHECK(d.size() == storage.size());
				CHECK_FALSE(d.empty());
			}

			THEN("we can call with arguments {pointer, size}")
			{
				[[maybe_unused]] auto d = xfer_make(storage.data(), storage.size());

				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_ARGS(atma::bounded_memory_concept, d));

				CHECK(std::data(d) == d.data());
				CHECK(d.data() == storage.data());
				CHECK(std::size(d) == d.size());
				CHECK(d.size() == storage.size());
				CHECK_FALSE(d.empty());
			}


			THEN("we can call with arguments {basic-memory}")
			{
				atma::basic_memory_t<xfer::value_type, xfer::allocator_type> memory;
				SCOPED_BASIC_MEMORY_ALLOCATION(memory, 4);

				[[maybe_unused]] auto d = xfer_make(memory);

				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_NOT_ARGS(atma::bounded_memory_concept, d));

				CHECK(atma::get_allocator(d) == d.get_allocator());
				CHECK(d.get_allocator() == memory.get_allocator());
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == memory.data());
			}

			THEN("we can call with arguments {basic-memory, size}")
			{
				auto const test_size = 2;

				atma::basic_memory_t<xfer::value_type, xfer::allocator_type> memory;
				SCOPED_BASIC_MEMORY_ALLOCATION(memory, 4);

				[[maybe_unused]] auto d = xfer_make(memory, test_size);

				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_ARGS(atma::bounded_memory_concept, d));

				CHECK(atma::get_allocator(d) == d.get_allocator());
				CHECK(d.get_allocator() == memory.get_allocator());
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == memory.data());
				CHECK(std::size(d) == 3);
				CHECK(d.size() == test_size);
				CHECK_FALSE(d.empty());
			}
		}
	}
}

#if 0
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
#endif


SCENARIO_OF("memory/operations", "memory_default_construct is called")
{
	GIVEN("an empty allocator and the dragon-type")
	{
		using allocator_type = atma::aligned_allocator_t<dragon_t>;
		using memory_t = atma::basic_memory_t<dragon_t, allocator_type>;

		static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

		GIVEN("memory pointing to uninitialized-memory")
		{
			auto dest_storage = std::unique_ptr<byte[]>(new byte[sizeof dragon_t * 6]);
			auto dest_memory = memory_t(reinterpret_cast<dragon_t*>(dest_storage.get()));

			THEN("memory_default_construct default-constructs the whole range")
			{
				atma::memory_default_construct(atma::xfer_dest(dest_memory, 6));

				CHECK_MEMORY(dest_memory,
					empty_dragon, empty_dragon, empty_dragon,
					empty_dragon, empty_dragon, empty_dragon);
			}
		}
	}
}



SCENARIO_OF("memory/operations", "range_construct is called")
{
	GIVEN("an empty allocator and the dragon-type")
	{
		using value_type = dragon_t;
		using allocator_type = atma::aligned_allocator_t<dragon_t>;
		using memory_t = atma::basic_memory_t<value_type, allocator_type>;

		static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

		dragon_t const oliver{"oliver", 33};

		GIVEN("memory pointing to an lvalue vector")
		{
			auto dest_storage = std::vector<value_type>(6, empty_dragon);
			auto dest_memory = memory_t(dest_storage.data());

			THEN("range_construct can construct the whole range with a direct constructor")
			{
				atma::memory_construct(
					dest_storage,
					"oliver", 33);

				CHECK_MEMORY(dest_memory,
					oliver, oliver, oliver,
					oliver, oliver, oliver);
			}

			THEN("a partial-range can be constructed via a direct constructor")
			{
				atma::memory_construct(
					atma::xfer_dest(dest_memory, 4),
					"oliver", 33);

				CHECK_MEMORY(dest_memory,
					oliver, oliver, oliver, oliver,
					empty_dragon, empty_dragon);
			}

			THEN("a partial-range can be constructed via a direct constructor")
			{
				atma::memory_construct(
					atma::xfer_dest(dest_memory + 1, 4),
					"oliver", 33);

				CHECK_MEMORY(dest_memory,
					empty_dragon,
					oliver, oliver, oliver, oliver,
					empty_dragon);
			}

			THEN("a partial-range can be constructed via the copy-constructor")
			{
				atma::memory_construct(
					atma::xfer_dest(dest_memory + 1, 4),
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
					dragon_t dragons_dest[4] = {};
					dragon_t dragons[4] = { oliver, henry, marcie, rachael };

					atma::memory_copy_construct(
						atma::xfer_dest(dest_storage, 4),
						atma::xfer_src(src_storage));

					CHECK_MEMORY(dest_memory,
						oliver, henry, marcie, rachael,
						empty_dragon, empty_dragon);
				}

				THEN("memory_copy_construct can copy-construct the middle of the range")
				{
					atma::memory_copy_construct(
						atma::xfer_dest(dest_memory + 1, 4),
						atma::xfer_src(src_storage));

					CHECK_MEMORY(dest_memory,
						empty_dragon,
						oliver, henry, marcie, rachael,
						empty_dragon);
				}

				THEN("memory_copy_construct can copy-construct bits of both ranges")
				{
					atma::memory_copy_construct(
						atma::xfer_dest(dest_memory + 4, 2),
						atma::xfer_src(src_storage, 2, 2));

					CHECK_MEMORY(dest_memory,
						empty_dragon, empty_dragon, empty_dragon, empty_dragon,
						marcie, rachael);
				}

				THEN("memory_copy_construct can copy-construct from iterateors")
				{
					atma::memory_copy_construct(
						atma::xfer_dest(dest_memory + 2, 4),
						src_storage.begin(), src_storage.end());

					CHECK_MEMORY(dest_memory,
						empty_dragon, empty_dragon,
						oliver, henry, marcie, rachael);
				}

				THEN("")
				{
					auto dest2_storage = std::vector<value_type>(4, empty_dragon);

					//static_assert(atma::concepts::models<atma::assignable_concept, typename decltype(dest2_storage)::value_type>::value);
					//static_assert(atma::concepts::models_v<atma::dest_memory_range_concept, decltype(dest2_storage)>);

					atma::memory_copy_construct(dest2_storage, src_storage);
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
					//MODELS_ARGS(dest_memory_concept, dest),
					//MODELS_ARGS(src_memory_concept, src),
					//MODELS_ARGS(bounded_memory_concept, dest),
					//MODELS_ARGS(bounded_memory_concept, src))

					static_assert(MODELS_ARGS(atma::dest_memory_concept, atma::xfer_dest(dest_memory, 4)));

					atma::memory_move_construct(
						atma::xfer_dest(dest_memory, 4),
						atma::xfer_src(src_storage.data(), src_storage.size()));

					CHECK_MEMORY(dest_memory,
						oliver, henry, marcie, rachael,
						empty_dragon, empty_dragon);

					CHECK_VECTOR(src_storage,
						empty_dragon, empty_dragon, empty_dragon, empty_dragon);
				}

				THEN("range_move_construct can move part of the range")
				{
					atma::memory_move_construct(
						atma::xfer_dest(dest_memory, 2),
						atma::xfer_src(src_storage, 0, 2));

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

		dragon_t const oliver{"oliver", 33};
		dragon_t const henry{"henry", 24};
		dragon_t const marcie{"marcie", 27};
		dragon_t const rachael{"rachael", 19};

		auto dest_storage = std::vector<value_type>{oliver, henry, marcie, rachael};
		auto dest_memory = memory_t(dest_storage.data());

		THEN("destruct calls the destructor of the whole range")
		{
			atma::memory_destruct(
				atma::xfer_dest(dest_memory, 4));

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
					atma::xfer_dest(dest_memory, 2),
					atma::xfer_src(src_storage, 2, 2));

				CHECK_VECTOR(dest_storage,
					7, 8, 3, 4);
			}

			THEN("memmove performs overlapping regions correctly")
			{
				atma::memory::memmove(
					atma::xfer_dest(dest_memory, 2),
					atma::xfer_src(dest_memory + 1, 2));

				CHECK_VECTOR(dest_storage,
					2, 3, 3, 4);
			}
		}
	}
}
#endif
