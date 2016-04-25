#include <rose/console.hpp>

#include <atma/config/platform.hpp>
#include <atma/assert.hpp>


using namespace rose;
using rose::console_t;


console_t::console_t()
{
	auto hwnd = GetConsoleWindow();
	if (hwnd == 0) {
		bool b = AllocConsole() == TRUE;
		ATMA_ASSERT(b, "couldn't allocate console");
		if (b) {
			hwnd = GetConsoleWindow();
			ATMA_ASSERT(hwnd != 0, "allocated console but couldn't get handle");
		}
	}

	console_handle_ = (uintptr)GetStdHandle(STD_OUTPUT_HANDLE);
}

auto console_t::write(char const* str, size_t size) -> size_t
{
	DWORD numwrit;
	WriteConsole((HANDLE)console_handle_, str, (DWORD)size, &numwrit, nullptr);
	return numwrit;
}





