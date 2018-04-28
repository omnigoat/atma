#pragma once

#include <atma/atomic.hpp>
#include <atma/math/functions.hpp>

#include <memory_resource>


namespace atma
{
	struct arena_allocator_t : std::pmr::memory_resource
	{
		struct page_t;

		arena_allocator_t(size_t page_size, size_t block_size, std::pmr::memory_resource*);

	protected:
		// implement std::pmr::memory_resource
		auto do_allocate(size_t bytes, size_t alignment) -> void* override;
		auto do_deallocate(void* p, size_t bytes, size_t alignment) -> void override;
		auto do_is_equal(std::pmr::memory_resource const&) const noexcept -> bool override;

		inline auto freemask_size(size_t bytes) -> size_t;

	private:
		std::pmr::memory_resource* upstream_ = nullptr;


		// we split the freemask into uint16s because we must atomically
		// modify them, and there is no such thing as an 8-bit CAS
		size_t const freemask_word_size = 2u;

		static size_t const block_alignment = 16u;

		// if set to zero, it means pages can be of different sizes, and
		// the @size member in page_t is relevant. if non-zero, all pages
		// share the same size, and the @size field is used by various
		// implementations for purposes.
		size_t page_size_ = 0;
		uint32 page_count_ = 0;

		// like above
		size_t block_size_ = 0;
		size_t block_count_ = 0;


		page_t* first_page_ = nullptr;
	};

	struct arena_allocator_t::page_t
	{
		struct emptiness_report_t;

		uint32 const size;
		uint32 const block_count;
		byte* const memory;
		page_t* const next = nullptr;
		uint64 freemask = 0;

		// sentinel-page is going to be null
		auto valid() const -> bool { return memory != nullptr; }
		auto has_space(size_t blocks) const -> emptiness_report_t;
	};

	struct arena_allocator_t::page_t::emptiness_report_t
	{
		// the number of blocks requested
		uint16 requested_blocks = 0;
		// the short in our freemask
		uint16 short_idx = 0;
		// the bit within the short
		uint16 bit_idx = 0;
		// the actual short that was in our freemask
		uint16 freemask_short = 0;

		inline static emptiness_report_t const empty;

		auto new_freemask_short() const -> uint16 { return freemask_short | (((1 << requested_blocks) - 1) << bit_idx); }
		auto block_idx() const { return short_idx * 16 + bit_idx; }

		operator bool() const { return requested_blocks != 0; }
	};

	inline auto arena_allocator_t::page_t::has_space(size_t required_blocks) const -> emptiness_report_t
	{
		uint16 bit_idx = 0;
		uint16 mask = uint16(1 << required_blocks) - 1;
		uint16 freemask_short = 0;

		for (uint16 short_idx = 0; memory && bit_idx != block_count - required_blocks; ++bit_idx, short_idx = bit_idx / 16)
		{
			uint64 shifted_mask = mask << bit_idx;

			freemask_short = reinterpret_cast<uint16 const*>(&freemask)[short_idx];

			if (((shifted_mask | freemask) ^ freemask) == shifted_mask)
			{
				// get short idx
				uint16 short_idx = bit_idx / 16u;
				bit_idx = bit_idx % 16;

				return {(uint16)required_blocks, short_idx, bit_idx, freemask_short};
			}
		}

		return emptiness_report_t::empty;
	}

	inline arena_allocator_t::arena_allocator_t(size_t page_size, size_t block_size, std::pmr::memory_resource* upstream)
		: upstream_{upstream}
		, page_size_{page_size}
		, block_size_{block_size}
		, block_count_{page_size_ / block_size_}
		, first_page_{(page_t*)upstream_->allocate(sizeof(page_t))}
	{
		ATMA_ASSERT(page_size % block_size == 0, "block-size must fit nicely into page-size");

		// default-construct sentinel page
		new (first_page_) page_t{};
	}

	

	inline auto arena_allocator_t::do_allocate(size_t bytes, size_t alignment) -> void*
	{
		ATMA_ASSERT(alignment <= 16, "alignment requirement bigger than 16 bytes. that's really big... the biggest I've seen. and I've been with a lot of guys");

		// tricky maths: ceil(bytes / block_size_)
		size_t required_blocks = (bytes + block_size_ - 1) / block_size_;
		//if (required_blocks > )

	get_page:
		page_t* page = atma::atomic_load(&first_page_);
		page_t::emptiness_report_t report;

		// go through valid pages first
		for (; page->valid(); page = atma::atomic_load(&page->next))
		{
			if (report = page->has_space(required_blocks))
				goto get_page_bit_idx;
		}

		// we've already allocated the maximum number of pages, so we simply have to fail
		if (atma::atomic_post_increment(&page_count_) != 0)
			return nullptr;

		// total-allocation of a page's worth of memory is the size of the freemask
		size_t total_alloc_size = freemask_size(page_size_ / block_size_);
		// align it to the block-alignment
		total_alloc_size = aml::alignby(total_alloc_size, block_alignment);
		// and add the page-size onto that
		total_alloc_size += page_size_;

		// allocate new page
		page_t* new_page = (page_t*)upstream_->allocate(sizeof(page_t));
		// construct new page with new memory
		new (new_page) page_t{(uint32)page_size_, (uint32)block_size_, (byte*)upstream_->allocate(total_alloc_size, block_alignment), page};
		// fill freemask with zeroes
		std::fill(new_page->memory, new_page->memory + freemask_size(page_size_), byte{});

		// if we swapped, we're all good, we have a usable page. assume it's empty enough.
		if (atma::atomic_compare_exchange(&first_page_, page, new_page, &page))
		{
			page = new_page;
			if (report = page->has_space(required_blocks); !report)
				goto get_page;
		}
		// else, someone got in our way, and we need to try again afresh
		else
		{
			upstream_->deallocate(new_page->memory, page_size_);
			upstream_->deallocate(new_page, sizeof(page_t));
			goto get_page;
		}
		

	get_page_bit_idx:
		ATMA_ASSERT(report);

		// now atomically set the new value of the byte for the page's freemask
		uint16 new_freemask_short = report.new_freemask_short();
		if (!atma::atomic_compare_exchange(reinterpret_cast<uint16*>(&page->freemask) + report.short_idx, report.freemask_short, new_freemask_short))
			goto get_page;

		// now we have exclusive access to the @length blocks of memory
		byte* block = page->memory + report.block_idx() * block_size_;
		
		return block;
	}

	inline auto arena_allocator_t::do_deallocate(void* p, size_t bytes, size_t alignment) -> void {}
	inline auto arena_allocator_t::do_is_equal(std::pmr::memory_resource const&) const noexcept -> bool { return false; }

	inline auto arena_allocator_t::freemask_size(size_t bytes) -> size_t
	{
		return aml::alignby((bytes + 7) / 8, freemask_word_size);
	}

}
