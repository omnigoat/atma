#pragma once

#include <atma/config/platform.hpp>
#include <atma/assert.hpp>
#include <atma/intrusive_ptr.hpp>
#include <atma/bitmask.hpp>

#include <filesystem>


namespace stdfs = std::experimental::filesystem;

namespace rose
{
	enum class access_flags_t
	{
		read,
		write,
	};

	ATMA_BITMASK(access_mask_t, access_flags_t);

	struct mmap_t : atma::ref_counted
	{
#if ATMA_PLATFORM_WINDOWS
		using handle_t = HANDLE;
#endif

		mmap_t(stdfs::path const&, access_mask_t = access_flags_t::read);
		~mmap_t();

		auto valid() const -> bool;
		auto handle() const -> handle_t;
		auto size() const -> size_t;

	private:
		stdfs::path path_;
		access_mask_t access_mask_;

		handle_t handle_;
		size_t size_;

		friend struct mmap_stream_t;
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
}

