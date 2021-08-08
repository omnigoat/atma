#pragma once

#include <iostream>
#include <atomic>
#include <memory>

import atma.types;

namespace atma { namespace math { namespace ct {

	constexpr uint32 exp2(uint32 x)
	{
		return x == 0 ? 1 : x == 1 ? 2 : 2 * exp2(x - 1);
	}

	constexpr uint32 max(uint32 a, uint32 b)
	{
		return a < b ? b : a;
	}

}}}

namespace atma
{
	template <typename Payload>
	struct handle_table_t
	{
		using handle_t = uint32;

		handle_table_t();

		// construct
		template <typename... Args>
		auto construct(Args&&...) -> handle_t;

		// ref-counting
		auto retain(handle_t) -> void;
		auto release(handle_t) -> bool;
		auto expired(handle_t) const -> bool;

		// weak ref-counting
		auto weak_retain(handle_t) -> void;
		auto weak_release(handle_t) -> void;

		auto get(handle_t) -> Payload*;
		auto get(handle_t) const -> Payload const*;

		auto swap(handle_t, handle_t) -> void;

		auto dump_ascii() -> void;


	private: // internal math constants
		static const uint32 genr_bits = 8;
		static const uint32 slot_bits = 12;
		static const uint32 page_bits = 12;
		static_assert(genr_bits + slot_bits + page_bits == 32, "bad bits");

		static const uint32 slot_byte_bits = 7;
		static const uint32 slot_bit_bits = 5;
		static_assert(slot_bit_bits + slot_byte_bits == slot_bits, "bad bits");

		static const uint32 genr_max = atma::math::ct::exp2(genr_bits);
		static const uint32 slot_max = atma::math::ct::exp2(slot_bits);
		static const uint32 page_max = atma::math::ct::exp2(page_bits);

		static const uint32 genr_mask = (genr_max - 1) << slot_bits << page_bits;
		static const uint32 slot_mask = (slot_max - 1) << page_bits;
		static const uint32 page_mask = (page_max - 1);
		static_assert((genr_mask | slot_mask | page_mask) == 0xffffffff, "bad bits");

		static const uint32 slot_byte_mask = (math::ct::exp2(slot_byte_bits) - 1) << slot_bit_bits << page_bits;
		static const uint32 slot_bit_mask = (math::ct::exp2(slot_bit_bits) - 1) << page_bits;;

		static uint32 extract_genr_idx(handle_t h) { return (h & genr_mask) >> slot_bits >> page_bits; }
		static uint32 extract_slot_idx(handle_t h) { return (h & slot_mask) >> page_bits; }
		static uint32 extract_slot_byte_idx(handle_t h) { return (h & slot_byte_mask) >> slot_bit_bits >> page_bits; }
		static uint32 extract_slot_bit_idx(handle_t h) { return (h & slot_bit_mask) >> page_bits; }
		static uint32 extract_page_idx(handle_t h) { return h & page_mask; }

		static auto construct_handle(uint32 slot_idx, uint32 page_idx) -> handle_t
			{ return (slot_idx << page_bits) | page_idx; }

	private: // table management
		struct slot_t;
		struct page_t;

		auto weak_release_impl(page_t*, slot_t*, handle_t) -> void;

		auto lookup_handle(handle_t) -> std::pair<page_t*, slot_t*>;
		auto lookup_handle(handle_t) const -> std::pair<page_t const*, slot_t const*>;

		page_t* pages_[page_max] = {};
		page_t* first_page_ = nullptr;
		uint32 pages_size_ = 0;
	};

	template <typename P>
	struct handle_table_t<P>::slot_t
	{
		template <typename... Args>
		slot_t(Args&&... args)
			: ref_count{1}
			, wref_count{1}
			, prev_slot{}
			, prev_page{}
			, payload{std::forward<Args>(args)...}
		{}

		std::atomic<uint32> ref_count;
		std::atomic<uint32> wref_count;
		P payload;
		uint32 prev_slot : 12;
		uint32 prev_page : 12;
	};

	template <typename T>
	struct handle_table_t<T>::page_t
	{
		page_t(uint32 id)
			: id{id}
			, memory{slot_max}
			, freefield{}
		{}

		uint32 id = 0;
		uint32 size = 0;
		alignas(8) page_t* next = nullptr;
		atma::memory_t<slot_t> memory;
		uint32 freefield[math::ct::max(1, slot_max / 32)];
	};





	template <typename T>
	inline handle_table_t<T>::handle_table_t()
	{}

	template <typename T>
	template <typename... Args>
	inline auto handle_table_t<T>::construct(Args&&... args) -> handle_t
	{
	pages_begin:
		page_t* p = first_page_;
		page_t** pp = &first_page_;

		while (p != nullptr)
		{
			// wait for in-progress page to be published or not
			if (p->id == page_max)
				goto pages_begin;

			if (p->size < slot_max)
				break;

			pp = &p->next;
			p = p->next;
		}

		if (p == nullptr)
		{
			// allocate to pp
			if (pages_size_ < page_max)
			{
				auto pidx = atma::atomic_post_increment(&pages_size_);

				auto np = new page_t{pidx};

				if (pidx < page_max)
				{
					// use up the first slot so that page/slot (0, 0) can be used as invalid
					if (pidx == 0)
						np->freefield[0] |= 0x80000000;

					// publish
					if (atma::atomic_compare_exchange<page_t*>(pages_ + pidx, nullptr, np))
					{
						if (atma::atomic_compare_exchange<page_t*>(pp, p, np))  {
							p = np;
							goto slots_begin;
						}
						else {
							ATMA_ENSURE(atma::atomic_compare_exchange(pages_ + pidx, np, nullptr));
						}
					}
				}
				
				delete np;
			}

			goto pages_begin;
		}

		// with supposedly not-full page
	slots_begin:
		uint32 idx = slot_max;
		for (auto i = 0u, ie = std::max(1u, slot_max / 32); i != ie; ++i)
		{
			uint32 u32 = p->freefield[i];
			if (u32 == 0xffffffff)
				continue;

		freeslot_u32_retry:
			uint32 m = 0x80000000;
			for (auto j = 0; j != 32; ++j, m >>= 1)
			{
				uint32 nu32 = u32 | m;
				if (nu32 != u32)
				{
					if (atma::atomic_compare_exchange(&p->freefield[i], u32, nu32, &u32))
					{
						atma::atomic_pre_increment(&p->size);
						idx = (i << slot_bit_bits) | j;

						// construct now-owned slot
						p->memory.construct(idx, std::forward<Args>(args)...);
						goto freeslot_end;
					}

					goto freeslot_u32_retry;
				}
				else
				{
					continue;
				}
			}
		}

	freeslot_end:
		if (idx == slot_max)
			goto pages_begin;

		return construct_handle(idx, p->id);
	}

	template <typename T>
	inline auto handle_table_t<T>::retain(handle_t h) -> void
	{
		auto ps = lookup_handle(h);
		if (ps.first && ps.second)
		{
			++ps.second->ref_count;
		}
	}

	template <typename T>
	inline auto handle_table_t<T>::release(handle_t h) -> bool
	{
		auto [page, slot] = lookup_handle(h);
		if (page && slot)
		{
			ATMA_ASSERT(slot->ref_count > 0, "bad ref counts");

			if (--slot->ref_count == 0)
			{
				slot->payload.~T();
				weak_release_impl(page, slot, h);
				return true;
			}
		}

		return false;
	}

	template <typename T>
	inline auto handle_table_t<T>::expired(handle_t h) const -> bool
	{
		auto ps = lookup_handle(h);
		if (ps.first && ps.second)
		{
			return ps.second->ref_count == 0;
		}

		return true;
	}

	template <typename T>
	inline auto handle_table_t<T>::weak_retain(handle_t h) -> void
	{
		auto ps = lookup_handle(h);
		if (ps.first && ps.second)
		{
			++ps.second->wref_count;
		}
	}

	template <typename T>
	inline auto handle_table_t<T>::weak_release(handle_t h) -> void
	{
		auto ps = lookup_handle(h);
		if (ps.first && ps.second)
		{
			weak_release_impl(ps.first, ps.second, h);
		}
	}

	template <typename T>
	inline auto handle_table_t<T>::get(handle_t h) -> T*
	{
		auto ps = lookup_handle(h);
		if (ps.first && ps.second)
		{
			return &ps.second->payload;
		}

		return nullptr;
	}

	template <typename T>
	inline auto handle_table_t<T>::get(handle_t h) const -> T const*
	{
		auto ps = lookup_handle(h);
		if (ps.first && ps.second)
		{
			return &ps.second->payload;
		}

		return nullptr;
	}

	template <typename T>
	inline auto handle_table_t<T>::swap(handle_t a, handle_t b) -> void
	{
		auto as = lookup_handle(a);
		auto bs = lookup_handle(b);

		if (as.first && as.second && bs.first && bs.second)
		{
			using std::swap;
			swap(as.second->payload, bs.second->payload);
		}
	}

	template <typename T>
	inline auto handle_table_t<T>::weak_release_impl(page_t* page, slot_t* slot, handle_t h) -> void
	{
		ATMA_ASSERT(slot->wref_count > 0, "bad wref counts");

		if (--slot->wref_count != 0)
			return;

		uint32 byteidx = extract_slot_byte_idx(h);
		uint32 bitidx = extract_slot_bit_idx(h);
		uint32 u32 = page->freefield[byteidx];
		uint32 nu32;

		do {
			nu32 = u32 & ~(0x80000000 >> bitidx);
		} while (!atma::atomic_compare_exchange(&page->freefield[byteidx], u32, nu32, &u32));

		atma::atomic_pre_decrement(&page->size);
	}


	template <typename T>
	inline auto handle_table_t<T>::dump_ascii() -> void
	{
		for (int i = 0; i != pages_size_; ++i)
		{
			if (!pages_[i])
				continue;

			auto const* p = pages_[i];
			for (int j = 0; j != slot_max; ++j)
			{
				if (i == 0 && j == 0)
					std::cout << "-";
				else if (p->freefield[j / 32] & (0x80000000 >> (j % 32)))
					std::cout << pages_[i]->memory[j].ref_count.load();
				else
					std::cout << ".";
			}

			std::cout << " ";
		}

		std::cout << "E" << std::endl;
	}

	template <typename T>
	inline auto handle_table_t<T>::lookup_handle(handle_t h) -> std::pair<page_t*, slot_t*>
	{
		if (h == 0)
			return {nullptr, nullptr};

		uint32 pidx = extract_page_idx(h);
		page_t* page = pages_[pidx];
		if (page == nullptr)
			return {nullptr, nullptr};

		auto sidx = extract_slot_idx(h);
		auto* slot = &page->memory[sidx];

		return {page, slot};
	}

	template <typename T>
	inline auto handle_table_t<T>::lookup_handle(handle_t h) const -> std::pair<page_t const*, slot_t const*>
	{
		if (h == 0)
			return {nullptr, nullptr};

		uint32 pidx = extract_page_idx(h);
		page_t* page = pages_[pidx];
		if (page == nullptr)
			return {nullptr, nullptr};

		auto sidx = extract_slot_idx(h);
		auto const* slot = &page->memory[sidx];

		return {page, slot};
	}
}

