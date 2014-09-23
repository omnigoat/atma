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
		auto bytes() const -> size_t;
		auto c_str() const -> char const*;


		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto begin() -> iterator;
		auto end() -> iterator;

		auto begin_raw() const -> char const*;
		auto end_raw() const -> char const*;
		auto begin_raw() -> char*;
		auto end_raw() -> char*;

		auto push_back(char c) -> void;
		auto push_back(utf8_char_t const& c) -> void;

		//template <typename IT> auto insert(const_iterator const&, IT const&, IT const&);
		//auto insert(const_iterator const&, char const*, char const*) -> void;

		auto clear() -> void;

		auto operator += (utf8_string_t const& rhs) -> utf8_string_t&;

	private:
		typedef std::vector<value_t> chars_t;
		chars_t chars_;

		friend class utf16_string_t;
		friend auto operator < (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool;
		friend auto operator + (utf8_string_t const& lhs, utf8_string_t const& rhs) -> utf8_string_t;
		friend auto operator + (utf8_string_t const& lhs, std::string const& rhs) -> utf8_string_t;
		friend auto operator + (utf8_string_t const& lhs, utf8_string_range_t const& rhs) -> utf8_string_t;
		friend auto operator << (std::ostream& out, utf8_string_t const& rhs) -> std::ostream&;
	};

	auto operator == (utf8_string_t const&, utf8_string_t const&) -> bool;
	auto operator != (utf8_string_t const&, utf8_string_t const&) -> bool;

	auto operator == (utf8_string_t const&, char const*) -> bool;
	auto operator == (char const*, utf8_string_t const&) -> bool;



	template <typename T>
	class utf8_string_t::iterator_t : public std::iterator<std::bidirectional_iterator_tag, T>
	{
		friend class utf8_string_t;

		using source_container_t = std::conditional_t<std::is_const<T>::value, utf8_string_t const, utf8_string_t>;
		using source_iterator  = std::conditional_t<std::is_const<T>::value, utf8_string_t::chars_t::const_iterator, utf8_string_t::chars_t::iterator>;
		using source_char_t = std::conditional_t<std::is_const<T>::value, char const, char>;

	public:
		iterator_t(source_container_t& owner, source_char_t* x)
		: owner_(&owner), char_(x, x + utf8_char_bytecount(x))
		{}

		iterator_t(iterator_t const& rhs)
			: owner_(rhs.owner_), char_(rhs.char_)
		{}

		template <typename U>
		iterator_t(iterator_t<U> const& rhs, std::enable_if_t<std::is_convertible<U*, T*>::value, int> = int())
		: owner_(rhs.owner_), char_(rhs.char_)
		{}
		
		auto operator = (iterator_t const& rhs) -> iterator_t
		{
			owner_ = rhs.owner_;
			char_ = rhs.char_;
			return *this;
		}

		auto operator * () -> utf8_char_t& {
			return char_;
		}

		auto operator ++ () -> iterator_t&
		{
			ATMA_ASSERT(char_.begin != owner_->end_raw());
			auto mv = 
			char_.begin = char_.end;
			char_.end = utf8_char_advance(char_.end);
			return *this;
		}

		auto operator ++ (int) -> iterator_t {
			auto t = *this;
			++*this;
			return t;
		}

		auto operator -> () const -> T* {
			return &char_;
		}

	private:
		source_container_t* owner_;
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

	
//=====================================================================
} // namespace atma
//=====================================================================
