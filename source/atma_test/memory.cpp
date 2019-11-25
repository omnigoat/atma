#include <atma/unit_test.hpp>
#include <atma/memory.hpp>
#include <atma/string.hpp>
#include <atma/functor.hpp>
#include <atma/preprocessor.hpp>

#include <numeric>

#define CHECK_MEMORY_II_m(r, v, i, elem) \
	CHECK_EQ(((typename decltype(v)::pointer)v)[i], elem);

#define CHECK_MEMORY_II(v, seq) \
	BOOST_PP_SEQ_FOR_EACH_I(CHECK_MEMORY_II_m, v, seq)

#define CHECK_MEMORY(v, ...) \
	CHECK_MEMORY_II(v, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define SCOPED_BASIC_MEMORY_ALLOCATION_II(allocation_name, memory_name, size) auto allocation_name = memory_name.allocate(size); SCOPE_GUARD([&]{memory_name.deallocate(allocation_name);})
#define SCOPED_BASIC_MEMORY_ALLOCATION(memory_name, size) SCOPED_BASIC_MEMORY_ALLOCATION_II(temp_alloc_##__COUNTER__, memory_name, size)


template <typename T>
struct xfer_type_info_t;

#define DEFINE_VALUE_TYPE_FOR_TESTING_m(r,value_type,i,x) \
	inline static value_type const compar##i = value_type(BOOST_PP_TUPLE_REM_CTOR(x)); \

#define DEFINE_VALUE_TYPE_FOR_TESTING(value_type, ...) \
	template <> struct xfer_type_info_t<value_type> \
	{ \
		BOOST_PP_SEQ_FOR_EACH_I(DEFINE_VALUE_TYPE_FOR_TESTING_m, value_type, BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)) \
		\
		constexpr static auto curry_direct_construct_args = [](auto&& f, auto&&... args) \
		{ \
			return f(std::forward<decltype(args)>(args)...,  \
				BOOST_PP_TUPLE_REM_CTOR( \
					BOOST_PP_SEQ_HEAD( \
						BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)))); \
		}; \
	};

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

	inline static value_type const compar = xfer_type_info_t<value_type>::compar0;
	constexpr static auto curry_direct_construct_args = xfer_type_info_t<value_type>::curry_direct_construct_args;
};




//
// dragon_t
// -----------
//   test type
//
struct dragon_t
{
	constexpr dragon_t() = default;

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


DEFINE_VALUE_TYPE_FOR_TESTING(dragon_t, \
	("oliver", 33) \
	("henry", 21) \
	("marcie", 27) \
	("rachael", 19));

DEFINE_VALUE_TYPE_FOR_TESTING(int, (1)(2)(3)(4));



//
//
//
//
//
#define test_memory_tags (atma::dest_memory_tag_t, atma::src_memory_tag_t)
#define test_allocators  (std::allocator, atma::arena_allocator_t)
#define test_value_types (int, dragon_t)

#define xfer_type_combinations \
	test_memory_tags \
	test_allocators \
	test_value_types

// every combination of tags/allocator_types/value_types
#define xfer_type_list_m(i, ...) BOOST_PP_COMMA_IF(i) __VA_ARGS__
#define xfer_type_list FOR_EACH_TEMPLATE_TYPE_COMBINATION(xfer_type_list_m, \
	GENERATE_TEMPLATE_TYPE_COMBINATIONS(xfer_maker, xfer_type_combinations))

// tuples of <allocator_type, value_type>
#define allocator_value_tuples_m(i, d, allocator_type, value_type) BOOST_PP_COMMA_IF(i) std::tuple<allocator_type<value_type>, value_type>
#define allocator_value_tuples FOR_EACH_COMBINATION(allocator_value_tuples_m, ~, GENERATE_COMBINATIONS_OF_TUPLES(test_allocators test_value_types))

// type-to-string all our allocators
#define TYPE_ALLOCATORS_TO_STRING(i, d, allocator_type, value_type) TYPE_TO_STRING(allocator_type<value_type>);
FOR_EACH_COMBINATION(TYPE_ALLOCATORS_TO_STRING, ~, GENERATE_COMBINATIONS_OF_TUPLES(test_allocators test_value_types))


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



SCENARIO_TEMPLATE("a (dest|src)_memxfer_t is directly constructed", xfer, xfer_type_list)
{
	using value_type     = typename xfer::value_type;
	using allocator_type = typename xfer::allocator_type;
	using memxfer_type   = typename xfer::memxfer_t;
	using storage_type   = atma::vector<value_type, allocator_type>;

	GIVEN("reliable storage of a type")
	{
		auto storage = storage_type(4);

		if constexpr (std::is_empty_v<allocator_type>)
		{
			WHEN("it is constructible from solely pointer")
			{
				[[maybe_unused]] auto d = memxfer_type(storage.data());
			
				THEN("the result-type is conceptually an 'unbounded memory'")
				{
					CHECK(MODELS_ARGS(atma::memory_concept, d));
					CHECK(MODELS_NOT_ARGS(atma::bounded_memory_concept, d));
				}

				THEN("the result has the same value")
				{
					CHECK(std::data(d) == d.data());
					CHECK(d.data() == storage.data());
				}
			}
		}

		WHEN("it is directly constructed from an allocator & pointer")
		{
			[[maybe_unused]] auto d = memxfer_type(storage.get_allocator(), storage.data());

			THEN("the result-type is conceptually an 'unbounded memory'")
			{
				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_NOT_ARGS(atma::bounded_memory_concept, d));
			}

			THEN("the result has the same allocator")
			{
				CHECK(atma::get_allocator(d) == d.get_allocator());
				CHECK(d.get_allocator() == storage.get_allocator());
			}

			THEN("the result has the same value")
			{
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == storage.data());
			}
		}
	}
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

	GIVEN("an allocator & storage of a given type")
	{
		using value_type     = typename xfer::value_type;
		using allocator_type = typename xfer::allocator_type;
		using memxfer_type   = typename xfer::memxfer_t;
		using storage_type   = atma::vector<value_type, allocator_type>;

		WHEN("we call xfer_make with arguments {allocator, pointer}")
		{
			auto storage = storage_type(4);

			[[maybe_unused]] auto d = xfer_make(storage.get_allocator(), storage.data());

			THEN("the result-type is conceptually an 'unbounded memory'")
			{
				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_NOT_ARGS(atma::bounded_memory_concept, d));
			}

			THEN("the result has the same allocator")
			{
				CHECK(atma::get_allocator(d) == d.get_allocator());
				CHECK(d.get_allocator() == storage.get_allocator());
			}

			THEN("the result has the same value")
			{
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == storage.data());
			}
		}

		WHEN("we call xfer_make with arguments {pointer}")
		{
			auto storage = storage_type(4);

			[[maybe_unused]] auto d = xfer_make(storage.data());

			THEN("the result-type is conceptually an 'unbounded memory'")
			{
				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_NOT_ARGS(atma::bounded_memory_concept, d));
			}

			THEN("the result has the same value")
			{
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == storage.data());
			}
		}

		WHEN("we call xfer_make with arguments {allocator, pointer, size}")
		{
			auto storage = storage_type(4);

			[[maybe_unused]] auto d = xfer_make(storage.get_allocator(), storage.data(), storage.size());

			THEN("the result-type is conceptually a 'bounded memory'")
			{
				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_ARGS(atma::bounded_memory_concept, d));
			}

			THEN("the result has the same allocator")
			{
				CHECK(atma::get_allocator(d) == d.get_allocator());
				CHECK(d.get_allocator() == storage.get_allocator());
			}

			THEN("the result has the same value")
			{
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == storage.data());
			}

			THEN("the result has the same size")
			{
				CHECK_FALSE(d.empty());
				CHECK(std::size(d) == d.size());
				CHECK(d.size() == storage.size());
			}
		}

		THEN("we can call with arguments {pointer, size}")
		{
			auto storage = storage_type(4);

			[[maybe_unused]] auto d = xfer_make(storage.data(), storage.size());

			THEN("the result-type is conceptually a 'bounded memory'")
			{
				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_ARGS(atma::bounded_memory_concept, d));
			}

			THEN("the result has the same value")
			{
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == storage.data());
			}

			THEN("the result has the same size")
			{
				CHECK_FALSE(d.empty());
				CHECK(std::size(d) == d.size());
				CHECK(d.size() == storage.size());
			}
		}


		THEN("we can call with arguments {basic-memory}")
		{
			atma::basic_memory_t<xfer::value_type, xfer::allocator_type> memory;
			SCOPED_BASIC_MEMORY_ALLOCATION(memory, 4);

			[[maybe_unused]] auto d = xfer_make(memory);

			THEN("the result-type is conceptually an 'unbounded memory'")
			{
				CHECK(MODELS_ARGS(atma::memory_concept, d));
				CHECK(MODELS_NOT_ARGS(atma::bounded_memory_concept, d));
			}

			THEN("the result has the same allocator")
			{
				CHECK(atma::get_allocator(d) == d.get_allocator());
				CHECK(d.get_allocator() == memory.get_allocator());
			}

			THEN("the result has the same value")
			{
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == memory.data());
			}
		}

		THEN("we can call with arguments {basic-memory, size}")
		{
			auto const test_size = 2;

			auto memory = atma::basic_memory_t<value_type, allocator_type>();
			SCOPED_BASIC_MEMORY_ALLOCATION(memory, 4);

			[[maybe_unused]] auto d = xfer_make(memory, test_size);

			CHECK(MODELS_ARGS(atma::memory_concept, d));
			CHECK(MODELS_ARGS(atma::bounded_memory_concept, d));

			THEN("the result has the same allocator")
			{
				CHECK(atma::get_allocator(d) == d.get_allocator());
				CHECK(d.get_allocator() == memory.get_allocator());
			}

			THEN("the result has the same value")
			{
				CHECK(std::data(d) == d.data());
				CHECK(d.data() == memory.data());
			}

			THEN("the result has the same size")
			{
				CHECK_FALSE(d.empty());
				CHECK(std::size(d) == d.size());
				CHECK(d.size() == test_size);
			}
		}
	}
}


SCENARIO_TEMPLATE("memory_default_construct is called", xfer, allocator_value_tuples)
{
	using allocator_type = std::tuple_element_t<0, xfer>;
	using value_type     = std::tuple_element_t<1, xfer>;
	using storage_type   = atma::vector<value_type, allocator_type>;

	using memory_t = atma::basic_memory_t<value_type, allocator_type>;

	// non-class types don't really make sense to default-construct, because
	// their contents are undetermined, i.e., random
	if constexpr (std::is_class_v<value_type>)
	{
		GIVEN("'defval', a default-initialized value-type (value-type is a class-type)")
		GIVEN("memory pointing to uninitialized-memory of six (6) elements")
		{
			const auto storage_size = 6u;
			[[maybe_unused]] value_type const defval;

			auto dest_storage = std::unique_ptr<value_type[]>(new value_type[storage_size]);
			auto dest_memory = memory_t(dest_storage.get());

			WHEN("memory_default_construct is called upon whole range")
			{
				atma::memory_default_construct(atma::xfer_dest(dest_memory, storage_size));

				THEN("every element equates to defval")
				{
					CHECK_MEMORY(dest_memory,
						defval, defval, defval,
						defval, defval, defval);
				}
			}
		}
	}
}

SCENARIO_TEMPLATE("memory_value_construct is called", xfer, allocator_value_tuples)
{
	using allocator_type = std::tuple_element_t<0, xfer>;
	using value_type     = std::tuple_element_t<1, xfer>;
	using storage_type   = atma::vector<value_type, allocator_type>;

	using memory_t = atma::basic_memory_t<value_type, allocator_type>;

	GIVEN("'valval', a value-initialized value-type")
	GIVEN("memory pointing to uninitialized-memory of six (6) elements")
	{
		const auto storage_size = 6u;
		[[maybe_unused]] auto const valval = value_type();

		auto dest_storage = std::unique_ptr<value_type[]>(new value_type[storage_size]);
		auto dest_memory = memory_t(dest_storage.get());

		WHEN("memory_value_construct is called upon the whole range")
		{
			atma::memory_value_construct(atma::xfer_dest(dest_memory, storage_size));

			THEN("the whole range is value-constructed")
			{
				CHECK_MEMORY(dest_memory,
					valval, valval, valval,
					valval, valval, valval);
			}
		}
	}
}

SCENARIO_TEMPLATE("memory_construct is called", xfer, allocator_value_tuples)
{
	using allocator_type = std::tuple_element_t<0, xfer>;
	using value_type     = std::tuple_element_t<1, xfer>;
	using storage_type   = atma::vector<value_type, allocator_type>;

	using value_type_info = xfer_type_info_t<value_type>;

	GIVEN("'defval', a value-initialized instance of our value_type")
	GIVEN("'compar', a direct-initialized instance of our value_type, with a value known to us")
	GIVEN("a vector of six (6) value-initialized elements")
	{
		auto const defval = value_type();
		auto const compar = value_type_info::compar0;

		auto dest_storage = std::vector<value_type>(6, defval);
		auto dest_memory = atma::basic_memory_t<value_type, allocator_type>(dest_storage.data());

		WHEN("memory_construct is called upon a vector with arguments for a direct constructor")
		{
			value_type_info::curry_direct_construct_args(atma::memory_construct, dest_storage);

			THEN("every element in the vector equates to the compar")
			{
				CHECK_MEMORY(dest_memory,
					compar, compar, compar,
					compar, compar, compar);
			}
		}

		GIVEN("a subrange of [0, 4)")
		{
			auto subrange = atma::xfer_dest(dest_memory, 4);

			WHEN("memory_construct is called with arguments for a direct constructor")
			{
				value_type_info::curry_direct_construct_args(atma::memory_construct,
					subrange);

				THEN("elements [0, 4) equate to compar, and elements [4, 6) compare against defval")
				{
					CHECK_MEMORY(dest_memory,
						compar, compar, compar, compar,
						defval, defval);
				}
			}
		}

		GIVEN("a subrange of [1, 5)")
		{
			auto subrange = atma::xfer_dest(dest_memory + 1, 4);

			WHEN("memory_construct is called with arguments for a direct constructor")
			{
				value_type_info::curry_direct_construct_args(atma::memory_construct, subrange);

				THEN("element [0] equates to defval")
				THEN("elements [1, 5) equate to compar")
				THEN("element [5] equates to defval")
				{
					CHECK_MEMORY(dest_memory,
						defval,
						compar, compar, compar, compar,
						defval);
				}
			}

			WHEN("memory_construct is called with arguments for the copy-constructor")
			{
				atma::memory_construct(subrange, compar);

				THEN("element [0] equates to defval")
				THEN("elements [1, 5) equate to compar")
				THEN("element [5] equates to defval")
				{
					CHECK_MEMORY(dest_memory,
						defval,
						compar, compar, compar, compar,
						defval);
				}
			}
		}
	}
}

SCENARIO_TEMPLATE("memory_copy_construct is called", xfer, allocator_value_tuples)
{
	using allocator_type = std::tuple_element_t<0, xfer>;
	using value_type     = std::tuple_element_t<1, xfer>;
	using storage_type   = atma::vector<value_type, allocator_type>;

	using info = xfer_type_info_t<value_type>;

	GIVEN("'defval', a value-initialized instance of our value_type")
	GIVEN("a destination vector of six (6) value-initialized elements that equate to defval")
	GIVEN("four (4) known values to compare against")
	{
		auto const defval = value_type();

		auto const& compar0 = info::compar0;
		auto const& compar1 = info::compar1;
		auto const& compar2 = info::compar2;
		auto const& compar3 = info::compar3;

		auto dest_storage = std::vector<value_type>(6, defval);
		auto dest_memory = atma::basic_memory_t<value_type, allocator_type>(dest_storage.data());

		GIVEN("a source array of those four values")
		{
			value_type source[4] = {info::compar0, info::compar1, info::compar2, info::compar3};
			
			WHEN("memory_copy_construct is called with dest being a subrange of the first four (4) elements")
			{
				atma::memory_copy_construct(
					atma::xfer_dest(dest_storage, 4),
					atma::xfer_src(source));

				THEN("the first four elements equate to our known values")
				{
					CHECK_MEMORY(dest_memory,
						compar0, compar1, compar2, compar3,
						defval, defval);
				}
			}
		}

		GIVEN("a source vector of those four values")
		{
			auto const source = std::vector<value_type>{info::compar0, info::compar1, info::compar2, info::compar3};

			WHEN("memory_copy_construct is called with destination subrange [1, 5)")
			{
				atma::memory_copy_construct(
					atma::xfer_dest(dest_memory + 1, 4),
					atma::xfer_src(source));

				THEN("the middle four elements [1, 5) equate to our known values")
				{
					CHECK_MEMORY(dest_memory,
						defval,
						compar0, compar1, compar2, compar3,
						defval);
				}
			}

			THEN("memory_copy_construct can copy-construct bits of both ranges")
			{
				atma::memory_copy_construct(
					atma::xfer_dest(dest_memory + 4, 2),
					atma::xfer_src(source, 2, 2));

				CHECK_MEMORY(dest_memory,
					defval, defval, defval, defval,
					compar2, compar3);
			}

			THEN("memory_copy_construct can copy-construct from iterateors")
			{
				atma::memory_copy_construct(
					atma::xfer_dest(dest_memory + 2, 4),
					source.begin(), source.end());

				CHECK_MEMORY(dest_memory,
					defval, defval,
					compar0, compar1, compar2, compar3);
			}

			THEN("")
			{
				auto dest2_storage = std::vector<value_type>(4, defval);

				//static_assert(atma::concepts::models<atma::assignable_concept, typename decltype(dest2_storage)::value_type>::value);
				//static_assert(atma::concepts::models_v<atma::dest_memory_range_concept, decltype(dest2_storage)>);

				atma::memory_copy_construct(dest2_storage, source);
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

