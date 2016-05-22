#include <rose/mmap.hpp>

using namespace rose;
using rose::mmap_t;


mmap_t::mmap_t(stdfs::path const& path, access_mask_t am)
	: path_{path}, access_mask_{am}
	, handle_{}, size_{}
{
	if (!stdfs::exists(path_))
		return;

	DWORD file_access = (am & access_flags_t::read ? GENERIC_READ : 0) | (am & access_flags_t::write ? GENERIC_WRITE : 0);
	auto file_handle = CreateFile(path_.c_str(), file_access, 0, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);

	DWORD map_access = am & access_flags_t::write ? PAGE_READWRITE : PAGE_READONLY;
	handle_ = CreateFileMapping(file_handle, nullptr, map_access, 0, 0, 0);

	LARGE_INTEGER i;
	GetFileSizeEx(file_handle, &i);
	size_ = i.QuadPart;

	CloseHandle(file_handle);
}

mmap_t::~mmap_t()
{
	CloseHandle(handle_);
}

