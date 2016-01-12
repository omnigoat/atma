#pragma once

#include <atma/utf/utf8_string_header.hpp>
#include <atma/utf/utf8_string_range_header.hpp>


namespace atma {

	inline utf8_string_t::utf8_string_t()
		: capacity_(8)
		, size_()
		, data_(new char[capacity_])
	{
		memset(data_, 0, capacity_);
	}

	inline utf8_string_t::utf8_string_t(char const* str, size_t size)
		: capacity_(imem_quantize_capacity(size))
		, size_(size)
		, data_(new char[capacity_])
	{
		memcpy(data_, str, size_);
		data_[size_] = '\0';
	}

	inline utf8_string_t::utf8_string_t(char const* begin, char const* end)
		: utf8_string_t(begin, end - begin)
	{
	}

	inline utf8_string_t::utf8_string_t(char const* str)
		: utf8_string_t(str, strlen(str))
	{
	}

	inline utf8_string_t::utf8_string_t(const_iterator const& begin, const_iterator const& end)
		: utf8_string_t(*begin, *end)
	{
	}

	inline utf8_string_t::utf8_string_t(utf8_string_t const& str)
		: utf8_string_t(str.raw_begin(), str.raw_size())
	{
	}

	inline utf8_string_t::utf8_string_t(utf8_string_range_t const& range)
		: utf8_string_t{range.begin(), range.raw_size()}
	{
	}

#if 0
	utf8_string_t::utf8_string_t(const utf16_string_t& rhs)
	{
		// optimistically reserve one char per char16_t. this will work optimally
		// for english text, and will still totes work fine for anything else.
		chars_.reserve(rhs.char_count_);

		utf8_from_utf16(std::back_inserter(chars_), rhs.chars_.begin(), rhs.chars_.end());
	}
#endif

	inline utf8_string_t::utf8_string_t(utf8_string_t&& rhs)
		: capacity_(rhs.capacity_)
		, size_(rhs.size_)
		, data_(rhs.data_)
	{
		rhs.capacity_ = 0;
		rhs.size_ = 0;
		rhs.data_ = nullptr;
	}

	inline utf8_string_t::~utf8_string_t()
	{
		delete data_;
		capacity_ = 0;
		size_ = 0;
		data_ = nullptr;
	}

	inline auto utf8_string_t::operator += (utf8_string_t const& rhs) -> utf8_string_t&
	{
		imem_quantize(size_ + rhs.size_, true);

		memcpy(data_ + size_, rhs.data_, rhs.size_);
		size_ += rhs.size_;
		data_[size_] = '\0';

		return *this;
	}

	inline auto utf8_string_t::operator += (char const* rhs) -> utf8_string_t&
	{
		auto rhslen = strlen(rhs);
		imem_quantize(size_ + rhslen, true);

		memcpy(data_ + size_, rhs, rhslen);
		size_ += rhslen;
		data_[size_] = '\0';

		return *this;
	}

	inline auto utf8_string_t::operator = (utf8_string_t const& rhs) -> utf8_string_t&
	{
		imem_quantize(rhs.size_, false);

		size_ = rhs.size_;
		memcpy(data_, rhs.data_, size_);
		data_[size_] = '\0';

		return *this;
	}

	inline auto utf8_string_t::empty() const -> bool
	{
		return size_ == 0;
	}

	inline auto utf8_string_t::raw_size() const -> size_t
	{
		return size_;
	}

	inline auto utf8_string_t::c_str() const -> char const*
	{
		return raw_begin();
	}

	inline auto utf8_string_t::raw_begin() const -> char const*
	{
		return data_;
	}

	inline auto utf8_string_t::raw_end() const -> char const*
	{
		return data_ + size_;
	}

	inline auto utf8_string_t::begin() const -> const_iterator
	{
		return const_iterator(this, raw_begin());
	}

	inline auto utf8_string_t::end() const -> const_iterator
	{
		return const_iterator(this, raw_end());
	}

	inline auto utf8_string_t::raw_iter_of(const_iterator const& iter) const -> char const*
	{
		return iter.ptr_;
	}

	inline auto utf8_string_t::push_back(char c) -> void
	{
		ATMA_ASSERT(c ^ 0x80);

		imem_quantize_grow(1);

		data_[size_++] = c;
		data_[size_] = '\0';
	}

	inline auto utf8_string_t::push_back(utf8_char_t c) -> void
	{
#if 0
		auto sz = std::distance(c.begin, c.end);
		imem_quantize_grow(sz);

		memcpy(data_ + size_, c.begin, sz);
		size_ += sz;
		data_[size_] = '\0';
#endif
	}

	inline auto utf8_string_t::append(char const* begin, char const* end) -> void
	{
		auto sz = end - begin;
		if (sz == 0)
			return;
		imem_quantize_grow(sz);

		memcpy(data_ + size_, begin, sz);
		size_ += sz;
		data_[size_] = '\0';
	}

	inline auto utf8_string_t::append(char const* str) -> void
	{
		append(str, str + strlen(str));
	}

	inline auto utf8_string_t::append(utf8_string_t const& str) -> void
	{
		append(str.raw_begin(), str.raw_end());
	}

	inline auto utf8_string_t::append(utf8_string_range_t const& str) -> void
	{
		append(str.begin(), str.end());
	}

	inline auto utf8_string_t::clear() -> void
	{
		size_ = 0;
	}

	inline auto utf8_string_t::imem_quantize(size_t size, bool keep) -> void
	{
		size_t newcap = imem_quantize_capacity(size);

		if (newcap != capacity_)
			imem_realloc(newcap, keep);
	}

	inline auto utf8_string_t::imem_quantize_grow(size_t size) -> void
	{
		if (capacity_ <= size_ + size)
			imem_realloc(imem_quantize_capacity(size_ + size), true);
	}

	inline auto utf8_string_t::imem_quantize_capacity(size_t size) const -> size_t
	{
		size_t result = 8;
		while (size >= 8)
		{
			result *= 2;
			size /= 2;
		}

		return result;
	}

	inline auto utf8_string_t::imem_realloc(size_t capacity, bool keep) -> void
	{
		if (keep && data_)
		{
			auto tmp = data_;
			capacity_ = capacity;
			data_ = new char[capacity_];
			memcpy(data_, tmp, size_);
			data_[size_] = '\0';
			delete tmp;
		}
		else
		{
			delete data_;
			capacity_ = capacity;
			data_ = new char[capacity_];
		}
	}




	//=====================================================================
	// operators
	//=====================================================================
	inline auto operator == (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return lhs.raw_size() == rhs.raw_size() && ::strcmp(lhs.raw_begin(), rhs.raw_begin()) == 0;
	}

	inline auto operator != (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	inline auto operator == (utf8_string_t const& lhs, char const* rhs) -> bool
	{
		return ::strcmp(lhs.raw_begin(), rhs) == 0;
	}

	inline auto operator < (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return std::lexicographical_compare(lhs.raw_begin(), lhs.raw_end(), rhs.raw_begin(), rhs.raw_end());
	}

	inline auto operator + (utf8_string_t const& lhs, utf8_string_t const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result += rhs;
		return result;
	}

	inline auto operator + (utf8_string_t const& lhs, char const* rhs) -> utf8_string_t
	{
		auto result = lhs;
		result += rhs;
		return result;
	}

#if 0
	inline auto operator + (utf8_string_t const& lhs, std::string const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		//result.chars_.insert(result.chars_.end() - 1, rhs.begin(), rhs.end());
		result.insert(result.end(), rhs.begin(), rhs.end());
		return result;
	}

	inline auto operator + (utf8_string_t const& lhs, utf8_string_range_t const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result.chars_.insert(result.chars_.end() - 1, rhs.begin(), rhs.end());
		return result;
	}
#endif

	inline auto operator << (std::ostream& out, utf8_string_t const& rhs) -> std::ostream&
	{
		if (!rhs.empty())
			out.write(rhs.raw_begin(), rhs.raw_size());
		return out;
	}




	//=====================================================================
	// iterator
	//=====================================================================
	inline utf8_string_t::iterator_t::iterator_t(owner_t const* owner, char const* ptr)
		: owner_(owner)
		, ptr_(ptr)
	{
		ATMA_ASSERT(ptr);
		ATMA_ASSERT(utf8_byte_is_leading(*ptr));
	}

	inline utf8_string_t::iterator_t::iterator_t(iterator_t const& rhs)
		: owner_(rhs.owner_)
		, ptr_(rhs.ptr_)
	{}

	inline auto utf8_string_t::iterator_t::operator = (iterator_t const& rhs) -> iterator_t&
	{
		owner_ = rhs.owner_;
		ptr_ = rhs.ptr_;
		return *this;
	}

	inline auto utf8_string_t::iterator_t::operator ++ () -> iterator_t&
	{
		utf8_char_advance(ptr_);
		return *this;
	}

	inline auto utf8_string_t::iterator_t::operator ++ (int) -> iterator_t
	{
		auto t = *this;
		++*this;
		return t;
	}

	inline auto utf8_string_t::iterator_t::operator * () const -> value_type
	{
		return utf8_char_t{ptr_};
	}

	inline auto utf8_string_t::iterator_t::operator -> () const -> value_type
	{
		return utf8_char_t{ptr_};
	}

	inline auto operator == (utf8_string_t::iterator_t const& lhs, utf8_string_t::iterator_t const& rhs) -> bool
	{
		return lhs.ptr_ == rhs.ptr_;
	}

	inline auto operator != (utf8_string_t::iterator_t const& lhs, utf8_string_t::iterator_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}



	//=====================================================================
	// functions
	//=====================================================================
	inline auto strcpy(utf8_string_t& dest, utf8_string_t const& src) -> void
	{
		ATMA_ASSERT(dest.raw_size() >= src.raw_size());

		auto i = dest.begin();
		for (auto& x : src)
			*i++ = x;
	}



	//
	// find_if
	//
	template <typename T>
	inline auto find_if(utf8_string_t::const_iterator const& begin, utf8_string_t::const_iterator const& end, T&& pred) -> utf8_string_t::const_iterator
	{
		auto i = begin;
		for (; i != end; ++i)
			if (pred(*i))
				break;

		return i;
	}

	template <typename T>
	inline auto find_if(utf8_string_t const& string, T&& pred) -> utf8_string_t::const_iterator
	{
		return find_if(string.begin(), string.end(), std::forward<T>(pred));
	}

	//
	// find_first_of
	//
	inline auto find_first_of(utf8_string_t::const_iterator const& begin, utf8_string_t::const_iterator const& end, char const* delims) -> utf8_string_t::const_iterator
	{
		ATMA_ASSERT(delims);

		return find_if(begin, end, [&](utf8_char_t const& lhs) {
			return utf8_charseq_any_of(delims, [&lhs](utf8_char_t const& rhs) {
				return lhs == rhs; }); });
	}

	inline auto find_first_of(utf8_string_t const& str, utf8_string_t::const_iterator const& whhere, char const* delims) -> utf8_string_t::const_iterator
	{
		return find_first_of(whhere, str.end(), delims);
	}

	inline auto find_first_of(utf8_string_t const& str, utf8_string_t::const_iterator const& i, char x) -> utf8_string_t::const_iterator
	{
		ATMA_ASSERT(utf8_char_is_ascii(x));

		char delims[2] = {x, '\0'};

		return find_first_of(i, str.end(), delims);
	}

	inline auto find_first_of(utf8_string_t const& str, char x) -> utf8_string_t::const_iterator
	{
		return find_first_of(str, str.begin(), x);
	}

	inline auto find_first_of(utf8_string_t const& str, char const* delims) -> utf8_string_t::const_iterator
	{
		return find_first_of(str.begin(), str.end(), delims);
	}

	struct utf8_appender_t
	{
		utf8_appender_t(utf8_string_t& dest)
			: str_(dest)
		{}

		template <typename T>
		auto operator ()(T&& x) const -> utf8_appender_t const&
		{
			str_.append(std::forward<T>(x));
			return *this;
		}

	private:
		utf8_string_t& str_;
	};


}

