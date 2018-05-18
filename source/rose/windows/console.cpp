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

	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	//Clear the error state for each of the C++ standard stream objects. We need to do this, as
	//attempts to access the standard streams before they refer to a valid target will cause the
	//iostream objects to enter an error state. In versions of Visual Studio after 2005, this seems
	//to always occur during startup regardless of whether anything has been read from or written to
	//the console or not.
	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();
}

auto console_t::set_color(uint32 c) -> void
{
	SetConsoleTextAttribute((HANDLE)console_handle_, (WORD)c);
}

auto console_t::write(char const* str, size_t size) -> size_t
{
	wchar_t buf[4 * 1024];
	int r = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, (int)size, buf, 4 * 1024);
	if (r == 0)
	{
		auto e = GetLastError();
		switch (e)
		{
			case ERROR_INSUFFICIENT_BUFFER:
				break;
			case ERROR_INVALID_FLAGS:
				break;
			case ERROR_INVALID_PARAMETER:
				break;
			case ERROR_NO_UNICODE_TRANSLATION:
				break;
		}
	}

	DWORD numwrit;
	WriteConsole((HANDLE)console_handle_, buf, (DWORD)r, &numwrit, nullptr);
	return numwrit;
}



//
//
//
auto rose::default_console_log_handler_t::handle(atma::log_level_t level, atma::unique_memory_t const& data) -> void
{
	auto handle_spacing = [&](atma::log_style_t LS)
	{
		if (last_log_style_ == atma::log_style_t::pretty_print || LS == atma::log_style_t::pretty_print)
			console_.write("\n", 1);
		last_log_style_ = LS;
	};

	atma::decode_logging_data(data,
		handle_spacing,
		atma::curry(&rose::console_t::set_color, &console_),
		atma::curry(&rose::console_t::write, &console_));
}


