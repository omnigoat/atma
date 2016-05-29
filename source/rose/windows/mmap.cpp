#include <rose/mmap.hpp>

using namespace rose;
using rose::mmap_t;


mmap_t::mmap_t(stdfs::path const& path, file_access_mask_t fam)
	: path_{path}, access_mask_{fam}
	, handle_{}, size_{}
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

mmap_t::~mmap_t()
{
	CloseHandle(handle_);
}

