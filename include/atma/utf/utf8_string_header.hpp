#pragma once

#include <atma/types.hpp>
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
#include <atma/utf/utf8_char.hpp>

#include <string>
#include <vector>
#include <cstdint>


namespace atma {

	struct utf8_char_t;
	class utf8_string_t;
	class utf8_string_range_t;


	class utf8_string_t
	{
	public:
		template <typename T> class iterator_t;
		
		typedef char value_t;
		typedef iterator_t<utf8_char_t> iterator;
		typedef iterator_t<utf8_char_t const> const_iterator;

		utf8_string_t();
		utf8_string_t(const_iterator const&, const_iterator const&);
		utf8_string_t(char const* str);
		utf8_string_t(char const* str_begin, char const* str_end);
		utf8_string_t(utf8_string_range_t const&);
		utf8_string_t(utf8_string_t const&);
		utf8_string_t(utf8_string_t&&);

		auto empty() const -> bool;
		auto c_str() const -> char const*;

		// size in bytes, but not characters
		auto raw_size() const -> size_t;


		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto begin() -> iterator;
		auto end() -> iterator;

		auto raw_begin() const -> char const*;
		auto raw_end() const -> char const*;
		auto raw_begin() -> char*;
		auto raw_end() -> char*;

		auto raw_iter_of(const_iterator const&) const -> char const*;

		auto push_back(char c) -> void;
		auto push_back(utf8_char_t const& c) -> void;

		//template <typename IT> auto insert(const_iterator const&, IT const&, IT const&);
		//auto insert(const_iterator const&, char const*, char const*) -> void;

		auto clear() -> void;

		auto operator += (utf8_string_t const& rhs) -> utf8_string_t&;
		auto operator += (char const*) -> utf8_string_t&;
		
	private:
		auto imem_quantize_capacity(size_t) const -> size_t;
		auto imem_realloc(size_t) -> void;

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



	template <typename value_type_tx>
	class utf8_string_t::iterator_t
	{
		friend class utf8_string_t;

		using owner_t      = std::conditional_t<std::is_const<value_type_tx>::value, utf8_string_t const, utf8_string_t>;
		using owner_iter_t = std::conditional_t<std::is_const<value_type_tx>::value, char const*, char*>;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = value_type_tx;
		using difference_type   = ptrdiff_t;
		using distance_type     = ptrdiff_t;
		using pointer           = value_type*;
		using reference         = value_type&;
		using const_reference   = value_type const&;


		iterator_t(owner_t& owner, owner_iter_t iter)
		: owner_(&owner)
		{
			if (iter != owner_->raw_end())
				char_ = utf8_char_t(iter, utf8_char_advance(iter));
		}

		iterator_t(iterator_t const& rhs)
			: owner_(rhs.owner_), char_(rhs.char_)
		{}

		template <typename U>
		iterator_t(iterator_t<U> const& rhs, std::enable_if_t<std::is_convertible<U*, value_type_tx*>::value, int> = int())
		: owner_(rhs.owner_), char_(rhs.char_)
		{}
		
		auto operator = (iterator_t const& rhs) -> iterator_t&
		{
			owner_ = rhs.owner_;
			char_ = rhs.char_;
			return *this;
		}

		auto operator * () -> reference {
			return char_;
		}

		auto operator ++ () -> iterator_t&
		{
			ATMA_ASSERT(char_.begin != owner_->raw_end());
			char_.begin = char_.end;
			char_.end = utf8_char_advance(char_.end);
			return *this;
		}

		auto operator ++ (int) -> iterator_t {
			auto t = *this;
			++*this;
			return t;
		}

		auto operator -> () const -> pointer {
			return &char_;
		}

	private:
		owner_t* owner_;
		utf8_char_t char_;

		template <typename>
		friend class iterator_t;

		template <typename T, typename U>
		friend auto operator == (iterator_t<T> const&, iterator_t<U> const&) -> bool;
	};

	template <typename T, typename U>
	inline auto operator == (utf8_string_t::iterator_t<T> const& lhs, utf8_string_t::iterator_t<U> const& rhs) -> bool
	{
		return lhs.char_.begin == rhs.char_.begin;
	}

	template <typename T, typename U>
	inline auto operator != (utf8_string_t::iterator_t<T> const& lhs, utf8_string_t::iterator_t<U> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

}
