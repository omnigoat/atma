#pragma once

#include <atma/utf/utf8_string.hpp>
#include <atma/utf/utf8_string_range.hpp>

#include <atma/enable_if.hpp>
#include <atma/vector.hpp>

#include <algorithm>


namespace atma {

	// bam.
	using string = utf8_string_t;
	using string_range_t = utf8_string_range_t;





	struct string_encoder_t
	{
		string_encoder_t(char* buf, size_t size);
		string_encoder_t(string& buf);

		template <typename T>
		auto operator << (T&& t) -> string_encoder_t&
		{
			write(std::forward<T>(t));
			return *this;
		}

		auto write(char const*) -> size_t;
		auto write(int64) -> size_t;
		auto write(uint64) -> size_t;

	private:
		bool put_buf(char);
		bool put_str(char);

	private:
		using put_fn = bool(string_encoder_t::*)(char);

		put_fn put_fn_;

	private:
		char* buf_ = nullptr;
		size_t size_ = 0;
		size_t p_ = 0;

		string* str_ = nullptr;
	};

	inline string_encoder_t::string_encoder_t(char* buf, size_t size)
		: buf_(buf), size_(size)
		, put_fn_(&string_encoder_t::put_buf)
	{}

	inline bool string_encoder_t::put_buf(char c)
	{
		if (p_ == size_)
			return false;

		buf_[p_++] = c;
		return true;
	}

	inline auto string_encoder_t::write(char const* str) -> size_t
	{
		size_t r = 0;
		for ( ; *str; ++str, ++r)
		{
			if (!(this->*put_fn_)(*str))
				break;
		}

		return r;
	}

	inline auto string_encoder_t::write(int64 x) -> size_t
	{
		if (!(this->*put_fn_)('-'))
			return 0;

		return 1 + write(uint64(x));
	}

	inline auto string_encoder_t::write(uint64 x) -> size_t
	{
		// first step: find no. of digits
		uint64 x2 = x;
		size_t digits = 0;
		uint64 m = 1;
		for (;;) { x2 /= 10; ++digits; if (x2 != 0) break; m *= 10; }

		for ( ; digits != 0; m /= 10)
		{
			(this->*put_fn_)('0' + (x / m) % 10);
		}

		return 4;
	}









	template <typename T, typename = atma::enable_if<std::is_integral<T>, std::is_unsigned<T>>>
	inline auto to_string(T x, uint base = 10) -> atma::string
	{
		atma::string s;

		// do characters
		do {
			auto xb = x % base;

			if (xb > 9)
				s.push_back(xb - 10 + 'a');
			else
				s.push_back(xb + '0');

			x /= base;
		} while (x > 0);

		std::reverse(s.raw_begin(), s.raw_end());

		return s;
	}

	



	inline auto split(atma::string const& s, char const* delims) -> atma::vector<atma::string>
	{
		auto begin = s.begin();
		auto end = s.end();

		atma::vector<atma::string> result;

		auto si = begin;
		for (auto i = begin, pi = begin; i != end; ++i)
		{
			for (auto const* d = delims; *d; utf8_char_advance(d))
			{
				//if (utf8_char_equality(*i, d))
				//{
				//	result.push_back(atma::string(si, pi));
				//	si = i;
				//	break;
				//}
			}

			pi = i;
		}
	}

}
