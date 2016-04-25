#include <rose/console.hpp>

#include <atma/config/platform.hpp>


using namespace rose;
using rose::console_t;


console_t::console_t()
{
	auto hwnd = GetConsoleWindow();
	if (hwnd == 0) {
		bool b = AllocConsole();
		if (b) {
			hwnd = GetConsoleWindow();
			ATMA_ASSERT(hwnd != 0, "allocated console but couldn't get handle");
		}
	}
}

console_t::~console_t()
{
}





