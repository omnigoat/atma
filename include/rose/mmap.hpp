#pragma once

#include <rose/rose_fwd.hpp>

#include <atma/config/platform.hpp>
#include <atma/assert.hpp>
#include <atma/intrusive_ptr.hpp>

#include <filesystem>


namespace stdfs = std::experimental::filesystem;

namespace rose
{
	struct mmap_t : atma::ref_counted
	{
#if ATMA_PLATFORM_WINDOWS
		using handle_t = HANDLE;
#endif

		mmap_t(stdfs::path const&, file_access_mask_t = file_access_t::read);
		~mmap_t();

		auto valid() const -> bool;
		auto handle() const -> handle_t;
		auto size() const -> size_t;
		auto access_mask() const -> file_access_mask_t;

	private:
		stdfs::path path_;
		file_access_mask_t access_mask_;

		handle_t handle_;
		size_t size_;

		friend struct mmap_bytestream_t;
	};

	using mmap_ptr = atma::intrusive_ptr<mmap_t>;

	inline auto mmap_t::valid() const -> bool
	{
		return handle_ != nullptr;
	}

	inline auto mmap_t::handle() const -> handle_t
	{
		return handle_;
	}

	inline auto mmap_t::size() const -> size_t
	{
		return size_;
	}

	inline auto mmap_t::access_mask() const -> file_access_mask_t
	{
		return access_mask_;
	}
}

