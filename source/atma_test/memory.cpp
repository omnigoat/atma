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

#define SCOPED_BASIC_MEMORY_ALLOCATION(memory_name, size) memory_name.allocate(size); SCOPE_GUARD([&memory_name]{memory_name.deallocate(size);})




//
// xfer_type_info_t
// --------------------
//
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




//
//  TYPE USAGE INFORMATION
//  -------------------------
//    here we write out, for each value-type we wish to test against,
//    four sets of construction arguments which will be our "known values"
//    for comparisons
//
DEFINE_VALUE_TYPE_FOR_TESTING(dragon_t, \
	("oliver", 33) \
	("henry", 21) \
	("marcie", 27) \
	("rachael", 19));

DEFINE_VALUE_TYPE_FOR_TESTING(int, (1)(2)(3)(4));




//
// TEST TYPES SETUP
// -------------------------
// the tuple-seqs of types that we will make combinations out of for coverage
//
#define TEST_XFER_MEMORY_TAG_TYPES (atma::dest_memory_tag_t, atma::src_memory_tag_t)
#define TEST_XFER_ALLOCATOR_TYPES  (std::allocator, atma::arena_allocator_t)
#define TEST_XFER_VALUE_TYPES (int, dragon_t)

#define MEMORYTAG_ALLOCATOR_VALUETYPE_SEQS \
	TEST_XFER_MEMORY_TAG_TYPES \
	TEST_XFER_ALLOCATOR_TYPES \
	TEST_XFER_VALUE_TYPES

// type-to-string all our allocators
#define TYPE_ALLOCATORS_TO_STRING(i, d, allocator_type, value_type) TYPE_TO_STRING(allocator_type<value_type>);
FOR_EACH_COMBINATION(TYPE_ALLOCATORS_TO_STRING, ~, GENERATE_COMBINATIONS_OF_TUPLES(TEST_XFER_ALLOCATOR_TYPES TEST_XFER_VALUE_TYPES))


//
// XFER_TYPE_COMBINATIONS
// -------------------------
//   a list of 'xfer_maker<tag_type, allocator_type, value_type>' for every
//   combination within MEMORYTAG_ALLOCATOR_VALUETYPE_SEQS
//
#define XFER_TYPE_COMBINATIONS_m(i, ...) BOOST_PP_COMMA_IF(i) __VA_ARGS__
#define XFER_TYPE_COMBINATIONS FOR_EACH_TEMPLATE_TYPE_COMBINATION(XFER_TYPE_COMBINATIONS_m, \
	GENERATE_TEMPLATE_TYPE_COMBINATIONS(xfer_maker, MEMORYTAG_ALLOCATOR_VALUETYPE_SEQS))

//
// ALLOCATOR_VALUE_TUPLES
// -------------------------
//   a list of 'std::tuple<allocator_type, value_type>' for every combination in
//   our test coverage lists
//
#define ALLOCATOR_VALUE_TUPLES_m(i, d, allocator_type, value_type) BOOST_PP_COMMA_IF(i) std::tuple<allocator_type<value_type>, value_type>
#define ALLOCATOR_VALUE_TUPLES FOR_EACH_COMBINATION(ALLOCATOR_VALUE_TUPLES_m, ~, GENERATE_COMBINATIONS_OF_TUPLES(TEST_XFER_ALLOCATOR_TYPES TEST_XFER_VALUE_TYPES))














SCENARIO_TEMPLATE("base_memory_t performing EBO", xfer, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = std::tuple_element_t<0, xfer>;
	using value_type     = std::tuple_element_t<1, xfer>;

	if constexpr (std::is_empty_v<allocator_type>)
	{
		GIVEN("an empty allocator")
		{
			THEN("sizeof base_memory_t is 1")
			{
				atma::detail::base_memory_t<value_type, allocator_type> memory;
				CHECK(sizeof(memory) == 1);
			}
		}
	}
	else
	{
		GIVEN("a non-empty allocator")
		{
			THEN("sizeof base_memory_t is the size of the allocator")
			{
				atma::detail::base_memory_t<value_type, allocator_type> memory;
				CHECK(sizeof(memory) == sizeof(allocator_type));
			}
		}
	}
}

SCENARIO_TEMPLATE("basic_memory_t can be constructed", xfer, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = std::tuple_element_t<0, xfer>;
	using value_type     = std::tuple_element_t<1, xfer>;

	using xferti = xfer_type_info_t<value_type>;


	using memory_t = atma::basic_memory_t<value_type, allocator_type>;
	constexpr bool is_empty_allocator = std::is_empty_v<allocator_type>;

	using storage_t = std::vector<value_type, allocator_type>;

	GIVEN_IF_CONSTEXPR(is_empty_allocator, "the allocator is empty")
	{
		WHEN("basic_memory_t is default-constructed")
		{
			memory_t memory;

			THEN("sizeof basic_memory_t is the size of a pointer")
			{
				CHECK(sizeof(memory) == sizeof(value_type*));
			}
		}
	}

	WHEN("basic_memory_t is default-constructed")
	{
		memory_t memory;

		THEN("the memory equates to nullptr")
		{
			CHECK((value_type*)memory == nullptr);
		}
	}

	GIVEN("a vector of four elements known to us")
	{
		auto storage = storage_t{xferti::compar0, xferti::compar1, xferti::compar2, xferti::compar3};
		
		#define CHECK_MEMORY_AGAINST_COMPARS(memory) \
			CHECK((value_type*)memory == storage.data()); \
			CHECK_MEMORY(memory, xferti::compar0, xferti::compar1, xferti::compar2, xferti::compar3);

		WHEN("basic_memory_t is directly-constructed from a pointer & allocator")
		{
			auto memory = memory_t{storage.data(), storage.get_allocator()};

			AND_WHEN_IF_CONSTEXPR(is_empty_allocator, "the allocator is empty")
			THEN("sizeof basic_memory_t equtes to the size of a pointer")
			{
				CHECK(sizeof(memory) == sizeof(value_type*));
			}

			THEN("the pointer & values match")
			{
				CHECK_MEMORY_AGAINST_COMPARS(memory);
			}
		}

		WHEN("basic_memory_t is default-constructed, and then assigned a pointer")
		{
			memory_t memory;
			memory = storage.data();

			THEN("it evaluates to our known values")
			{	
				CHECK_MEMORY_AGAINST_COMPARS(memory);
			}
		}

		WHEN("basic_memory_t is indexed")
		{
			memory_t memory{storage.data()};

			THEN("it evaluates to our known values")
			{
				CHECK(memory[0] == xferti::compar0);
				CHECK(memory[1] == xferti::compar1);
				CHECK(memory[2] == xferti::compar2);
				CHECK(memory[3] == xferti::compar3);
			}
		}

		WHEN("a basic_memory_t is constructed from another with 'x + 2'")
		{
			auto m1 = memory_t{storage.data()};
			auto m2 = m1 + 2;
			static_assert(std::is_same_v<decltype(m2), decltype(m1)>);

			THEN("the pointers of both basic_memory_ts equal each other")
			{
				// test via conversion to pointer
				CHECK((value_type*)m2 == ((value_type*)m1 + 2));
				// test equality operator of memory
				CHECK(m2 == (m1 + 2));
			}

			THEN("the values of the instantiated memory are the third and fourth elements")
			{
				CHECK_MEMORY(m2, xferti::compar2, xferti::compar3);
			}
		}
	}
}



SCENARIO_TEMPLATE("a (dest|src)_memxfer_t is directly constructed", xfer, XFER_TYPE_COMBINATIONS)
{
	using value_type     = typename xfer::value_type;
	using allocator_type = typename xfer::allocator_type;
	using memxfer_type   = typename xfer::memxfer_t;
	using storage_type   = atma::vector<value_type, allocator_type>;

	GIVEN("storage of four value-initialized elements")
	{
		auto storage = storage_type(4);

		if constexpr (std::is_empty_v<allocator_type>)
		{
			WHEN("it is constructible from solely a pointer")
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


SCENARIO_TEMPLATE("xfer_dest() or xfer_src() is called", xfer, XFER_TYPE_COMBINATIONS)
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


SCENARIO_TEMPLATE("memory_default_construct is called", xfer, ALLOCATOR_VALUE_TUPLES)
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
			[[maybe_unused]] value_type const defval;

			auto storage = std::unique_ptr<value_type[]>(new value_type[6]);
			auto memory = memory_t(storage.get());

			WHEN("memory_default_construct is called upon whole range")
			{
				atma::memory_default_construct(atma::xfer_dest(memory, 6));

				THEN("every element equates to defval")
				{
					CHECK_MEMORY(memory,
						defval, defval, defval,
						defval, defval, defval);
				}
			}
		}
	}
}

SCENARIO_TEMPLATE("memory_value_construct is called", xfer, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = std::tuple_element_t<0, xfer>;
	using value_type     = std::tuple_element_t<1, xfer>;
	using storage_type   = atma::vector<value_type, allocator_type>;

	using memory_t = atma::basic_memory_t<value_type, allocator_type>;

	GIVEN("'valval', a value-initialized value-type")
	GIVEN("memory pointing to uninitialized-memory of six (6) elements")
	{
		[[maybe_unused]] auto const valval = value_type();

		auto storage = std::unique_ptr<value_type[]>(new value_type[6]);
		auto memory = memory_t(storage.get());

		WHEN("memory_value_construct is called upon the whole range")
		{
			atma::memory_value_construct(atma::xfer_dest(memory, 6));

			THEN("the whole range is value-constructed")
			{
				CHECK_MEMORY(memory,
					valval, valval, valval,
					valval, valval, valval);
			}
		}
	}
}

SCENARIO_TEMPLATE("memory_construct is called", xfer, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = std::tuple_element_t<0, xfer>;
	using value_type     = std::tuple_element_t<1, xfer>;
	using storage_type   = atma::vector<value_type, allocator_type>;

	using value_type_info = xfer_type_info_t<value_type>;

	auto const valval = value_type();
	auto const compar = value_type_info::compar0;

	GIVEN("'valval', a value-initialized instance of our value_type")
	GIVEN("'compar', a direct-initialized instance of our value_type, with a value known to us")
	GIVEN("a vector of six (6) value-initialized elements")
	{
		auto storage = std::vector<value_type>(6, valval);

		WHEN("memory_construct is called upon a vector with arguments for a direct constructor")
		{
			value_type_info::curry_direct_construct_args(atma::memory_construct, storage);

			THEN("every element in the vector equates to the compar")
			{
				CHECK_VECTOR(storage,
					compar, compar, compar,
					compar, compar, compar);
			}
		}
	}

	GIVEN("'valval', a value-initialized instance of our value_type")
	GIVEN("'compar', a direct-initialized instance of our value_type, with a value known to us")
	GIVEN("a basic_memory_t allocated with six (6) value-initialized elements")
	{
		auto memory = atma::basic_memory_t<value_type, allocator_type>();
		SCOPED_BASIC_MEMORY_ALLOCATION(memory, 6);
		atma::memory_value_construct(atma::xfer_dest(memory, 6));

		GIVEN("a subrange of [0, 4)")
		{
			auto subrange = atma::xfer_dest(memory, 4);

			WHEN("memory_construct is called with arguments for a direct constructor")
			{
				value_type_info::curry_direct_construct_args(atma::memory_construct, subrange);

				THEN("elements [0, 4) equate to compar, and elements [4, 6) compare against valval")
				{
					CHECK_MEMORY(memory,
						compar, compar, compar, compar,
						valval, valval);
				}
			}
		}

		GIVEN("a subrange of [1, 5)")
		{
			auto subrange = atma::xfer_dest(memory + 1, 4);

			WHEN("memory_construct is called with arguments for a direct constructor")
			{
				value_type_info::curry_direct_construct_args(atma::memory_construct, subrange);

				THEN("element [0] equates to valval")
				THEN("elements [1, 5) equate to compar")
				THEN("element [5] equates to valval")
				{
					CHECK_MEMORY(memory,
						valval,
						compar, compar, compar, compar,
						valval);
				}
			}

			WHEN("memory_construct is called with arguments for the copy-constructor")
			{
				atma::memory_construct(subrange, compar);

				THEN("element [0] equates to valval")
				THEN("elements [1, 5) equate to compar")
				THEN("element [5] equates to valval")
				{
					CHECK_MEMORY(memory,
						valval,
						compar, compar, compar, compar,
						valval);
				}
			}
		}
	}
}

SCENARIO_TEMPLATE("memory_copy_construct is called", xfer, ALLOCATOR_VALUE_TUPLES)
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

		GIVEN("a source vector of those four known values")
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


SCENARIO_TEMPLATE("memory_move_construct is called", xfer, ALLOCATOR_VALUE_TUPLES)
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

		GIVEN("source memory pointing to an lvalue vector")
		{
			auto src_storage = std::vector<value_type>{compar0, compar1, compar2, compar3};

			GIVEN("a destination subrange of [0, 4)")
			GIVEN("a source range of the full source vector")
			{
				WHEN("memory_move_construct is called")
				{
					atma::memory_move_construct(
						atma::xfer_dest(dest_memory, 4),
						atma::xfer_src(src_storage));

					THEN("the destination range has the four elements in positions [0, 4)")
					{
						CHECK_MEMORY(dest_memory,
							compar0, compar1, compar2, compar3,
							defval, defval);
					}

					THEN_IF_CONSTEXPR(std::is_class_v<value_type>, "the source range has now-empty elements")
					{
						CHECK_VECTOR(src_storage,
							defval, defval, defval, defval);
					}
				}
			}

			GIVEN("a destination subrange of [2, 4)")
			GIVEN("a source range of the first two elements")
			{
				WHEN("memory_move_construct is called")
				{
					atma::memory_move_construct(
						atma::xfer_dest(dest_memory + 2, 2),
						atma::xfer_src(src_storage, 0, 2));

					THEN("destination elements [0, 2) equate to defval")
					THEN("destination elements [2, 4) equate to comparands 0 and 1")
					THEN("destination elements [4, 6) equate to defval")
					{
						CHECK_MEMORY(dest_memory,
							defval, defval,
							compar0, compar1,
							defval, defval);
					}

					if constexpr (std::is_class_v<value_type>)
					{
						THEN("source elements [0, 2) equate to defval")
						THEN("source elements [2, 4) remain untouched")
						{
							CHECK_VECTOR(src_storage,
								defval, defval,
								compar2, compar3);
						}
					}
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

