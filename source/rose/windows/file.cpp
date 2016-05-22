#include <rose/file.hpp>

#include <rose/mmap.hpp>

#include <atma/algorithm.hpp>

using namespace rose;
using rose::file_t;

namespace
{
	void close_file_handle(FILE* handle)
	{
		if (handle)
			fclose(handle);
	}
}

file_t::file_t()
	: filename_()
	, access_()
	, filesize_()
{}

file_t::file_t(atma::string const& filename, file_access_t access)
	: filename_(filename)
	, access_(access)
	, filesize_()
{
	char const* fa[] = {"r", "w", "r+"};
	handle_.reset(fopen(filename.c_str(), fa[(uint)access]), &close_file_handle);
	if (handle_ == nullptr)
		return;

	// get filesize
	fseek(handle_.get(), 0, SEEK_END);
	filesize_ = ftell(handle_.get());
	fseek(handle_.get(), 0, SEEK_SET);
}

auto file_t::valid() const -> bool
{
	return handle_ != nullptr;
}

auto file_t::size() const -> size_t
{
	return filesize_;
}

auto file_t::position() const -> size_t
{
	return ftell(handle_.get());
}

auto file_t::seek(size_t x) -> stream_status_t
{
	auto r = fseek(handle_.get(), (long)x, SEEK_SET);
	if (r == 0)
		return stream_status_t::good;
	else
		return stream_status_t::error;
}

auto file_t::move(int64 x) -> stream_status_t
{
	auto r = fseek(handle_.get(), (long)x, SEEK_CUR);
	if (r == 0)
		return stream_status_t::good;
	else
		return stream_status_t::error;
}

auto file_t::read(void* buf, size_t size) -> read_result_t
{
	size_t r = fread(buf, 1, size, handle_.get());

	if (r == size)
		return {stream_status_t::good, r};
	else if (feof(handle_.get()))
		return {stream_status_t::eof, r};
	else
		return {stream_status_t::error, r};
}

auto file_t::write(void const* data, size_t size) -> write_result_t
{
	size_t r = fwrite(data, 1, size, handle_.get());

	if (r == size)
		return {stream_status_t::good, r};
	else if (feof(handle_.get()))
		return {stream_status_t::eof, r};
	else
		return {stream_status_t::error, r};
}

// absract-stream
auto file_t::stream_opers() const -> stream_opers_mask_t
{
	return stream_opers_mask_t{stream_opers_t::read, stream_opers_t::write, stream_opers_t::random_access};
}

// input-stream
auto file_t::g_size() const -> size_t
{
	return filesize_;
}

auto file_t::g_seek(size_t x) -> stream_status_t
{
	return seek(x);
}

auto file_t::g_move(int64 x) -> stream_status_t
{
	return move(x);
}

// output-stream
auto file_t::p_size() const -> size_t
{
	return filesize_;
}

auto file_t::p_seek(size_t x) -> stream_status_t
{
	return seek(x);
}

auto file_t::p_move(int64 x) -> stream_status_t
{
	return move(x);
}
