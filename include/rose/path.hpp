#pragma once


#include <atma/string.hpp>

#include <filesystem>

namespace stdfs = std::experimental::filesystem;


//
// rose::path_t
//  - essentially a utf8-only version of std::path
//
namespace rose
{
	struct path_t
	{
		path_t() {}
		path_t(atma::string const& x) : string_{x} {}
		path_t(char const* x) : string_{x} {}
		path_t(char const* begin, char const* end) : string_{begin, end} {}
		path_t(atma::string&& x) : string_{std::move(x)} {}

		auto operator /= (atma::string const&) -> path_t&;

		auto string() const -> atma::string const& { return string_; }
		auto c_str() const -> char const* { return string_.c_str(); }
		auto extension() const -> atma::string;
		auto directory() const -> path_t;

		operator stdfs::path() const { return stdfs::path{string_.c_str()}; }

	private:
		atma::string string_;
	};

	inline auto path_t::operator /= (atma::string const& rhs) -> path_t&
	{
		if (!string_.empty() && *--string_.end() != '/')
			string_.push_back('/');
		string_.append(rhs);
		return *this;
	}

	inline auto path_t::extension() const -> atma::string
	{
		if (string_.empty())
			return atma::string{};

		auto rit = atma::find_first_of(string_.rbegin(), string_.rend(), "/");
		auto it = rit.base();
		for (auto it2 = it; it2 != string_.end(); ++it2)
			if (*it2 == '.')
				return atma::string((*++it2).data(), string_.raw_end());

		return atma::string{};
	}

	inline auto path_t::directory() const -> path_t
	{
		if (string_.empty())
			return path_t{};

		auto rit = atma::find_first_of(string_.rbegin(), string_.rend(), "/");

		return path_t{string_.raw_begin(), (*rit).data()};
	}

	inline auto operator < (path_t const& lhs, path_t const& rhs)
	{
		return lhs.string() < rhs.string();
	}

	inline auto operator == (path_t const& lhs, path_t const& rhs) -> bool
	{
		return lhs.string() == rhs.string();
	}

	inline auto operator != (path_t const& lhs, path_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	inline auto operator / (path_t const& lhs, path_t const& rhs) -> path_t
	{
		return path_t{lhs.string() + "/" + rhs.string()};
	}

	inline auto operator / (path_t const& lhs, atma::string const& rhs) -> path_t
	{
		return path_t{lhs.string() + "/" + rhs};
	}

	inline auto operator / (path_t const& lhs, char const* rhs) -> path_t
	{
		return path_t{lhs.string() + "/" + rhs};
	}






	inline auto findinc_path_separator(char const* begin, char const* end) -> char const*
	{
		auto i = begin;
		while (i != end && *i != '/')
			++i;

		return (i == end) ? i : i + 1;
	}

	struct path_range_iter_t
	{
		path_range_iter_t(char const* begin, char const* end, char const* terminal)
			:range_{begin, end}, terminal_(terminal)
		{}

		auto operator * () const -> atma::utf8_string_range_t const&
		{
			return range_;
		}

		auto operator -> () const -> atma::utf8_string_range_t const*
		{
			return &range_;
		}

		auto operator ++ () -> path_range_iter_t&
		{
			range_ = atma::utf8_string_range_t{range_.end(), findinc_path_separator(range_.end(), terminal_)};
			return *this;
		}

		auto operator != (path_range_iter_t const& rhs) const -> bool
		{
			return range_ != rhs.range_ || terminal_ != rhs.terminal_;
		}

	private:
		atma::utf8_string_range_t range_;
		char const* terminal_;
	};

	struct path_range_t
	{
		path_range_t(char const* str, size_t size)
			: str_(str), size_(size)
		{}

		auto begin() -> path_range_iter_t
		{
			auto r = findinc_path_separator(str_, str_ + size_);
			return{str_, r, str_ + size_};
		}

		auto end() -> path_range_iter_t
		{
			return{str_ + size_, str_ + size_, str_ + size_};
		}

	private:
		char const* str_;
		size_t size_;
	};


	inline auto path_split_range(atma::string const& p) -> path_range_t
	{
		return path_range_t{p.c_str(), p.raw_size()};
	}

	inline auto path_split_range(path_t const& p) -> path_range_t
	{
		return path_split_range(p.string());
	}
}
