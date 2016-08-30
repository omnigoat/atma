#pragma once

#include <atma/config/platform.hpp>
#include <atma/logging.hpp>

namespace atma {

	struct combined_color_t {
		combined_color_t(unsigned char color) : color(color) {}
		const unsigned char color;
	};

	struct foreground_color_t : combined_color_t {
		foreground_color_t(unsigned char color) : combined_color_t(color) {}
	};

	struct background_color_t : combined_color_t {
		background_color_t(unsigned char color) : combined_color_t(color) {}
	};

	// combines two colors
	inline combined_color_t operator + (const background_color_t& lhs, const foreground_color_t& rhs) {
		return combined_color_t( (lhs.color & 0xf0) | (rhs.color & 0x0f) );
	}

	inline combined_color_t operator + (const foreground_color_t& lhs, const background_color_t& rhs) {
		return combined_color_t( (rhs.color & 0xf0) | (lhs.color & 0x0f) );
	}

	inline unsigned char& std_out_color() {
		static unsigned char _ = 0x07;
		return _;
	}

	inline unsigned char& std_err_color() {
		static unsigned char _ = 0x07;
		return _;
	}

	namespace detail {
		inline void set_std_out_color(unsigned char color) {
			static HANDLE std_out = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(std_out, color);
		}

		inline void set_std_err_color(unsigned char color) {
			static HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);
			SetConsoleTextAttribute(handle, color);
		}
	}


	// std::out
	inline void set_std_out_color(const combined_color_t& color) {
		std_out_color() = color.color;
		detail::set_std_out_color(std_out_color());
	}

	inline void set_std_out_color(const foreground_color_t& c) {
		std_out_color() = (std_out_color() & 0xf0) | (c.color & 0x0f);
		detail::set_std_out_color(std_out_color());
	}

	inline void set_std_out_color(const background_color_t& c) {
		std_out_color() = (std_out_color() & 0x0f) | (c.color & 0xf0);
		detail::set_std_out_color(std_out_color());
	}

	// std::err
	inline void set_std_err_color(const combined_color_t& color) {
		std_err_color() = color.color;
		detail::set_std_err_color(std_err_color());
	}

	inline void set_std_err_color(const foreground_color_t& c) {
		std_err_color() = (std_err_color() & 0xf0) | (c.color & 0x0f);
		detail::set_std_err_color(std_err_color());
	}

	inline void set_std_err_color(const background_color_t& c) {
		std_err_color() = (std_err_color() & 0x0f) | (c.color & 0xf0);
		detail::set_std_err_color(std_err_color());
	}



	inline std::ostream& operator << (std::ostream& lhs, const combined_color_t& rhs)
	{
		if (&lhs == &std::cout) {
			set_std_out_color(rhs);
		}
		else if (&lhs == &std::cerr) {
			set_std_err_color(rhs);
		}

		return lhs;
	}

	inline std::ostream& operator << (std::ostream& lhs, const foreground_color_t& rhs)
	{
		if (&lhs == &std::cout) {
			set_std_out_color(rhs);
		}
		else if (&lhs == &std::cerr) {
			set_std_err_color(rhs);
		}

		return lhs;
	}

	inline std::ostream& operator << (std::ostream& lhs, const background_color_t& rhs)
	{
		if (&lhs == &std::cout) {
			set_std_out_color(rhs);
		}
		else if (&lhs == &std::cerr) {
			set_std_err_color(rhs);
		}

		return lhs;
	}
	

	namespace
	{
		foreground_color_t fg_red(0x0c);
		foreground_color_t fg_green(0x0a);
		foreground_color_t fg_blue(0x09);
		foreground_color_t fg_yellow(0x0e);
		foreground_color_t fg_brightwhite(0xf);
		foreground_color_t fg_dark_green(0x2);

		background_color_t bg_red(0xc0);
		background_color_t bg_green(0xa0);
		background_color_t bg_blue(0x90);
		background_color_t bg_yellow(0xe0);
		background_color_t bg_dark_red(0x40);
		background_color_t bg_dark_green(0x20);
		background_color_t bg_dark_blue(0x10);

		combined_color_t reset(0x07);
	}


	struct console_logging_handler_t : logging_handler_t
	{
		console_logging_handler_t()
		{
			
		}

		auto handle(log_level_t level, unique_memory_t const& data) -> void override
		{
			
		}
	};






	
}
