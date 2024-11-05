#include <atma/unit_test.hpp>
#include <atma/string.hpp>
#include <atma/functor.hpp>
#include <atma/preprocessor.hpp>

#include <numeric>

import atma.memory;




#define CHECK_MEMORY_II_m(r, v, i, elem) \
	CHECK_EQ(((typename decltype(v)::pointer)v.data())[i], elem);

#define CHECK_MEMORY_II(v, seq) \
	BOOST_PP_SEQ_FOR_EACH_I(CHECK_MEMORY_II_m, v, seq)

#define CHECK_MEMORY(v, ...) \
	CHECK_MEMORY_II(v, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define SCOPED_BASIC_MEMORY_ALLOCATION(memory_name, size) memory_name.self_allocate(size); SCOPE_GUARD([&memory_name]{memory_name.self_deallocate(size);})







//
// VALUE TYPES WE WILL TEST AGAINST
// ================================
//
// for every type we want to incorporate into our testing matrix,
// we will need to define `value_test_traits<>` for it
// 

template <typename ValueType>
struct value_test_traits;



//
// int, our dear friend
// -----------------------
// we test the simplest type, an integral
//
template <>
struct value_test_traits<int>
{
	static constexpr auto construction_tuples()
	{
		return std::make_tuple
		(
			std::make_tuple(1),
			std::make_tuple(2),
			std::make_tuple(3),
			std::make_tuple(4)
		);
	}

	static constexpr auto construction_tuples2()
	{
		return std::make_tuple
		(
			std::make_tuple(5),
			std::make_tuple(6),
			std::make_tuple(7),
			std::make_tuple(8)
		);
	}

	static constexpr auto direct_construction_arguments()
	{
		return std::make_tuple(77);
	}
};

//
// dragon_t
// -----------
// a "heavy" type that allocates dynamic memory
//
struct dragon_t
{
	constexpr dragon_t() = default;
	dragon_t(dragon_t const& rhs) = default;
	dragon_t(dragon_t&& rhs)
		: name(std::move(rhs.name))
		, age(rhs.age)
	{
		rhs.age = 0;
	}

	constexpr ~dragon_t()
	{
		name = "";
		age = 0;
	}

	constexpr dragon_t(char const* name, int age)
		: name(name), age(age)
	{}

	bool operator == (dragon_t const& rhs) const = default;

	std::string name;
	int age = 0;
};



template <>
struct value_test_traits<dragon_t>
{
	static constexpr auto construction_tuples()
	{
		return std::make_tuple
		(
			std::make_tuple("oliver", 33),
			std::make_tuple("henry", 21),
			std::make_tuple("marcie", 27),
			std::make_tuple("rachael", 19)
		);
	}

	static constexpr auto construction_tuples2()
	{
		return std::make_tuple
		(
			std::make_tuple("john", 43),
			std::make_tuple("paul", 51),
			std::make_tuple("george", 64),
			std::make_tuple("ringo", 77)
		);
	}

	static constexpr auto direct_construction_arguments()
	{
		return std::make_tuple("elizabeth", 31);
	}
};





//
//  VALUES
//
//
//


template <typename ValueType>
struct preferred_allocator
{
	using type = std::conditional_t<std::is_class_v<ValueType>,
		atma::aligned_allocator_t<ValueType>,
		std::allocator<ValueType>>;
};

template <typename ValueType>
using preferred_allocator_t = typename preferred_allocator<ValueType>::type;


template <typename Value>
struct value_type_splatter
{
	using value_type = Value;
	using preferred_allocator_type = preferred_allocator_t<value_type>;
	using traits = value_test_traits<value_type>;


	// value-initialized element
	inline static value_type const valval = value_type();

	static constexpr auto _make_singular_ = [](auto const& first, auto const&... tuples)
	{
		return std::make_from_tuple<value_type>(first);
	};

	static constexpr auto _make_tuple_ = [](auto const&... tuples)
	{
		return std::make_tuple(std::make_from_tuple<value_type>(tuples)...);
	};

	static constexpr auto _make_vector_ = [](auto const&... tuples)
	{
		return std::vector{ std::make_from_tuple<value_type>(tuples)... };
	};

	// one comparand from each group
	inline static auto compar = std::apply(_make_singular_, traits::construction_tuples());
	inline static auto compar2 = std::apply(_make_singular_, traits::construction_tuples2());

	// comparands as a tuple for structured binding
	inline static auto comparands = std::apply(_make_tuple_, traits::construction_tuples());
	inline static auto comparands2 = std::apply(_make_tuple_, traits::construction_tuples2());

	// comparands as vectors
	inline static auto comparands_vector = std::apply(_make_vector_, traits::construction_tuples());
	inline static auto comparands2_vector = std::apply(_make_vector_, traits::construction_tuples2());

	// the value we will use to compare against for anything that has been
	// constructed directly (usually through execute_with_direct_construct_args)
	inline static auto dirval = std::make_from_tuple<value_type>(traits::direct_construction_arguments());


	inline static auto execute_with_direct_construct_args = [](auto f, auto&&... args)
	{
		return std::apply([&](auto&&... inner_args) {
			std::invoke(f,
				std::forward<decltype(args)>(args)...,
				std::forward<decltype(inner_args)>(inner_args)...);
		}, traits::direct_construction_arguments());
	};
};


// allow us to test xfer_dest/xfer_src in one set of tests
template <typename Tag, template <typename...> typename Allocator, typename Value>
struct xfer_maker
{
	using allocator_type = Allocator<Value>;
	using value_type = Value;

	using memxfer_t = atma::memxfer_t<Tag, Value, Allocator<Value>>;
	using bounded_memxfer_t = atma::bounded_memxfer_t<Tag, Value, std::dynamic_extent, Allocator<Value>>;

	constexpr static auto make = [](auto&&... args)
	{
		if constexpr (std::is_same_v<Tag, atma::dest_memory_tag_t>)
			return atma::xfer_dest(std::forward<decltype(args)>(args)...);
		else
			return atma::xfer_src(std::forward<decltype(args)>(args)...);
	};

	// general comparand
	inline static value_type const compar = std::get<0>(value_type_splatter<value_type>::comparands);

	// execute_with_direct_construct_args takes a function and a list of arguments,
	// and calls that function with the supplied arguments, FOLLOWED by a
	// arguments defined in DEFINE_VALUE_TYPE_FOR_TESTING
	constexpr static auto execute_with_direct_construct_args = value_type_splatter<value_type>::execute_with_direct_construct_args;
};






template <template <typename...> typename Allocator, typename Value>
struct allocator_type_splatter : value_type_splatter<Value>
{
	using allocator_type = Allocator<Value>;
};








//
//  TYPE USAGE INFORMATION
//  -------------------------
//    here we write out, for each value-type we wish to test against,
//    four sets of construction arguments which will be our "known values"
//    for comparisons
//




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
#define ALLOCATOR_VALUE_TUPLES_m(i, d, allocator_type, value_type) BOOST_PP_COMMA_IF(i) allocator_type_splatter<allocator_type, value_type> // std::tuple<allocator_type<value_type>, value_type>
#define ALLOCATOR_VALUE_TUPLES FOR_EACH_COMBINATION(ALLOCATOR_VALUE_TUPLES_m, ~, GENERATE_COMBINATIONS_OF_TUPLES(TEST_XFER_ALLOCATOR_TYPES TEST_XFER_VALUE_TYPES))


#define VALUE_TYPES_LIST_m(r, d, i, value_type) \
	BOOST_PP_COMMA_IF(i) value_type_splatter<value_type>

#define VALUE_TYPES_LIST \
	BOOST_PP_SEQ_FOR_EACH_I(VALUE_TYPES_LIST_m, ~, BOOST_PP_TUPLE_TO_SEQ(TEST_XFER_VALUE_TYPES))




SCENARIO_TEMPLATE("our value-types are well-behaved", info, VALUE_TYPES_LIST)
{
	using value_type = typename info::value_type;

	// when a type is moved _from_, we want it to equal value-initialized
	if constexpr (std::is_class_v<value_type>)
	{
		GIVEN("value_type is a class-type")
		GIVEN("'valval', a value-initialized value-type")
		GIVEN("'test_value, initialized with known values")
		{
			auto const& valval = info::valval;
			auto test_value = info::compar;

			WHEN("we move from 'test_value' to a dummy variable")
			{
				[[maybe_unused]] auto dummy = std::move(test_value);

				THEN("'test_value' equals 'valval'")
				{
					CHECK(test_value == valval);
				}
			}
		}
	}

	// we have made sure that when all class-types destruct they reset
	// their state that is equivalent to being default-initialized
	// (a.k.a. value-initialized)
	if constexpr (std::is_class_v<value_type>)
	{
		GIVEN("value_type is a class-type")
		GIVEN("'valval', a value-initialized value-type")
		GIVEN("'test_value, initialized with known values")
		{
			auto const& valval = info::valval;
			auto test_value = info::compar;

			WHEN("the destructor ~value_type() is called")
			{
				test_value.~value_type();

				THEN("'test_value' equals 'valval'")
				{
					CHECK(test_value == valval);
				}
			}
		}
	}
}



SCENARIO_TEMPLATE("base_memory_t performing EBO", info, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = typename info::allocator_type;
	using value_type = typename info::value_type;

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


SCENARIO_TEMPLATE("basic_memory_t is constructed", info, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = typename info::allocator_type;
	using value_type = typename info::value_type;
	using storage_type = std::vector<value_type, allocator_type>;
	using memory_type = atma::basic_memory_t<value_type, allocator_type>;

	constexpr bool is_empty_allocator = std::is_empty_v<allocator_type>;

	if constexpr (is_empty_allocator)
	{
		GIVEN("the allocator is empty")
		WHEN("basic_memory_t is default-constructed")
		{
			memory_type memory;

			THEN("sizeof(basic_memory_t) is the size of a pointer")
			{
				CHECK(sizeof(memory) == sizeof(value_type*));
			}
		}
	}

	WHEN("basic_memory_t is default-constructed")
	{
		memory_type memory;

		THEN("the memory equates to nullptr")
		{
			CHECK(memory.data() == nullptr);
		}
	}

	auto const& [compar0, compar1, compar2, compar3] = info::comparands;


	GIVEN("memory (a vector) of four elements known to us")
	{
		auto storage = storage_type{compar0, compar1, compar2, compar3};
		
		#define CHECK_MEMORY_AGAINST_COMPARS(memory) \
			CHECK(memory.data() == storage.data()); \
			CHECK_MEMORY(memory, compar0, compar1, compar2, compar3);

		WHEN("basic_memory_t is directly-constructed from a pointer & allocator")
		{
			auto memory = memory_type{storage.data(), storage.get_allocator()};

			if constexpr (is_empty_allocator)
			{
				AND_WHEN("the allocator is empty")
				THEN("sizeof(basic_memory_t) is the size of a pointer")
				{
					CHECK(sizeof(memory) == sizeof(value_type*));
				}
			}

			THEN("the pointer & values match")
			{
				CHECK_MEMORY_AGAINST_COMPARS(memory);
			}
		}

		WHEN("basic_memory_t is constructed with that vector.data()")
		{
			memory_type memory{storage.data()};

			THEN("it evaluates to our known values")
			{	
				CHECK_MEMORY_AGAINST_COMPARS(memory);
			}
		}

		WHEN("basic_memory_t is indexed")
		{
			memory_type memory{storage.data()};

			THEN("it evaluates to our known values")
			{
				CHECK(memory.data()[0] == compar0);
				CHECK(memory.data()[1] == compar1);
				CHECK(memory.data()[2] == compar2);
				CHECK(memory.data()[3] == compar3);
			}
		}

		WHEN("a basic_memory_t is constructed from another with 'x + 2'")
		{
			auto m1 = memory_type{storage.data()};
			auto m2 = m1 + 2;
			static_assert(std::is_same_v<decltype(m2), decltype(m1)>);

			THEN("the pointers of both basic_memory_ts equal each other")
			{
				// test via conversion to pointer
				CHECK(m2.data() == (m1.data() + 2));
				// test equality operator of memory
				CHECK(m2 == (m1 + 2));
			}

			THEN("the values of the instantiated memory are the third and fourth elements")
			{
				CHECK_MEMORY(m2, compar2, compar3);
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

	//atma::get_allocator(4);

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
					CHECK(atma::memory_concept<decltype(d)>);
					CHECK(!atma::bounded_memory_concept<decltype(d)>);
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
				CHECK(atma::memory_concept<decltype(d)>);
				CHECK(!atma::bounded_memory_concept<decltype(d)>);
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
				CHECK(atma::memory_concept<decltype(d)>);
				CHECK(!atma::bounded_memory_concept<decltype(d)>);
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
				CHECK(atma::memory_concept<decltype(d)>);
				CHECK(!atma::bounded_memory_concept<decltype(d)>);
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
				CHECK(atma::memory_concept<decltype(d)>);
				CHECK(atma::bounded_memory_concept<decltype(d)>);
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
				CHECK(atma::memory_concept<decltype(d)>);
				CHECK(atma::bounded_memory_concept<decltype(d)>);
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
			atma::basic_memory_t<typename xfer::value_type, typename xfer::allocator_type> memory;
			SCOPED_BASIC_MEMORY_ALLOCATION(memory, 4);

			[[maybe_unused]] auto d = xfer_make(memory);

			THEN("the result-type is conceptually an 'unbounded memory'")
			{
				CHECK(atma::memory_concept<decltype(d)>);
				CHECK(!atma::bounded_memory_concept<decltype(d)>);
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

			THEN("the result-type is conceptually a 'bounded memory'")
			{
				CHECK(atma::memory_concept<decltype(d)>);
				CHECK(atma::bounded_memory_concept<decltype(d)>);
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

			THEN("the result has the same size")
			{
				CHECK_FALSE(d.empty());
				CHECK(std::size(d) == d.size());
				CHECK(d.size() == test_size);
			}
		}
	}
}


SCENARIO_TEMPLATE("memory_default_construct is called", info, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = typename info::allocator_type;
	using value_type = typename info::value_type;
	using storage_type = atma::vector<value_type, allocator_type>;

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

SCENARIO_TEMPLATE("memory_value_construct is called", info, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = typename info::allocator_type;
	using value_type = typename info::value_type;
	using storage_type = atma::vector<value_type, allocator_type>;

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

SCENARIO_TEMPLATE("memory_direct_construct is called", info, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = typename info::allocator_type;
	using value_type = typename info::value_type;
	using storage_type = atma::vector<value_type, allocator_type>;

	[[maybe_unused]] info _{};

	auto const& valval = info::valval;
	auto const& dirval = info::dirval;
	[[maybe_unused]] auto const& comparands = info::comparands;
	
	GIVEN("'valval', a value-initialized instance of our value_type")
	GIVEN("'dirval', a direct-initialized instance of our value_type, with a value known to us")
	GIVEN("a vector of six (6) elements value-initialized from 'valval'")
	{
		auto storage = std::vector<value_type>(6, valval);

		WHEN("memory_direct_construct is called upon a vector with arguments for a direct constructor")
		{
			info::execute_with_direct_construct_args(
				atma::memory_direct_construct,
				atma::xfer_dest(storage));
			
			THEN("every element in the vector equates to the dirval")
			{
				CHECK_VECTOR(storage,
					dirval, dirval, dirval,
					dirval, dirval, dirval);
			}
		}
	}

	GIVEN("'valval', a value-initialized instance of our value_type")
	GIVEN("'dirval', a direct-initialized instance of our value_type, with a value known to us")
	GIVEN("a basic_memory_t allocated with six (6) value-initialized elements")
	{
		auto memory = atma::basic_memory_t<value_type, allocator_type>();
		SCOPED_BASIC_MEMORY_ALLOCATION(memory, 6);
		atma::memory_value_construct(atma::xfer_dest(memory, 6));

		GIVEN("a subrange of [0, 4)")
		{
			auto subrange = atma::xfer_dest(memory, 4);

			WHEN("memory_direct_construct is called with dirval-esque arguments for a direct constructor")
			{
				info::execute_with_direct_construct_args(
					atma::memory_direct_construct,
					subrange);

				THEN("elements [0, 4) equate to dirval, and elements [4, 6) equate to valval")
				{
					CHECK_MEMORY(memory,
						dirval, dirval, dirval, dirval,
						valval, valval);
				}
			}
		}

		GIVEN("a subrange of [1, 5)")
		{
			auto subrange = atma::xfer_dest(memory + 1, 4);

			WHEN("memory_direct_construct is called with arguments for a direct constructor")
			{
				info::execute_with_direct_construct_args(
					atma::memory_direct_construct,
					subrange);

				THEN("element [0] equates to valval")
				THEN("elements [1, 5) equate to dirval")
				THEN("element [5] equates to valval")
				{
					CHECK_MEMORY(memory,
						valval,
						dirval, dirval, dirval, dirval,
						valval);
				}
			}

			WHEN("memory_direct_construct is called with arguments for the copy-constructor")
			{
				atma::memory_direct_construct(subrange, dirval);

				THEN("element [0] equates to valval")
				THEN("elements [1, 5) equate to dirval")
				THEN("element [5] equates to valval")
				{
					CHECK_MEMORY(memory,
						valval,
						dirval, dirval, dirval, dirval,
						valval);
				}
			}
		}
	}
}

SCENARIO_TEMPLATE("memory_copy_construct is called", info, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = typename info::allocator_type;
	using value_type     = typename info::value_type;
	using storage_type   = atma::vector<value_type, allocator_type>;

	GIVEN("'valval', a value-initialized instance of our value_type")
	GIVEN("a destination vector of six (6) value-initialized elements equivalent to valval")
	GIVEN("four (4) known values to compare against")
	{
		auto const& valval = info::valval;
		auto const& [compar0, compar1, compar2, compar3] = info::comparands;

		auto dest_storage = storage_type(6, valval);
		auto dest_memory = atma::basic_memory_t<value_type, allocator_type>(dest_storage.data());

		GIVEN("a source array of those four values")
		{
			value_type source[4] = {compar0, compar1, compar2, compar3};
			
			WHEN("memory_copy_construct is called with dest being a subrange of the first four (4) elements")
			{
				atma::memory_copy_construct(
					atma::xfer_dest(dest_storage, 4),
					atma::xfer_src(source));

				THEN("the first four elements equate to our known values")
				{
					CHECK_MEMORY(dest_memory,
						compar0, compar1, compar2, compar3,
						valval, valval);
				}
			}
		}

		GIVEN("a source vector of those four known values")
		{
			auto const source = std::vector<value_type>{compar0, compar1, compar2, compar3};

			WHEN("memory_copy_construct is called with destination subrange [1, 5)")
			{
				atma::memory_copy_construct(
					atma::xfer_dest(dest_memory + 1, 4),
					atma::xfer_src(source));

				THEN("the middle four elements [1, 5) equate to our known values")
				{
					CHECK_MEMORY(dest_memory,
						valval,
						compar0, compar1, compar2, compar3,
						valval);
				}
			}

			WHEN("memory_copy_construct is called on the subranges of dest[4, 6) & source[2, 4)")
			{
				atma::memory_copy_construct(
					atma::xfer_dest(dest_memory + 4, 2),
					atma::xfer_src(source, 2, 2));

				THEN("the subrange of dest equals the source elements")
				{
					CHECK_MEMORY(dest_memory,
						valval, valval, valval, valval,
						compar2, compar3);
				}
			}

			WHEN("memory_copy_construct is called with iterators from the source vector")
			{
				atma::memory_copy_construct(
					atma::xfer_dest(dest_memory, 2, 2), //.subrange(2, 4),
					source.begin(), source.end());

				THEN("memory_copy_construct can copy-construct from iterateors")
				{
					CHECK_MEMORY(dest_memory,
						valval, valval,
						compar0, compar1, compar2, compar3);
				}
			}
		}
	}
}

SCENARIO_TEMPLATE("memory_move_construct is called", info, ALLOCATOR_VALUE_TUPLES)
{
	using allocator_type = typename info::allocator_type;
	using value_type     = typename info::value_type;
	using storage_type   = atma::vector<value_type, allocator_type>;

	GIVEN("'valval', a value-initialized instance of our value_type")
	GIVEN("a destination vector of six (6) value-initialized elements that equate to valval")
	GIVEN("four (4) known values to compare against")
	{
		auto const& valval = info::valval;
		auto const& [compar0, compar1, compar2, compar3] = info::comparands;

		auto dest_storage = std::vector<value_type>(6, valval);

		GIVEN("source memory pointing to an lvalue vector")
		{
			auto const& src_storage = info::comparands_vector;

			GIVEN("a destination subrange of [0, 4)")
			GIVEN("a source range of the full source vector")
			{
				WHEN("memory_move_construct is called")
				{
					atma::memory_move_construct(
						atma::xfer_dest(dest_storage, 4),
						atma::xfer_src(src_storage));

					THEN("the destination range has the four elements in positions [0, 4)")
					{
						CHECK_MEMORY(dest_storage,
							compar0, compar1, compar2, compar3,
							valval, valval);
					}

					if constexpr (false && std::is_class_v<value_type>)
					{
						// SERIOUSLY, this is only valid for memory_relocate, and should be put there

						THEN("the source range has now-empty elements")
						{
							CHECK_VECTOR(src_storage,
								valval, valval, valval, valval);
						}
					}
				}
			}

			GIVEN("a destination subrange of [2, 4)")
			GIVEN("a source range of the first two elements")
			{
				WHEN("memory_move_construct is called")
				{
					atma::memory_move_construct(
						atma::xfer_dest(dest_storage).subspan(2, 2),
						atma::xfer_src(src_storage, 0, 2));

					THEN("destination elements [0, 2) equate to valval")
					THEN("destination elements [2, 4) equate to comparands 0 and 1")
					THEN("destination elements [4, 6) equate to valval")
					{
						CHECK_MEMORY(dest_storage,
							valval, valval,
							compar0, compar1,
							valval, valval);
					}

					if constexpr (false && std::is_class_v<value_type>)
					{
						// SERIOUSLY, this is only valid for memory_relocate, and should be put there

						THEN("source elements [0, 2) equate to valval")
						THEN("source elements [2, 4) remain untouched")
						{
							CHECK_VECTOR(src_storage,
								valval, valval,
								compar2, compar3);
						}
					}
				}
			}
		}
	}
}

SCENARIO_TEMPLATE("destruct is called", info, VALUE_TYPES_LIST)
{
	using value_type = typename info::value_type;
	using allocator_type = atma::aligned_allocator_t<value_type>;
	using memory_type = atma::basic_memory_t<value_type, allocator_type>;

	static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

	if constexpr (std::is_class_v<value_type>)
	{
		GIVEN("a vector of four (4) elements known to us")
		GIVEN("'valval', a value-initialized element")
		{
			auto dest_storage = info::comparands_vector;
			auto const& valval = info::valval;

			WHEN("memory_destruct is called for the whole range")
			{
				atma::memory_destruct(
					atma::xfer_dest(dest_storage));

				THEN("destruct calls the destructor of the whole range")
				{
					CHECK_VECTOR(dest_storage,
						valval, valval, valval, valval);
				}
			}
		}
	}
}


#if 0
SCENARIO_TEMPLATE("user calls memory_copy", info, VALUE_TYPES_LIST)
{
	GIVEN("a memory-range of instantiated integers")
	{
		using value_type = typename info::value_type;
		using allocator_type = atma::aligned_allocator_t<value_type>;
		using memory_type = atma::basic_memory_t<value_type, allocator_type>;

		//static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

		auto dest = info::comparands_as_vector();
		auto src  = info::comparands_as_vector();

		GIVEN("source memory pointing to an lvalue vector")
		{
			THEN("memory_copy performs correctly")
			{
				atma::memory_copy(
					atma::xfer_dest(dest, 2),
					atma::xfer_src(src, 2, 2));

				CHECK_VECTOR(dest_storage,
					7, 8, 3, 4);
			}

			THEN("memory_move performs overlapping regions correctly")
			{
				atma::memory_move(
					atma::xfer_dest(dest_memory, 2),
					atma::xfer_src(dest_memory + 1, 2));

				CHECK_VECTOR(dest_storage,
					2, 3, 3, 4);
			}
		}
	}
}
#endif

SCENARIO("memory_copy or memory_move is called")
{
	GIVEN("a memory-range of instantiated integers")
	{
		using value_type = int;
		using allocator_type = atma::aligned_allocator_t<value_type>;
		using memory_t = atma::basic_memory_t<value_type, allocator_type>;

		static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

		auto dest_storage = std::vector<value_type>{ 1, 2, 3, 4 };
		auto dest_memory = memory_t(dest_storage.data());

		//atma::memory::range_construct<int, allocator_type>(dest_memory, 0);

		GIVEN("source memory pointing to an lvalue vector")
		{
			auto src_storage = std::vector<value_type>{ 5, 6, 7, 8 };

			THEN("memory_copy performs correctly")
			{
				atma::memory_copy(
					atma::xfer_dest(dest_memory, 2),
					atma::xfer_src(src_storage, 2, 2));

				CHECK_VECTOR(dest_storage,
					7, 8, 3, 4);
			}

			THEN("memory_move performs overlapping regions correctly")
			{
				atma::memory_move(
					atma::xfer_dest(dest_memory, 2),
					atma::xfer_src(dest_memory + 1, 2));

				CHECK_VECTOR(dest_storage,
					2, 3, 3, 4);
			}
		}
	}
}

SCENARIO("memory_relocate is used")
{
	GIVEN("a memory-range of instantiated dragons")
	{
		using value_type = dragon_t;
		using allocator_type = atma::aligned_allocator_t<dragon_t>;
		using memory_t = atma::basic_memory_t<value_type, allocator_type>;

		static_assert(std::is_empty_v<allocator_type>, "allocator not empty!");

		dragon_t const oliver{ "oliver", 33 };
		dragon_t const henry{ "henry", 24 };
		dragon_t const marcie{ "marcie", 27 };
		dragon_t const rachael{ "rachael", 19 };

		auto dest_storage = std::vector<value_type>{ oliver, henry, marcie, rachael };
		auto dest_memory = memory_t(dest_storage.data());

		THEN("trivial")
		{
			atma::vector<int> numbers{ 1, 2, 3, 4 };
			atma::memory_relocate(
				atma::xfer_dest(numbers, 2),
				atma::xfer_src(numbers, 2, 2));
		}

		THEN("non trivial")
		{
			atma::memory_relocate(
				atma::xfer_dest(dest_memory, 2),
				atma::xfer_src(dest_memory, 2, 2));

			//CHECK_VECTOR(dest_storage,
			//	marcie, rachael, marcie, rachael);
			CHECK(dest_storage[0] == marcie);
			CHECK(dest_storage[1] == rachael);
		}

		THEN("move-constructible")
		{
			struct bop
			{
				bop() = default;
				bop(bop const&) = delete;

				bop& operator = (bop const&) = delete;
				bop& operator = (bop&& rhs)
				{
					std::swap(name, rhs.name);
					return *this;
				}

				bop(std::string const& name)
					: name{name}
				{}

				std::string name;
			};

			bop bops[4] = {{"hello"}, {"how"}, {"are"}, {"you"}};
			//atma::vector<bop> bops; //{"hello", "how", "are", "you"};
			//bops.emplace_back("hello");
			//atma::typed_unique_memory_t<bop> yay{atma::allocate_n, 4};
			auto* yay = reinterpret_cast<bop*>(malloc(sizeof(bop) * 4));

			atma::memory_relocate(
				atma::xfer_dest(yay, 4),
				atma::xfer_src(bops, 4));

		}
	}
}


