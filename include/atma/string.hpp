#pragma once

#include <atma/utf/utf8_string.hpp>
#include <atma/utf/utf8_string_range.hpp>

#include <atma/enable_if.hpp>
#include <atma/vector.hpp>
#include <atma/streams.hpp>

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

	//inline auto string_encoder_t::write(int64 x) -> size_t
	//{
	//	if (x < 0)
	//		if (!(this->*put_fn_)('-'))
	//			return 0;
	//		else
	//			return 1 + write(uint64(x) - ULLONG_MAX + 1);
	//	else
	//		return write(uint64(x));
	//}

	//inline auto string_encoder_t::write(uint64 x) -> size_t;









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
