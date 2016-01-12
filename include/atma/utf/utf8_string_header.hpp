#pragma once

#include <atma/types.hpp>
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
#include <atma/utf/utf8_char.hpp>

#include <string>
#include <vector>
#include <cstdint>


namespace atma
{
	struct utf8_string_t;
	struct utf8_string_range_t;


	struct utf8_string_t
	{
		struct iterator_t;
		
		using value_t        = char;
		using iterator       = iterator_t;
		using const_iterator = iterator_t;

		utf8_string_t();
		utf8_string_t(char const* str, size_t size);
		utf8_string_t(char const* str_begin, char const* str_end);
		utf8_string_t(char const* str);
		utf8_string_t(const_iterator const&, const_iterator const&);
		utf8_string_t(utf8_string_range_t const&);
		utf8_string_t(utf8_string_t const&);
		utf8_string_t(utf8_string_t&&);

		~utf8_string_t();

		auto operator = (utf8_string_t const&) -> utf8_string_t&;
		auto operator += (utf8_string_t const& rhs) -> utf8_string_t&;
		auto operator += (char const*) -> utf8_string_t&;


		auto empty() const -> bool;
		auto c_str() const -> char const*;
		auto raw_size() const -> size_t;

		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;

		auto raw_begin() const -> char const*;
		auto raw_end() const -> char const*;

		auto raw_iter_of(const_iterator const&) const -> char const*;

		auto push_back(char) -> void;
		auto push_back(utf8_char_t) -> void;

		auto append(char const*, char const*) -> void;
		auto append(char const*) -> void;
		auto append(utf8_string_t const&) -> void;
		auto append(utf8_string_range_t const&) -> void;

		auto clear() -> void;

	private:
		auto imem_quantize(size_t, bool keep) -> void;
		auto imem_quantize_grow(size_t) -> void;

		auto imem_quantize_capacity(size_t) const -> size_t;
		auto imem_realloc(size_t, bool keep) -> void;


	private:
		size_t capacity_;
		size_t size_;
		char* data_;

		friend class utf16_string_t;
	};

	auto operator == (utf8_string_t const&, utf8_string_t const&) -> bool;
	auto operator != (utf8_string_t const&, utf8_string_t const&) -> bool;

	auto operator == (utf8_string_t const&, char const*) -> bool;
	auto operator == (char const*, utf8_string_t const&) -> bool;

	auto operator + (utf8_string_t const&, utf8_string_t const&)       -> utf8_string_t;
	auto operator + (utf8_string_t const&, char const*)                -> utf8_string_t;
	auto operator + (utf8_string_t const&, std::string const&)         -> utf8_string_t;
	auto operator + (utf8_string_t const&, utf8_string_range_t const&) -> utf8_string_t;

	auto operator << (std::ostream&, utf8_string_t const&) -> std::ostream&;




	struct utf8_string_t::iterator_t
	{
	private:
		friend struct utf8_string_t;

		using owner_t = utf8_string_t;

	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = ptrdiff_t;
		using value_type        = utf8_char_t;
		using pointer           = utf8_char_t*;
		using reference         = utf8_char_t const&;

		iterator_t(iterator_t const&);

		auto operator = (iterator_t const&) -> iterator_t&;
		auto operator ++ () -> iterator_t&;
		auto operator ++ (int) -> iterator_t;

		auto operator * () const -> value_type;
		auto operator -> () const -> value_type;

	private:
		iterator_t(owner_t const*, char const*);

	private:
		owner_t const* owner_;
		char const* ptr_;

		friend auto operator == (utf8_string_t::iterator_t const&, utf8_string_t::iterator_t const&) -> bool;
	};

	inline auto operator == (utf8_string_t::iterator_t const&, utf8_string_t::iterator_t const&) -> bool;
	inline auto operator != (utf8_string_t::iterator_t const&, utf8_string_t::iterator_t const&) -> bool;

}
