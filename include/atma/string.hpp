#pragma once

#include <atma/utf/utf8_string.hpp>

#include <atma/streams.hpp>

#include <algorithm>

import atma.vector;


namespace atma {

	// bam.
	using string = utf8_string_t;
	//using string_range_t = utf8_string_range_t;





	struct string_encoder_t
	{
		string_encoder_t(char*, size_t);
		string_encoder_t(string&);

		template <typename T>
		auto operator << (T&& t) -> string_encoder_t&
		{
			write(std::forward<T>(t));
			return *this;
		}

		auto write(char const*, size_t) -> size_t;
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
		memory_bytestream_t stream_;
		string* str_ = nullptr;
	};

	inline string_encoder_t::string_encoder_t(char* buf, size_t size)
		: stream_{buf, size}
		, put_fn_(&string_encoder_t::put_buf)
	{}

	inline bool string_encoder_t::put_buf(char c)
	{
		if (stream_.position() == stream_.size())
			return false;

		stream_.write(&c, 1);
		return true;
	}

	inline auto string_encoder_t::write(char const* str, size_t size) -> size_t
	{
		size_t r = 0;
		for ( ; r != size; ++r)
		{
			if (!(this->*put_fn_)(*str))
				break;
		}

		return r;
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
		if (x < 0)
			if (!(this->*put_fn_)('-'))
				return 0;
			else
				return 1 + write(ULLONG_MAX - uint64(x) + 1);
		else
			return write(uint64(x));
	}

	inline auto string_encoder_t::write(uint64 x) -> size_t
	{
		// binary search for digits (max value: 18,446,744,073,709,551,615)
		uint64 digits = 0;
		uint64 d = 0;
		if (x < 1'000'000'000'000)
			if (x < 1'000'000)
				if (x < 1000)
					if (x < 10) { digits = 1; d = 1; }
					else if (x < 100) { digits = 2; d = 10; }
					else { digits = 3; d = 100; }
				else
					if (x < 10'000) { digits = 4; d = 1000; }
					else if (x < 100'000) { digits = 5; d = 10'000; }
					else { digits = 6; d = 100'000; }
			else
				if (x < 1'000'000'000)
					if (x < 10'000'000) { digits = 7; d = 1'000'000; }
					else if (x < 100'000'000) { digits = 8; d = 10'000'000; }
					else { digits = 9; d = 100'000'000; }
				else
					if (x < 10'000'000'000) { digits = 10; d = 1'000'000'000; }
					else if (x < 100'000'000'000) { digits = 11; d = 10'000'000'000; }
					else { digits = 12; d = 100'000'000'000; }
		else if (x < 1'000'000'000'000'000)
			if (x < 10'000'000'000'000) { digits = 13; d = 1'000'000'000'000; }
			else if (x < 100'000'000'000'000) { digits = 14; d = 10'000'000'000'000; }
			else { digits = 15; d = 100'000'000'000'000; }
		else if (x < 100'000'000'000'000'000)
			if (x < 10'000'000'000'000'000) { digits = 16; d = 1'000'000'000'000'000; }
			else { digits = 17; d = 10'000'000'000'000'000; }
		else if (x < 1'000'000'000'000'000'000) { digits = 18; d = 100'000'000'000'000'000; }
		else if (x < 10'000'000'000'000'000'000) { digits = 19; d = 1'000'000'000'000'000'000; }
		else { digits = 20; d = 10'000'000'000'000'000'000; }

		size_t r = 0;
		for (; d != 0; d /= 10, ++r)
		{
			char c = (x / d) % 10;
			if (!(this->*put_fn_)('0' + c))
				break;
		}

		return r;
	}







	template <std::integral T>
	requires std::is_unsigned_v<T>
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
