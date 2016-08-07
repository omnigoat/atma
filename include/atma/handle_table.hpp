#pragma once

#include <atma/types.hpp>
//#include <atma/math/compile_time.hpp>

#include <iostream>
#include <atomic>


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

		template <typename... Args> auto allocate(Args&&...) -> handle_t;
		auto release(handle_t) -> bool;

		auto dump_ascii() -> void;


	private: // internal math constants
		static const uint32 genr_bits = 8;
		static const uint32 slot_bits = 6;
		static const uint32 page_bits = 12;
		//static_assert(genr_bits + slot_bits + page_bits == 32, "bad bits");

		static const uint32 slot_byte_bits = 1;
		static const uint32 slot_bit_bits = 5;
		//static_assert(slot_bit_bits + slot_byte_bits == slot_bits, "bad bits");

		static const uint32 genr_max = atma::math::ct::exp2(genr_bits);
		static const uint32 slot_max = atma::math::ct::exp2(slot_bits);
		static const uint32 page_max = atma::math::ct::exp2(page_bits);

		static const uint32 genr_mask = (genr_max - 1) << slot_bits << page_bits;
		static const uint32 slot_mask = (slot_max - 1) << page_bits;
		static const uint32 page_mask = (page_max - 1);
		//static_assert((genr_mask | slot_mask | page_mask) == 0xffffffff, "bad bits");

		static const uint32 slot_byte_mask = (math::ct::exp2(slot_byte_bits) - 1) << slot_bit_bits << page_bits;
		static const uint32 slot_bit_mask = (math::ct::exp2(slot_bit_bits) - 1) << page_bits;;

		static uint32 extract_genr_idx(handle_t h) { return (h & slot_genr_mask) >> slot_bits >> page_bits; }
		static uint32 extract_slot_idx(handle_t h) { return (h & slot_mask) >> page_bits; }
		static uint32 extract_slot_byte_idx(handle_t h) { return (h & slot_byte_mask) >> slot_bit_bits >> page_bits; }
		static uint32 extract_slot_bit_idx(handle_t h) { return (h & slot_bit_mask) >> page_bits; }
		static uint32 extract_page_idx(handle_t h) { return h & page_mask; }

		static auto construct_handle(uint32 genr, uint32 slot_idx, uint32 page_idx) -> handle_t
			{ return (genr << slot_bits << page_bits) | (slot_idx << page_bits) | page_idx; }

	private: // table management
		struct slot_t;
		struct page_t;

		page_t* pages_[page_max] = {};
		page_t* first_page_ = nullptr;
		uint32 pages_size_ = 0;
	};

	template <typename P>
	struct alignas(4) handle_table_t<P>::slot_t
	{
		slot_t(uint32 genr = 0)
			: ref_count{0}
			, genr{genr}
			, prev_slot{0}
			, prev_page{0}
		{}

		std::atomic<uint32> ref_count;
		uint32 genr : 8;
		uint32 prev_slot : 12;
		uint32 prev_page : 12;
		P payload;
	};

	template <typename T>
	struct handle_table_t<T>::page_t
	{
		page_t(uint32 id)
			: id(id)
			, memory{slot_max}
			, freefield{}
		{}

		// returns slot_max if no slot could be allocated
		auto allocate_slot() -> uint32;
		auto release_slot(uint32) -> bool;

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
	inline auto handle_table_t<T>::allocate(Args&&... args) -> handle_t
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
				p = new page_t{page_max};

				// insert page at end of chain, but don't publish it until we've
				// established there's enough space in the table
				if (atma::atomic_compare_exchange<page_t*>(pp, nullptr, p))
				{
					auto pidx = atma::atomic_post_increment<uint32>(&pages_size_);
					if (pidx < page_max)
					{
						// use up the first slot so that page/slot (0, 0) can be used as invalid
						if (pidx == 0)
							p->freefield[0] |= 0x80000000;

						// publish
						pages_[pidx] = p;
						atma::atomic_exchange(&p->id, pidx);
					}
					else
					{
						// unpublish
						*pp = nullptr;
						delete p;
						goto pages_begin;
					}
				}
				else
				{
					delete p;
					goto pages_begin;
				}
			}
			else
			{
				goto pages_begin;
			}
		}
		else if (p->id == page_max)
		{
			goto pages_begin;
		}


		// with supposedly not-full page
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
						p->memory.construct_default(idx, 1);
						p->memory[idx].ref_count++;
						ATMA_ASSERT(p->memory[idx].ref_count == 1);
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

		return construct_handle(0, idx, p->id);
	}

	template <typename T>
	inline auto handle_table_t<T>::release(handle_t h) -> bool
	{
		uint32 pidx = extract_page_idx(h);
		page_t* p = pages_[pidx];
		if (p == nullptr)
			return false;

		auto midx = extract_slot_idx(h);
		if (pidx == 0 && midx == 0)
			int breakpoint = 4;
		p->memory[midx].ref_count--;
		ATMA_ASSERT(p->memory[midx].ref_count == 0);
		
		auto byteidx = extract_slot_byte_idx(h);
		auto bitidx = extract_slot_bit_idx(h);

		uint32 u32 = p->freefield[byteidx];

		uint32 nu32;
		do {
			nu32 = u32 & ~(0x80000000 >> bitidx);
		} while (!atma::atomic_compare_exchange(&p->freefield[byteidx], u32, nu32, &u32));

		atma::atomic_pre_decrement(&p->size);

		return true;
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
					std::cout << "%";
				else if (p->freefield[j / 32] & (0x80000000 >> (j % 32)))
					std::cout << pages_[i]->memory[j].ref_count.load();
				else
					std::cout << ".";
			}

			std::cout << " ";
		}

		std::cout << "E" << std::endl;
	}

}

