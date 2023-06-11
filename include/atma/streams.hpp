#pragma once

#include <atma/bitmask.hpp>
#include <atma/event.hpp>

#include <algorithm>

import atma.types;
import atma.memory;
import atma.intrusive_ptr;

namespace atma
{
	enum class stream_status_t
	{
		good,
		exhausted,
		error,
	};

	enum class stream_opers_t
	{
		read,
		write,
		random_access,
	};

	ATMA_BITMASK(stream_opers_mask_t, stream_opers_t);

	struct read_result_t
	{
		read_result_t() : status(), bytes_read() {}
		read_result_t(stream_status_t s, size_t b) : status(s), bytes_read(b) {}

		stream_status_t status;
		size_t bytes_read;
	};

	struct write_result_t
	{
		stream_status_t status;
		size_t bytes_written;
	};

	struct stream_t
		: atma::ref_counted
	{
		virtual auto stream_status() const -> stream_status_t = 0;
		virtual auto stream_opers() const -> stream_opers_mask_t = 0;
	};

	struct input_bytestream_t
		: virtual stream_t
	{
		virtual auto read(void*, size_t) -> read_result_t = 0;
	};

	struct output_bytestream_t
		: virtual stream_t
	{
		virtual auto write(void const*, size_t) -> write_result_t = 0;
	};

	struct random_access_input_bytestream_t
		: input_bytestream_t
	{
		virtual auto g_size() const -> size_t = 0;
		virtual auto g_seek(size_t) -> stream_status_t = 0;
		virtual auto g_move(int64) -> stream_status_t = 0;
	};

	struct random_access_output_bytestream_t
		: output_bytestream_t
	{
		virtual auto p_size() const -> size_t = 0;
		virtual auto p_seek(size_t) -> stream_status_t = 0;
		virtual auto p_move(int64) -> stream_status_t = 0;
	};

	using stream_ptr = atma::intrusive_ptr<stream_t>;
	using input_bytestream_ptr = atma::intrusive_ptr<input_bytestream_t>;
	using output_bytestream_ptr = atma::intrusive_ptr<output_bytestream_t>;
	using random_access_input_bytestream_ptr = atma::intrusive_ptr<random_access_input_bytestream_t>;
	using random_access_output_bytestream_ptr = atma::intrusive_ptr<random_access_output_bytestream_t>;

	template <typename T, typename Y>
	inline auto stream_cast(atma::intrusive_ptr<Y> const& stream) -> atma::intrusive_ptr<T>
	{
		return atma::polymorphic_cast<T>(stream);
	}
}


// memory-stream
namespace atma
{
	struct memory_bytestream_t
		: random_access_input_bytestream_t
		, random_access_output_bytestream_t
	{
		memory_bytestream_t();
		memory_bytestream_t(void* data, size_t size);

		// abstract-stream
		auto stream_status() const -> stream_status_t override;
		auto stream_opers() const -> stream_opers_mask_t override;

		// input-stream
		auto read(void*, size_t) -> read_result_t override;

		// output-stream
		auto write(void const*, size_t) -> write_result_t override;

		auto size() const -> size_t;
		auto position() const -> size_t;
		auto seek(size_t) -> stream_status_t;
		auto move(int64) -> stream_status_t;

	protected:
		auto memory_stream_reset(void*, size_t size) -> void;

	private:
		// random-access-input-stream
		auto g_size() const -> size_t override;
		auto g_seek(size_t) -> stream_status_t override;
		auto g_move(int64) -> stream_status_t override;

		// random-access-output-stream
		auto p_size() const -> size_t override;
		auto p_seek(size_t) -> stream_status_t override;
		auto p_move(int64) -> stream_status_t override;

	private:
		byte* data_;
		size_t position_;
		size_t size_;
	};

	inline memory_bytestream_t::memory_bytestream_t()
		: data_()
		, size_()
		, position_()
	{}

	inline memory_bytestream_t::memory_bytestream_t(void* data, size_t size)
		: data_(reinterpret_cast<byte*>(data))
		, size_(size)
		, position_()
	{}

	inline auto memory_bytestream_t::size() const -> size_t
	{
		return size_;
	}

	inline auto memory_bytestream_t::position() const -> size_t
	{
		return position_;
	}

	inline auto memory_bytestream_t::seek(size_t x) -> stream_status_t
	{
		if (x < size_) {
			position_ = x;
			return stream_status_t::good;
		}
		else {
			return stream_status_t::error;
		}
	}

	inline auto memory_bytestream_t::move(int64 x) -> stream_status_t
	{
		if (position_ + x < size_) {
			position_ += x;
			return stream_status_t::good;
		}
		else {
			return stream_status_t::error;
		}
	}

	inline auto memory_bytestream_t::read(void* buf, size_t size) -> read_result_t
	{
		size_t r = std::min(size, size_ - position_);
		memcpy(buf, data_ + position_, r);

		if (r == size)
			return{stream_status_t::good, r};
		else
			return{stream_status_t::exhausted, r};
	}

	inline auto memory_bytestream_t::write(void const* data, size_t size) -> write_result_t
	{
		size_t r = std::min(size, size_ - position_);
		memcpy(data_ + position_, data, r);
		position_ += r;

		if (r == size)
			return{stream_status_t::good, r};
		else
			return{stream_status_t::exhausted, r};
	}

	// absract-stream
	inline auto memory_bytestream_t::stream_status() const -> stream_status_t
	{
		if (data_ == nullptr || position_ > size_)
			return stream_status_t::error;
		else if (position_ == size_)
			return stream_status_t::exhausted;
		else
			return stream_status_t::good;
	}

	inline auto memory_bytestream_t::stream_opers() const -> stream_opers_mask_t
	{
		return stream_opers_t::read | stream_opers_t::write | stream_opers_t::random_access;
	}

	// input-stream
	inline auto memory_bytestream_t::g_size() const -> size_t
	{
		return size();
	}

	inline 	auto memory_bytestream_t::g_seek(size_t x) -> stream_status_t
	{
		return seek(x);
	}

	inline auto memory_bytestream_t::g_move(int64 x) -> stream_status_t
	{
		return move(x);
	}

	// output-stream
	inline auto memory_bytestream_t::p_size() const -> size_t
	{
		return size();
	}

	inline auto memory_bytestream_t::p_seek(size_t x) -> stream_status_t
	{
		return seek(x);
	}

	inline auto memory_bytestream_t::p_move(int64 x) -> stream_status_t
	{
		return move(x);
	}

	inline auto memory_bytestream_t::memory_stream_reset(void* data, size_t size) -> void
	{
		data_ = reinterpret_cast<byte*>(data);
		size_ = size;
		position_ = 0;
	}
}

namespace atma
{
	template <typename... Args>
	struct reactive_stream_t : stream_t
	{
		auto subscribe(atma::function<void(Args...)> const&) -> void;

	protected:
		event_t<Args...> event_;
	};

	template <typename T>
	struct generator_stream_t : reactive_stream_t<void(T const&)>
	{
		generator_stream_t() = default;
		generator_stream_t(generator_stream_t const&) = delete;

		auto generate(T t) -> void;

	};

}


