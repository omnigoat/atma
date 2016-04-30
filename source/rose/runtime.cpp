#include <rose/runtime.hpp>

#include <rose/console.hpp>


using namespace rose;
using rose::runtime_t;


runtime_t::~runtime_t()
{
}

auto runtime_t::initialize_console() -> void
{
}

auto runtime_t::get_console() -> console_t&
{
	if (!console_)
	{
		console_.reset(new console_t);
	}

	return *console_;
}

