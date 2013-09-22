#ifndef ATMA_UTF_UTF8_ITERATOR
#define ATMA_UTF_UTF8_ITERATOR
//=====================================================================
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
//=====================================================================
namespace atma {
//=====================================================================
	
	struct utf8_iterator
	{
		utf8_iterator(char* begin);

		auto operator *() const -> char;
		auto operator ++() -> utf8_iterator&;
		auto operator --() -> utf8_iterator&;
		
	private:
		char* here_;
	};

	auto operator utf8_iterator::operator++ () -> utf8_iterator&
	{
		here_ = utf8_next_char(here_);
		return *this;
	}

	auto operator utf8_iterator::operator *() -> char
	{
		return *here_;
	}

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
