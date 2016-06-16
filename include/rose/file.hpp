#pragma once

#include <rose/rose_fwd.hpp>

#include <atma/streams.hpp>
#include <atma/string.hpp>
#include <atma/unique_memory.hpp>

#include <memory>


namespace rose
{
	struct file_t
		: atma::random_access_input_bytestream_t
		, atma::random_access_output_bytestream_t
	{
#if ATMA_PLATFORM_WINDOWS
		using handle_t = std::shared_ptr<FILE>;
#endif

		file_t();
		file_t(atma::string const&, file_access_mask_t = file_access_t::read);

		auto valid() const -> bool;
		auto size() const -> size_t;
		auto position() const -> size_t;

		auto seek(size_t) -> atma::stream_status_t;
		auto move(int64) -> atma::stream_status_t;

		// abstract-stream
		auto stream_opers() const -> atma::stream_opers_mask_t override;

		// input-stream
		auto read(void*, size_t) -> atma::read_result_t override;

		// output-stream
		auto write(void const*, size_t) -> atma::write_result_t override;


	private:
		// random-access-input-stream
		auto g_size() const -> size_t override;
		auto g_seek(size_t) -> atma::stream_status_t override;
		auto g_move(int64) -> atma::stream_status_t override;

		// random-access-output-stream
		auto p_size() const -> size_t override;
		auto p_seek(size_t) -> atma::stream_status_t override;
		auto p_move(int64) -> atma::stream_status_t override;

	private:
		atma::string filename_;
		file_access_mask_t access_;

		handle_t handle_;
		size_t filesize_;
	};


	inline auto file_t::valid() const -> bool
	{
		return handle_ != nullptr;
	}

	inline auto file_t::size() const -> size_t
	{
		return filesize_;
	}

	inline auto file_t::position() const -> size_t
	{
		return ftell(handle_.get());
	}


	inline auto read_into_memory(file_t& file) -> atma::unique_memory_t
	{
		atma::unique_memory_t memory{file.size()};
		file.read(memory.begin(), file.size());
		return memory;
	}

	template <size_t Bufsize, typename FN>
	inline auto for_each_line(atma::input_bytestream_t& stream, size_t maxsize, FN&& fn) -> void
	{
		char buf[Bufsize];
		atma::string line;

		// read bytes into line until newline is found
		auto rr = atma::read_result_t{atma::stream_status_t::good, 0};
		while (rr.status == atma::stream_status_t::good)
		{
			rr = stream.read(buf, Bufsize);
			auto bufend = buf + rr.bytes_read;
			auto bufp = buf;

			while (bufp != bufend)
			{
				auto newline = std::find(bufp, bufend, '\n');
				line.append(bufp, newline);

				if (newline != bufend) {
					fn(line.raw_begin(), line.raw_size());
					line.clear();
				}

				bufp = (newline == bufend) ? bufend : newline + 1;
			}
		}
	}
}
