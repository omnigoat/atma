#include <rose/file.hpp>

#include <rose/mmap.hpp>



using namespace rose;
using rose::file_t;
using atma::stream_status_t;

static char const* fa[] = {"rb", "rb", "wb", "rwb"};

file_t::file_t()
	: filename_()
	, access_()
	, filesize_()
{}

file_t::file_t(atma::string const& filename, file_access_mask_t access)
	: filename_(filename)
	, access_(access)
	, filesize_()
	, handle_{fopen(filename.c_str(), fa[(uint)access]), [](FILE* handle) {
		if (handle)
			fclose(handle);
	  }}
{
	if (handle_ == nullptr)
		return;

	// get filesize
	fseek(handle_.get(), 0, SEEK_END);
	filesize_ = ftell(handle_.get());
	fseek(handle_.get(), 0, SEEK_SET);
}

file_t::file_t(file_t&& rhs)
	: filename_{std::move(rhs.filename_)}
	, access_{rhs.access_}
	, filesize_{rhs.filesize_}
	, handle_{std::move(rhs.handle_)}
{
}

auto file_t::operator = (file_t&& rhs) -> file_t&
{
	filename_ = std::move(rhs.filename_);
	access_ = rhs.access_;
	filesize_ = rhs.filesize_;
	handle_.swap(rhs.handle_);

	return *this;
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

auto file_t::read(void* buf, size_t size) -> atma::read_result_t
{
	size_t r = fread(buf, 1, size, handle_.get());

	if (r == size)
		return {stream_status_t::good, r};
	else if (feof(handle_.get()))
		return {stream_status_t::exhausted, r};
	else
		return {stream_status_t::error, r};
}

auto file_t::write(void const* data, size_t size) -> atma::write_result_t
{
	size_t r = fwrite(data, 1, size, handle_.get());

	if (r == size)
		return {stream_status_t::good, r};
	else if (feof(handle_.get()))
		return {stream_status_t::exhausted, r};
	else
		return {stream_status_t::error, r};
}


