#ifndef ATMA_UTF_UTF8_STRING
#define ATMA_UTF_UTF8_STRING
//=====================================================================
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
#include <string>
#include <vector>
#include <cstdint>
//=====================================================================
namespace atma {
//=====================================================================

	//=====================================================================
	// we use these
	//=====================================================================
	struct utf8_char_t;
	class utf8_string_t;
	class utf8_string_range_t;
	


	struct utf8_char_t
	{
		char const* begin;
		char const* end;
	};

	inline auto operator == (utf8_char_t const& lhs, utf8_char_t const& rhs) -> bool {
		return (lhs.end - lhs.begin) == (rhs.end - rhs.begin)
			&& std::equal(lhs.begin, lhs.end, rhs.begin)
			;
	}

	inline auto operator == (utf8_char_t const& lhs, char x) -> bool {
		// it only makes sense to compare a character against a character, and
		// not a character against a leading-byte in a char-seq.
		ATMA_ASSERT(is_ascii(x));

		return *lhs.begin == x;
	}


	class utf8_string_t
	{
	public:
		template <typename T> class iterator_t;
		
		typedef char value_t;
		typedef iterator_t<utf8_char_t> iterator;
		typedef iterator_t<utf8_char_t const> const_iterator;

		utf8_string_t();
		utf8_string_t(char const* str);
		utf8_string_t(char const* str_begin, char const* str_end);
		//explicit utf8_string_t(utf16_string_t const&);
		utf8_string_t(utf8_string_t const&);
		utf8_string_t(utf8_string_t&&);

		

		auto empty() const -> bool { return chars_.empty(); }
		auto bytes() const -> uint32_t { return chars_.size(); }

		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto begin() -> iterator;
		auto end() -> iterator;

		auto bytes_begin() const -> char const* { return &chars_[0]; }
		auto bytes_end() const -> char const* { return &chars_[0] + chars_.size(); }
		auto bytes_begin() -> char* { return &chars_[0]; }
		auto bytes_end() -> char* { return &chars_[0]; }

		// push back a single character is valid only for code-points < 128
		auto push_back(char c) -> void { 
			ATMA_ASSERT(c ^ 0x80);
			chars_.push_back(c);
			++char_count_;
		}

		//auto insert()

		auto operator += (utf8_string_t const& rhs) -> utf8_string_t&
		{
			chars_.insert(chars_.end(), rhs.bytes_begin(), rhs.bytes_end());
			return *this;
		}

	private:
		typedef std::vector<value_t> chars_t;
		chars_t chars_;
		uint32_t char_count_;


		friend class utf16_string_t;
		friend auto operator < (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool;
		friend auto operator + (utf8_string_t const& lhs, utf8_string_t const& rhs) -> utf8_string_t;
		friend auto operator + (utf8_string_t const& lhs, std::string const& rhs) -> utf8_string_t;
		friend auto operator + (utf8_string_t const& lhs, utf8_string_range_t const& rhs) -> utf8_string_t;
		friend auto operator << (std::ostream& out, utf8_string_t const& rhs) -> std::ostream&;
	};




	template <typename T>
	class utf8_string_t::iterator_t : std::iterator<std::bidirectional_iterator_tag, T>
	{
	public:
		typedef typename std::conditional<std::is_const<T>::value, utf8_string_t const, utf8_string_t>::type& container_ref;
		typedef typename std::conditional<std::is_const<T>::value, char const*, char*>::type char_ptr;
		typedef typename std::conditional<std::is_const<T>::value, utf8_char_t const, utf8_char_t>::type char_t;

		iterator_t(container_ref owner, char_ptr x)
		: owner_(owner), char_(x, utf8_next_char(x))
		{}

		iterator_t(iterator_t<typename std::remove_const<T>::type> const& rhs)
		: owner_(rhs.owner_), char_(rhs.char_)
		{}
		
		auto operator = (iterator_t const& rhs) -> iterator_t
		{
			owner_ = rhs.owner_;
			char_ = rhs.char_;
			return *this;
		}

		auto operator * () -> char_t& {
			return char_;
		}

		auto operator ++ () -> iterator_t& {
			ATMA_ASSERT(x_ != owner_.bytes_end());
			char_.begin = char_.end;
			char_.end += utf8_next_char(x_);
			return *this;
		}

		auto operator -> () const -> char_t* {
			return &utf8_char_t;
		}

	private:
		container_ref owner_;
		char_t char_;

		template <typename T>
		friend auto operator == (iterator_t<T> const&, iterator_t<T> const&) -> bool;
	};

	template <typename T, typename U>
	inline auto operator == (utf8_string_t::iterator_t<T> const& lhs, utf8_string_t::iterator_t<U> const& rhs) -> bool
	{
		return lhs.x_ == rhs.x_;
	}

	template <typename T, typename U>
	inline auto operator != (utf8_string_t::iterator_t<T> const& lhs, utf8_string_t::iterator_t<U> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	
//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
