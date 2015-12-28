#define CATCH_CONFIG_MAIN
#include <atma/unit_test.hpp>

#include <atma/vector.hpp>

#define BOOST_PP_VARIADICS 1
#include <boost/preprocessor.hpp>

#define CANARY_STDOUT 0

struct canary_t
{
	static int const default_constructor = 0;
	static int const direct_constructor = 1;
	static int const copy_constructor = 2;
	static int const move_constructor = 3;
	static int const destructor = 4;

	struct event_t
	{
		event_t(int instruction)
			: id(-1), instruction(instruction), payload(-1)
		{}

		event_t(int id, int instruction)
			: id(id), instruction(instruction), payload(-1)
		{}

		event_t(int id, int instruction, int payload)
			: id(id), instruction(instruction), payload(payload)
		{}

		int id;
		int instruction;
		int payload;
	};

	struct scope_switcher_t
	{
		explicit scope_switcher_t(std::string const& name)
		{
			canary_t::switch_scope(name);
		}

		~scope_switcher_t()
		{
			canary_t::switch_scope_nil();
		}

		operator bool() const { return true; }
	};

	using event_log_t = std::vector<event_t>;
	using event_log_map_t = std::map<std::string, std::pair<int, event_log_t>>;


	canary_t()
		: scope(current_scope())
		, id(generate_id())
		, payload()
	{
#if CANARY_STDOUT
		std::cout << "[" << scope->first << ':' << id << "] canary_t::default-constructor(" << payload << ')' << std::endl;
#endif
		scope->second.second.emplace_back(id, default_constructor, payload);
	}

	canary_t(int payload)
		: scope(current_scope())
		, id(generate_id())
		, payload(payload)
	{
#if CANARY_STDOUT
		std::cout << "[" << scope->first << ':' << id << "] canary_t::direct-constructor(" << payload << ')' << std::endl;
#endif
		scope->second.second.emplace_back(id, direct_constructor, payload);
	}

	canary_t(canary_t const& rhs)
		: scope(current_scope())
		, id(generate_id())
		, payload(rhs.payload)
	{
#if CANARY_STDOUT
		std::cout << "[" << scope->first << ':' << id << "] canary_t::copy-constructor(" << payload << ')' << std::endl;
#endif
		scope->second.second.emplace_back(id, copy_constructor, payload);
	}

	canary_t(canary_t&& rhs)
		: scope(current_scope())
		, id(generate_id())
		, payload(rhs.payload)
	{
		rhs.payload = 0;
#if CANARY_STDOUT
		std::cout << "[" << scope->first << ':' << id << "] canary_t::move-constructor(" << payload << ')' << std::endl;
#endif
		scope->second.second.emplace_back(id, move_constructor, payload);
	}

	~canary_t()
	{
#if CANARY_STDOUT
		std::cout << "[" << scope->first << ':' << id << "] canary_t::destructor(" << payload << ')' << std::endl;
#endif
		scope->second.second.emplace_back(id, destructor, payload);
	}

	

	static auto event_log_matches(std::string const& name, std::vector<event_t> r) -> bool
	{
		auto const& event_log = event_log_handle(name)->second.second;

		if (r.size() != event_log.size())
			return false;

		auto e = event_log.begin();
		for (auto i = r.begin(); i != r.end(); ++i, ++e)
		{
			if (e->id != -1 && i->id != -1 && e->id != i->id)
				return false;

			if (e->instruction != -1 && i->instruction != -1 && e->instruction != i->instruction)
				return false;

			if (e->payload != -1 && i->payload != -1 && e->payload != i->payload)
				return false;
		}

		return true;
	}

	int payload;

private:
	event_log_map_t::iterator scope;
	int id;

private:
	static auto event_log_map() -> event_log_map_t&
	{
		thread_local static event_log_map_t logs;
		return logs;
	}

	static auto event_log_handle(std::string const& name) -> event_log_map_t::iterator
	{
		auto& map = event_log_map();

		auto it = map.find(name);
		if (it == map.end())
			it = map.insert({name, std::make_pair(0, event_log_t{})}).first;

		return it;
	}

	static event_log_map_t::iterator& current_scope()
	{
		static event_log_map_t::iterator _;
		return _;
	}

	static auto switch_scope(std::string const& name) -> void
	{
		current_scope() = event_log_handle(name);
	}

	static auto switch_scope_nil() -> void
	{
		current_scope() = event_log_map().end();
	}

	static int generate_id()
	{
		return ++current_scope()->second.first;
	}
};

auto operator == (canary_t const& lhs, int rhs) -> bool
{
	return lhs.payload == rhs;
}

auto operator == (canary_t const& lhs, canary_t const& rhs) -> bool
{
	return lhs.payload == rhs.payload;
}

#define CHECK_CANARY_SCOPE(name, ...) \
	CHECK(canary_t::event_log_matches(name, {__VA_ARGS__}))

#define CANARY_CC_ORDER(...) \
	BOOST_PP_TUPLE_ENUM((__VA_ARGS__))

#define CANARY_SWITCH_SCOPE(name) \
	if (auto S = canary_t::scope_switcher_t(name))
	//canary_t::switch_scope(name);

SCENARIO("vectors can be constructed", "[vector]")
{
	GIVEN("a default-constructed vector")
	{
		atma::vector<int> v;

		CHECK(v.empty());
		CHECK(v.size() == 0);
		CHECK(v.capacity() == 0);
	}
	
	GIVEN("a vector constructed with size 4 default items")
	{
		CANARY_SWITCH_SCOPE("default-constructed")
		{
			atma::vector<canary_t> v(4);

			CHECK(!v.empty());
			CHECK(v.size() == 4);
			CHECK(v.capacity() >= 4);
		}

		CHECK_CANARY_SCOPE("default-constructed",

			{1, canary_t::default_constructor},
			{2, canary_t::default_constructor},
			{3, canary_t::default_constructor},
			{4, canary_t::default_constructor},
			{1, canary_t::destructor},
			{2, canary_t::destructor},
			{3, canary_t::destructor},
			{4, canary_t::destructor},
		);
	}

	GIVEN("a vector constructed with size 4 copy-constructed items")
	{	
		CANARY_SWITCH_SCOPE("copy-constructed")
		{
			atma::vector<canary_t> v(4, canary_t{13});

			CHECK(!v.empty());
			CHECK(v.size() == 4);
			CHECK(v.capacity() >= 4);
			CHECK(v[0].payload == 13); CHECK(v[1].payload == 13); CHECK(v[2].payload == 13); CHECK(v[3].payload == 13);
		}

		CHECK_CANARY_SCOPE("copy-constructed",

			{1, canary_t::direct_constructor},

			{2, canary_t::copy_constructor},
			{3, canary_t::copy_constructor},
			{4, canary_t::copy_constructor},
			{5, canary_t::copy_constructor},

			{1, canary_t::destructor},

			{2, canary_t::destructor},
			{3, canary_t::destructor},
			{4, canary_t::destructor},
			{5, canary_t::destructor},
		);
	}

	GIVEN("a vector constructed with {1, 2, 3, 4}")
	{
		CANARY_SWITCH_SCOPE("initializer-list")
		{
			atma::vector<canary_t> v{1, 2, 3, 4};

			CHECK(!v.empty());
			CHECK(v.size() == 4);
			CHECK(v.capacity() >= 4);
			CHECK(v[0] == 1); CHECK(v[1] == 2); CHECK(v[2] == 3); CHECK(v[3] == 4);
		}

		CHECK_CANARY_SCOPE("initializer-list",

			CANARY_CC_ORDER(
				{1, canary_t::direct_constructor},
				{2, canary_t::direct_constructor},
				{3, canary_t::direct_constructor},
				{4, canary_t::direct_constructor}),

			{5, canary_t::copy_constructor},
			{6, canary_t::copy_constructor},
			{7, canary_t::copy_constructor},
			{8, canary_t::copy_constructor},

			CANARY_CC_ORDER(
				{4, canary_t::destructor},
				{3, canary_t::destructor},
				{2, canary_t::destructor},
				{1, canary_t::destructor}),

			{5, canary_t::destructor},
			{6, canary_t::destructor},
			{7, canary_t::destructor},
			{8, canary_t::destructor},
		);
	}

	GIVEN("a copy-constructed vector")
	{
		CANARY_SWITCH_SCOPE("copy-construct-vector")
		{
			atma::vector<canary_t> g{1, 2, 3, 4};
			atma::vector<canary_t> v{g};

			CHECK(!v.empty());
			CHECK(v.size() == 4);
			CHECK(v.capacity() >= 4);
			CHECK(v[0].payload == 1); CHECK(v[1].payload == 2); CHECK(v[2].payload == 3); CHECK(v[3].payload == 4);
			CHECK(v == g);
		}

		CHECK_CANARY_SCOPE("copy-construct-vector",

			// construction of temporaries
			CANARY_CC_ORDER(
				{1, canary_t::direct_constructor},
				{2, canary_t::direct_constructor},
				{3, canary_t::direct_constructor},
				{4, canary_t::direct_constructor}),

			// copy-construct into g
			{5, canary_t::copy_constructor},
			{6, canary_t::copy_constructor},
			{7, canary_t::copy_constructor},
			{8, canary_t::copy_constructor},

			// destruct temporaries
			CANARY_CC_ORDER(
				{4, canary_t::destructor},
				{3, canary_t::destructor},
				{2, canary_t::destructor},
				{1, canary_t::destructor}),

			// copy-construct into v
			{9, canary_t::copy_constructor},
			{10, canary_t::copy_constructor},
			{11, canary_t::copy_constructor},
			{12, canary_t::copy_constructor},

			// v destructs
			{9, canary_t::destructor},
			{10, canary_t::destructor},
			{11, canary_t::destructor},
			{12, canary_t::destructor},

			// g destructs
			{5, canary_t::destructor},
			{6, canary_t::destructor},
			{7, canary_t::destructor},
			{8, canary_t::destructor},
		);
	}

	GIVEN("a move-constructed vector")
	{
		atma::vector<int> g{1, 2, 3, 4};
		atma::vector<int> v{std::move(g)};

		THEN("origin vector is empty")
		{
			CHECK(g.empty());
			CHECK(g.size() == 0);
			CHECK(g.capacity() == 0);
		}

		AND_THEN("constructed vector equal")
		{
			CHECK(!v.empty());
			CHECK(v.size() == 4);
			CHECK(v.capacity() >= 4);
			CHECK(v[0] == 1); CHECK(v[1] == 2); CHECK(v[2] == 3); CHECK(v[3] == 4);
		}
	}
}

SCENARIO("vectors can be sized and resized", "[vector]")
{
	GIVEN("an empty vector")
	{
		atma::vector<int> v;

		CHECK(v.empty());
		CHECK(v.size() == 0);
		CHECK(v.capacity() >= 0);

		WHEN("it is resized")
		{
			v.resize(10);

			THEN("the size and capacity change")
			{
				CHECK(v.size() == 10);
				CHECK(v.capacity() >= 10);
			}
		}

		WHEN("more capacity is reserved")
		{
			v.reserve(10);

			THEN("the capacity changes but not the size")
			{
				CHECK(v.empty());
				CHECK(v.size() == 0);
				CHECK(v.capacity() >= 10);
			}
		}

		WHEN("more capacity is reserved, then shrink_to_fit")
		{
			v.reserve(10);
			v.shrink_to_fit();

			THEN("the capacity is zero")
			{
				CHECK(v.empty());
				CHECK(v.size() == 0);
				CHECK(v.capacity() == 0);
			}
		}
	}
}

SCENARIO("vectors can be assigned", "[vector]")
{
	GIVEN("an empty vector and vector of four components")
	{
		atma::vector<int> v;
		atma::vector<int> v2{1, 2, 3, 4};

		WHEN("v is assigned v2")
		{
			v = v2;

			THEN("a copy is made")
			{
				CHECK(!v.empty());
				CHECK(v.size() == 4);
				CHECK(v == v2);
			}
		}

		WHEN("v is move-assigned v2")
		{
			v = std::move(v2);

			THEN("v2 is reduced to nothing")
			{
				CHECK(!v.empty());
				CHECK(v.size() == 4);
				CHECK(v2.empty());
				CHECK(v2.capacity() == 0);

				atma::vector<int> t{1, 2, 3, 4};
				CHECK(v == t);
			}
		}
	}
}
