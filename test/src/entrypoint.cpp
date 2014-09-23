#include <atma/unittest/run.hpp>
#include <atma/utf/utf8_char.hpp>
#include <atma/xtm/function.hpp>

#include <functional>
#include <stack>
#include <list>

#if 0
namespace jester
{
	typedef std::function<void()> action_t;

	namespace detail
	{
		struct test_t
		{
			char const* name;
			action_t before_each_callback;

			typedef std::list<test_t> children_t;
			children_t children;
		};

		typedef test_t::children_t tests_t;

		auto root_tests() -> tests_t&
		{
			static tests_t _;
			return _;
		}

		typedef std::stack<tests_t*> test_stack_t;

		auto test_stack() -> test_stack_t&
		{
			static bool initialized = false;
			static test_stack_t _;

			if (!initialized)
			{
				_.push(&root_tests());
			}

			return _;
		}

		auto current_position() -> tests_t&
		{
			return *test_stack().top();
		}

		auto dive(char const* name) -> void
		{
			auto& children = current_position();
			auto i = std::find_if(children.begin(), children.end(), [name](test_t const& x) {
				return !strcmp(x.name, name);
			});

			ATMA_ASSERT(i != children.end());
			test_stack().push(&i->children);
		}
	}


	auto test(char const* name, std::function<void()> const& fn) -> void 
	{
		detail::current_position().insert(detail::test_t{name, fn});
		//detail::current_position() = 
		detail::static_suites().insert(std::make_pair(std::string(name), detail::suite_t{name, fn}));
	};
}
#endif


template <template <typename, typename...> class C, typename I, typename FN, typename T, typename... Args>
auto fold(C<T, Args...> const& xs, I const& initial, FN const& fn) -> decltype(fn(I(), T()))
{
	if (xs.empty())
		return initial;

	auto init = initial;
	for (auto const& x : xs)
		init = fn(init, x);

	return init;
}

template <typename C, typename FN>
auto filter(C const& xs, FN const& fn) -> void {
	return lazy_filter_range_t(xs, fn);
}


int main()
{
}
