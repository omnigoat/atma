#include <rose/mmap.hpp>

using namespace rose;
using rose::mmap_t;


mmap_t::mmap_t(stdfs::path const& path, file_access_mask_t fam)
	: path_{path}, access_mask_{fam}
	, handle_{INVALID_HANDLE_VALUE}, size_{}
{
	if (!stdfs::exists(path_))
		return;

	DWORD file_access = (fam & file_access_t::read ? GENERIC_READ : 0) | (fam & file_access_t::write ? GENERIC_WRITE : 0);
	auto file_handle = CreateFile(path_.c_str(), file_access, 0, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);

	DWORD map_access = fam & file_access_t::write ? PAGE_READWRITE : PAGE_READONLY;
	handle_ = CreateFileMapping(file_handle, nullptr, map_access, 0, 0, 0);

	LARGE_INTEGER i;
	GetFileSizeEx(file_handle, &i);
	size_ = i.QuadPart;

	CloseHandle(file_handle);
}

mmap_t::mmap_t(mmap_t&& rhs)
	: path_{std::move(rhs.path_)}
	, access_mask_{rhs.access_mask_}
	, handle_{std::move(rhs.handle_)}
	, size_{rhs.size_}
{
	rhs.handle_ = INVALID_HANDLE_VALUE;
}

mmap_t::~mmap_t()
{
	if (handle_ != INVALID_HANDLE_VALUE)
		CloseHandle(handle_);
}

auto mmap_t::operator = (mmap_t&& rhs) -> mmap_t&
{
	std::swap(path_, rhs.path_);
	std::swap(access_mask_, rhs.access_mask_);
	std::swap(handle_, rhs.handle_);
	std::swap(size_, rhs.size_);

	return *this;
}

auto mmap_t::valid() const -> bool
{
	return handle_ != INVALID_HANDLE_VALUE;
}