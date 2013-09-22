#ifndef ATMA_UTF_UTF8_ISTREAM
#define ATMA_UTF_UTF8_ISTREAM
//=====================================================================
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
//=====================================================================
namespace atma {
//=====================================================================
	
	class utf8_streambuf_t
	{
	public:
		typedef std::pair<const char*, const char*> sequence_t;

		virtual sequence_t sgetc() const = 0;
		virtual void bump() = 0;
	};

	class utf8_stringbuf : public utf8_streambuf_t
	{
	public:
		utf8_stringbuf(char const* begin, char const* end)
		: begin_(begin), end_(end), current_begin_(begin), current_end_(begin)
		{
			ATMA_ASSERT( is_utf8_leading_byte(*current_begin_) );
			if (!eof()) {
				bump();
			}
		}

		virtual sequence_t sgetc() const
		{
			ATMA_ASSERT(!eof());
			return sequence_t(current_begin_, current_end_);
		}

		virtual void bump() {
			ATMA_ASSERT(!eof());
			
			current_begin_ = current_end_;
			if (current_begin_ != end_) {
				current_end_ = utf8_next_char(current_begin_);
			}
		}

		auto eof() const -> bool {
			return current_begin_ == end_;
		}

	private:
		char const* begin_;
		char const* end_;
		char const* current_begin_;
		char const* current_end_;
	};

	// null-terminated streambuf
	class utf8_nt_string_streambuf : public utf8_streambuf_t
	{
	public:
		utf8_nt_string_streambuf(char const* begin)
		 : begin_(begin), current_begin_(begin), current_end_(begin)
		{
			bump();
		}

		virtual sequence_t sgetc() const
		{
			ATMA_ASSERT(*current_end_ != '\0');
			return sequence_t(current_begin_, current_end_);
		}

		virtual void bump() {
			ATMA_ASSERT(*current_end_ != '\0');
			current_begin_ = current_end_;
			current_end_ = utf8_next_char(current_begin_);
		}

	private:
		char const* begin_;
		char const* current_begin_;
		char const* current_end_;
	};

	//=====================================================================
	// istream
	// ---------
	//   a stream that iterates over raw utf8 characters
	//=====================================================================
	class utf8_istream_t
	{
	public:
		typedef utf8_streambuf_t::sequence_t sequence_t;

		utf8_istream_t(char const* str, unsigned int size);
		utf8_istream_t(char const* str_begin, char const* str_end);
		utf8_istream_t(char const* str);
		~utf8_istream_t();

		sequence_t get() const;
		utf8_istream_t& operator++();
		
	private:
		utf8_streambuf_t* buffer_;
	};
	
	utf8_istream_t::utf8_istream_t(char const* str, unsigned int size)
	 : utf8_istream_t(str, (str + size))
	{
	}

	utf8_istream_t::utf8_istream_t(char const* begin, char const* end)
	 : buffer_(new utf8_stringbuf(begin, end))
	{
	}

	utf8_istream_t::utf8_istream_t(char const* str)
	 : buffer_(new utf8_nt_string_streambuf(str))
	{
	}

	utf8_istream_t::~utf8_istream_t() {
		delete buffer_;
	}

	utf8_istream_t::sequence_t utf8_istream_t::get() const {
		return buffer_->sgetc();
	}
	
	utf8_istream_t& utf8_istream_t::operator++() {
		buffer_->bump();
		return *this;
	}





	//=====================================================================
	// iterator
	//=====================================================================
	class utf8_stream_iterator : public std::iterator<std::forward_iterator_tag, utf8_istream_t::sequence_t>
	{
	public:
		utf8_stream_iterator();
		utf8_stream_iterator(utf8_istream_t& stream);
		
		utf8_stream_iterator& operator ++();
		
		char const* begin() const;
		char const* end() const;
		unsigned int byte_count() const;

		utf8_istream_t::sequence_t operator * () const {
			return stream_->get();
		}

		friend bool operator == (const utf8_stream_iterator& lhs, const utf8_stream_iterator& rhs);
		friend bool operator != (const utf8_stream_iterator& lhs, const utf8_stream_iterator& rhs);

	private:
		void update_character_end();

		utf8_istream_t* stream_;
		utf8_istream_t::sequence_t current_sequence_;
	};

	
	utf8_stream_iterator::utf8_stream_iterator()
	 : stream_()
	{
	}

	utf8_stream_iterator::utf8_stream_iterator(utf8_istream_t& stream)
	 : stream_(&stream)
	{
		current_sequence_ = stream_->get();
	}

	char const* utf8_stream_iterator::begin() const {
		return current_sequence_.first;
	}

	char const* utf8_stream_iterator::end() const {
		return current_sequence_.second;
	}

	unsigned int utf8_stream_iterator::byte_count() const {
		return std::distance(current_sequence_.first, current_sequence_.second);
	}

	utf8_stream_iterator& utf8_stream_iterator::operator ++ ()
	{
		ATMA_ASSERT(stream_);
		//ATMA_ASSERT(!stream_->eof());

		++(*stream_);
		current_sequence_ = stream_->get();

		return *this;
	}

	#if 0
	inline bool operator == (const utf8_stream_iterator& lhs, const utf8_stream_iterator& rhs) {
		return
		  (
		    lhs.stream_ == rhs.stream_ ||
			lhs.stream_ == nullptr && rhs.stream_->eof() ||
			rhs.stream_ == nullptr && lhs.stream_->eof()
		  )
		  &&
		  lhs.byte_count() == rhs.byte_count() &&
		  std::equal(lhs.begin(), lhs.end(), rhs.begin())
		  ;
	}

	inline bool operator != (const utf8_stream_iterator& lhs, const utf8_stream_iterator& rhs) {
		return
		  lhs.stream_ != rhs.stream_ ||
		  lhs.byte_count() != rhs.byte_count() ||
		  !std::equal(lhs.begin(), lhs.end(), rhs.end())
		  ;
	}
	#endif



//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
