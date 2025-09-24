#pragma once

#include <atma/atomic.hpp>
#include <atma/math/functions.hpp>

#include <memory_resource>
#include <atomic>



namespace atma
{
	// performs the equivalent of ceil(x / y) for integrals
	template <typename T>
	auto ceil_div(T x, T y)
	{
		return (x + y - 1) / y;
	}

	struct arena_memory_resource_t : std::pmr::memory_resource
	{
		struct page_t;

		arena_memory_resource_t(
			size_t block_size, size_t block_count, size_t page_count = ~0,
			// memory-resource for allocating the actual pages of memory
			std::pmr::memory_resource* = std::pmr::new_delete_resource(),
			// memory-resource for allocating the page-control-structure
			std::pmr::memory_resource* = std::pmr::new_delete_resource());

		arena_memory_resource_t(arena_memory_resource_t const&) = delete;
		arena_memory_resource_t(arena_memory_resource_t&&) = delete;

		auto upstream_resource() const { return page_upstream_; }
		auto control_upstream_resource() const { return page_control_upstream_.resource(); }

	protected:
		// implement std::pmr::memory_resource
		auto do_allocate(size_t bytes, size_t alignment) -> void* override;
		auto do_deallocate(void* p, size_t bytes, size_t alignment) -> void override;
		auto do_is_equal(std::pmr::memory_resource const&) const noexcept -> bool override;

	private:
		std::pmr::memory_resource* page_upstream_;
		std::pmr::polymorphic_allocator<page_t> page_control_upstream_;

		// we split the freemask into uint16s because we must atomically
		// modify them, and there is no such thing as an 8-bit CAS
		static inline size_t const freemask_word_size = 2u;
		static inline size_t const block_alignment = 16u;

		size_t block_size_ = 0;
		size_t block_count_ = 0;
		size_t max_pages_ = 0;
		size_t page_count_ = 0;

		std::atomic<page_t*> first_page_;
	};

	struct arena_memory_resource_t::page_t
	{
		struct emptiness_report_t;

		page_t(byte* memory, page_t* next = nullptr)
			: memory{memory}
			, next{next}
		{}

		byte* const memory;
		std::atomic<page_t*> const next;
		uint64 freemask = 0;

		// sentinel-page is going to be null
		auto valid() const -> bool { return memory != nullptr; }
		auto has_space(size_t total_blocks, size_t required_blocks) const -> emptiness_report_t;
	};

	struct arena_memory_resource_t::page_t::emptiness_report_t
	{
		// the number of blocks requested
		uint16 requested_blocks = 0;
		// the short in our freemask
		uint16 short_idx = 0;
		// the bit within the short
		uint16 bit_idx = 0;
		// the actual short that was in our freemask
		uint16 freemask_short = 0;

		static emptiness_report_t const empty;

		auto new_freemask_short() const -> uint16 { return freemask_short | (((1 << requested_blocks) - 1) << bit_idx); }
		auto block_idx() const { return short_idx * 16 + bit_idx; }

		operator bool() const { return requested_blocks != 0; }
	};

	inline constexpr arena_memory_resource_t::page_t::emptiness_report_t const arena_memory_resource_t::page_t::emptiness_report_t::empty;

	inline auto arena_memory_resource_t::page_t::has_space(size_t block_count, size_t required_blocks) const -> emptiness_report_t
	{
		uint16 bit_idx = 0;
		uint16 mask = uint16(1 << required_blocks) - 1;
		uint16 freemask_short = 0;

		for (uint16 short_idx = 0; memory && bit_idx <= block_count - required_blocks; ++bit_idx, short_idx = bit_idx / 16)
		{
			uint64 shifted_mask = mask << bit_idx;

			freemask_short = reinterpret_cast<uint16 const*>(&freemask)[short_idx];

			if (((shifted_mask | freemask) ^ freemask) == shifted_mask)
			{
				// get short idx
				short_idx = bit_idx / 16u;
				bit_idx = bit_idx % 16;

				return {(uint16)required_blocks, short_idx, bit_idx, freemask_short};
			}
		}

		return emptiness_report_t::empty;
	}

	inline arena_memory_resource_t::arena_memory_resource_t
		( size_t block_size
		, size_t block_count
		, size_t page_count
		, std::pmr::memory_resource* page_upstream_resource
		, std::pmr::memory_resource* page_control_upstream_resource
		)
		: page_upstream_{page_upstream_resource}
		, page_control_upstream_{page_control_upstream_resource}
		, block_size_{block_size}
		, block_count_{block_count}
		, max_pages_{page_count}
	{
		ATMA_ASSERT(page_upstream_resource, "invalid upstream resource");
		ATMA_ASSERT(block_size_ >= 16, "minimum size is 16 bytes per block");

		// default-construct sentinel page
		first_page_ = page_control_upstream_.allocate(1);
		page_control_upstream_.construct(first_page_.load(), nullptr);
	}

	inline auto arena_memory_resource_t::do_allocate(size_t bytes, size_t alignment) -> void*
	{
		ATMA_ASSERT(alignment <= 16, "alignment requirement bigger than 16 bytes. that's really big... the biggest I've seen. and I've been with a lot of guys");

		size_t required_blocks = ceil_div(bytes, block_size_);
		page_t* new_page = nullptr;
		int attempts = 0;

	get_page:
		// the heuristic is five
		if (attempts++ == 5)
			return nullptr;

		page_t* page = first_page_;
		page_t::emptiness_report_t report;

		// go through valid pages first
		for (; page->valid(); page = page->next)
		{
			if ((report = page->has_space(block_count_, required_blocks)))
				goto get_page_bit_idx;
		}

		// we've already allocated the maximum number of pages
		if (atma::atomic_post_increment(&page_count_) >= max_pages_)
			goto get_page;

		// allocate new page & memory
		{
			new_page = page_control_upstream_.allocate(1);
			auto new_page_memory = (byte*)page_upstream_->allocate(block_size_ * block_count_, block_alignment);

			// construct new page with new memory
			page_control_upstream_.construct(new_page, new_page_memory, page);
		}

		// if we swapped, we're all good, we have a usable page. assume it's empty enough.
		if (first_page_.compare_exchange_strong(page, new_page))
		{
			page = new_page;
			if (!(report = page->has_space(block_count_, required_blocks)))
				goto get_page;
		}
		// else, someone got in our way, and we need to try again afresh
		else
		{
			page_upstream_->deallocate(new_page->memory, block_size_ * block_count_);
			page_control_upstream_.deallocate(new_page, 1);
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

	inline auto arena_memory_resource_t::do_deallocate(void* ptr, size_t size, size_t alignment) -> void
	{
		// find the page
		page_t* current_page = first_page_;
		for (; current_page; current_page = current_page->next)
		{
			if (current_page->memory < ptr && ptr < (current_page->memory + block_count_ * block_size_))
				break;
		}

		// oh no
		if (!current_page)
			return;

		size_t block_idx = ((byte*)ptr - current_page->memory) / block_size_;
		size_t block_length = ceil_div(size, block_size_);

		// this is the mask of the blocks we're using
		uint64 mask = ((1 << block_length) - 1) << block_idx;

		// atomically flip it to off
		uint64 old_mask = current_page->freemask;
		while (!atomic_compare_exchange(&current_page->freemask, old_mask, old_mask & ~mask, &old_mask))
			ATMA_ASSERT((old_mask & mask) == mask);
	}

	inline auto arena_memory_resource_t::do_is_equal(std::pmr::memory_resource const&) const noexcept -> bool
	{
		return false;
	}

}


namespace atma
{
	template <typename T>
	struct arena_allocator_t
	{
		using value_type = T;

		arena_allocator_t()
			: resource_(new arena_memory_resource_t(512, 1024))
		{}

		template <typename Y>
		arena_allocator_t(arena_allocator_t<Y> const& rhs)
			: resource_(rhs.resource_)
		{}

		[[nodiscard]] value_type* allocate(std::size_t n)
		{
			return (value_type*)resource_->allocate(n * sizeof(value_type));
		}

		void deallocate(value_type* p, std::size_t sz)
		{
			resource_->deallocate(p, sz);
		}

		bool operator == (arena_allocator_t const& rhs) const
		{
			return resource_ == rhs.resource_;
		}

	private:
		std::shared_ptr<std::pmr::memory_resource> resource_;

		template <typename> friend struct arena_allocator_t;
	};
}
