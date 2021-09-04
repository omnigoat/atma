#pragma once

#include <atma/assert.hpp>
#include <atma/atomic.hpp>
#include <atma/config/platform.hpp>
#include <atma/math/functions.hpp>

#include <chrono>
#include <thread>
#include <algorithm>
#include <atomic>

import atma.types;
import atma.memory;

namespace atma
{
	


	struct base_lockfree_queue_t
	{
		struct allocation_t;
		struct decoder_t;

		base_lockfree_queue_t();
		base_lockfree_queue_t(void*, uint32);
		base_lockfree_queue_t(uint32);

		auto commit(allocation_t&) -> void;
		auto consume() -> decoder_t;
		auto finalize(decoder_t&) -> void;

	protected:
		struct headerer_t;
		struct housekeeping_t;
		
		using atomic_uint32_t = std::atomic<uint32>;

		static_assert(sizeof(atomic_uint32_t) == 4, "assumptions");
		static_assert(alignof(atomic_uint32_t) <= 4, "assumptions");

	protected:
		// HEADER
		//
		//  2 bits: alloc-state {empty, flag_commit, full, mid_read}
		//  2 bits: alloc-type {invalid, normal, jump, pad}
		//  2 bits: alignment (exponent for 4*2^x, giving us 4, 8, 16, or 32 bytes alignment)
		// 28 bits: size (allowing up to 64mb allocations)
		//
		// state:
		//  - explicit in alloc-state, except for an empty state with the alloc-type
		//    set to "jump". this is a mid-clear flag.
		//
		//  - empty +  pad  =>  ready for commit
		//  - empty + jump  =>  ready for finalize
		//
		enum class allocstate_t : uint32
		{
			empty,
			flag_commit,
			full,
			mid_read,
		};

		enum class alloctype_t : uint32
		{
			invalid,
			normal,
			jump,
			pad,
		};

		enum class allocerr_t : uint32
		{
			success,
			invalid,
			invalid_contiguous,
		};

		struct allocinfo_t
		{
			uint32 p;
			allocerr_t err :  2;
			uint32 sz      : 30;

			operator bool() const { return err == allocerr_t::success; }
		};
		
		static constexpr uint32 header_size = 4;
		static constexpr uint32 header_state_bitsize = 2;
		static constexpr uint32 header_state_bitshift = 30;
		static constexpr uint32 header_type_bitsize = 2;
		static constexpr uint32 header_type_bitshift = 28;
		static constexpr uint32 header_alignment_bitsize = 2;
		static constexpr uint32 header_alignment_bitshift = 26;
		static constexpr uint32 header_size_bitsize = 26;
		static constexpr uint32 jump_command_body_size = sizeof(void*) + sizeof(uint32);

		static constexpr uint32 header_state_bitmask = aml::pow2(header_state_bitsize) - 1;
		static constexpr uint32 header_type_bitmask = aml::pow2(header_type_bitsize) - 1;
		static constexpr uint32 header_alignment_bitmask = aml::pow2(header_alignment_bitsize) - 1;
		static constexpr uint32 header_size_bitmask = aml::pow2(header_size_bitsize) - 1;

		static uint32 const invalid_allocation_mask = 0x80000000;
		static uint32 const invalid_contiguous_mask = 0x40000000;

		std::chrono::nanoseconds const starve_timeout{5000};

		auto impl_read_queue_write_info() -> std::tuple<byte*, uint32, uint32>;
		auto impl_read_queue_read_info() -> std::tuple<byte*, uint32, uint32>;
		auto impl_make_allocation(byte* wb, uint32 wbs, uint32 wp, alloctype_t, uint32 alignment, uint32 size) -> allocation_t;
		auto impl_encode_jump(uint32 available, byte* wb, uint32 wbs, uint32 wp) -> void;

	private:
		base_lockfree_queue_t(void*, uint32, bool);

		static auto buf_init(void*, uint32, bool) -> void*;
		static auto buf_housekeeping(void*) -> housekeeping_t*;

		static auto available_space(uint32 wp, uint32 ep, uint32 bufsize, bool contiguous) -> uint32;

	protected:
		struct header_t
		{
			header_t(uint32 x = 0)
				: u32{x}
			{}

			union
			{
				uint32 u32;

				struct
				{
					uint32       size      : header_size_bitsize;
					uint32       alignment : header_alignment_bitsize;
					alloctype_t  type      : header_type_bitsize;
					allocstate_t state     : header_state_bitsize;
				};
			};
		};

		using cursor_t = uint32;

		auto impl_allocate_default(housekeeping_t*, cursor_t const& w, cursor_t const& e, uint32 size, uint32 alignment, bool ct) -> allocinfo_t;
		auto impl_allocate_pad(housekeeping_t*, cursor_t const& w, cursor_t const& e) -> allocinfo_t;
		auto impl_allocate(housekeeping_t*, cursor_t const& w, cursor_t const& e, uint32 size, uint32 alignment, bool ct) -> allocinfo_t;

		struct alignas(16) housekeeping_t
		{
			housekeeping_t(byte* buffer, uint32 buffer_size, bool requires_delete)
				: buffer_{buffer}
				, buffer_size_{buffer_size}
				, requires_delete_{requires_delete}
				, w{}, f{}, r{}, e{buffer_size}
			{}

			cursor_t w; // write
			cursor_t f; // full
			cursor_t r; // read
			cursor_t e; // empty
		
			auto buffer() const -> byte* { return buffer_; }
			auto buffer_size() const -> uint32 { return buffer_size_; }
			auto requires_delete() const -> bool { return requires_delete_; }

		private:
			byte* buffer_ = nullptr;
			uint32 buffer_size_ = 0;
			bool requires_delete_ = false;
		};


	
		struct buffer_t
		{
			buffer_t() 
				: pointer()
				, uses()
			{}

			auto housekeeping() -> housekeeping_t* { return *((housekeeping_t**)pointer - 1); }

			union
			{
				atma::atomic128_t atomic;
				struct
				{
					byte* pointer;
					uint64 uses;
				};
			};
		};

		buffer_t writing_, reading_;
	};


	// headerer_t
	struct base_lockfree_queue_t::headerer_t
	{
		auto alignment() const -> uint32;
		auto size() const -> uint32;
		auto data() const -> void*;

	protected:
		headerer_t(byte* buf, uint32 op, uint32 p, uint32 state, uint32 type, uint32 alignment, uint32 size)
			: buf_{buf}
			, op_{op}, p_{p}
			, state_{state}, type_{type}, alignment_{alignment}, size_{size}
		{}

		headerer_t(byte* buf, uint32 op_, uint32 p_, uint32 header)
			: headerer_t{buf, op_, p_,
				(header >> header_state_bitshift) & header_state_bitmask,
				(header >> header_type_bitshift) & header_type_bitmask,
				(header >> header_alignment_bitshift) & header_alignment_bitmask,
				header & header_size_bitmask}
		{}

		auto header() const -> uint32 { return (state_ << header_state_bitshift) | (type_ << header_type_bitshift) | (alignment_ << header_alignment_bitshift) | size_; }
		auto buffer_size() const -> uint32 { return buf_housekeeping(buf_)->buffer_size(); }
		auto type() const -> alloctype_t { return (alloctype_t)type_; }
		auto raw_size() const -> uint32 { return size_; }

	protected:
		// buffer
		byte* buf_ = nullptr;
		// originating-position, position
		uint32 op_ = 0, p_ = 0;

		// header
		uint32 state_     : header_state_bitsize;
		uint32 type_      : header_type_bitsize;
		uint32 alignment_ : header_alignment_bitsize;
		uint32 size_      : header_size_bitsize;
	};


	// allocation_t
	struct base_lockfree_queue_t::allocation_t : headerer_t
	{
		operator bool() const { return buf_ != nullptr; }

		auto encode_byte(byte) -> void*;
		auto encode_uint16(uint16) -> void;
		auto encode_uint32(uint32) -> void;
		auto encode_uint64(uint64) -> void;
		auto encode_data(void const*, uint32) -> void;
		auto encode_data(unique_memory_t const&) -> void;
		template <typename T> auto encode_pointer(T*) -> void;

		// forward-construct. requires contiguous memory.
		template <typename T> auto encode_struct(T&&) -> bool;

	private:
		allocation_t(byte* buf, uint32 wp, allocstate_t, alloctype_t, uint32 alignment, uint32 size);

		friend struct base_lockfree_queue_t;
	};


	// decoder_t
	struct base_lockfree_queue_t::decoder_t : headerer_t
	{
		decoder_t(decoder_t const&) = delete;
		decoder_t(decoder_t&&);
		~decoder_t();

		operator bool() const { return type_ != 0; }

		auto decode_byte(byte&) -> void;
		auto decode_uint16(uint16&) -> void;
		auto decode_uint32(uint32&) -> void;
		auto decode_uint64(uint64&) -> void;
		template <typename T> auto decode_pointer(T*&) -> void;
		auto decode_data() -> unique_memory_t;

		template <typename T> auto decode_struct(T&) -> void;

		auto local_copy(unique_memory_t& mem) -> void
		{
			auto sz = size();
			mem.reset(sz);
			for (size_t i = 0; i != sz; ++i)
				decode_byte(mem.begin()[i]);
		}

	private:
		decoder_t();
		decoder_t(byte* buf, uint32 rp);

		friend struct base_lockfree_queue_t;
	};




	inline base_lockfree_queue_t::base_lockfree_queue_t()
	{}

	inline base_lockfree_queue_t::base_lockfree_queue_t(void* buf, uint32 size)
		: base_lockfree_queue_t{buf, size, false}
	{}

	inline base_lockfree_queue_t::base_lockfree_queue_t(uint32 sz)
		: base_lockfree_queue_t{new byte[sz]{}, sz, true}
	{}

	inline base_lockfree_queue_t::base_lockfree_queue_t(void* buf, uint32 size, bool requires_delete)
	{
		ATMA_ASSERT(size > sizeof(housekeeping_t*));

		auto nbuf = buf_init(buf, size, requires_delete);

		writing_.pointer = (byte*)nbuf;
		reading_.pointer = (byte*)nbuf;
	}

	inline auto base_lockfree_queue_t::buf_init(void* buf, uint32 size, bool requires_delete) -> void*
	{
		size -= sizeof(housekeeping_t*);
		auto addr = (byte*)((housekeeping_t**)buf + 1);
		*reinterpret_cast<housekeeping_t**>(buf) = atma::aligned_allocator_t<housekeeping_t>().allocate(1); //new housekeeping_t{addr, size, requires_delete};
		new (*reinterpret_cast<housekeeping_t**>(buf)) housekeeping_t{addr, size, requires_delete};

		return addr;
	}

	inline auto base_lockfree_queue_t::buf_housekeeping(void* buf) -> housekeeping_t*
	{
		return *((housekeeping_t**)buf - 1);
	}

	inline auto base_lockfree_queue_t::available_space(uint32 wp, uint32 ep, uint32 bufsize, bool contiguous) -> uint32
	{
		auto result = ep <= wp ? (bufsize - wp + (contiguous ? 0 : ep)) : ep - wp;

		if ((wp + result) % bufsize == ep)
			result -= 1;

		return result;
	}

	inline auto base_lockfree_queue_t::consume() -> decoder_t
	{
		// 1) load read-buffer, and increment use-count
		buffer_t buf;
#if 1 // I think we can get away with just the atomic-load of the buffer?
		atma::atomic_load_128(&buf, &reading_);
#else
		for (;;)
		{
			if (buf.uses == invalid_use_count)
				return decoder_t{};

			auto nbuf = buf;
			++nbuf.uses;

			if (atma::atomic_compare_exchange(&reading_, buf, nbuf, &buf))
				break;
		}
#endif

		byte* rb = buf.pointer;
		
		// 2) load housekeeping & queue-state
		housekeeping_t* hk = buf_housekeeping(rb);

		// 3) decode allocation
		//
		// NOTE: after loading qs, other read-threads can totally consume the queue,
		// and write-threads can fill it up, with differently-sized allocations, which
		// means we may dereference our "header" with a mental value. this is fine, as
		// we will discard this if rp has moved (and ac has updated)
		cursor_t r = atma::atomic_load(&hk->r);
		cursor_t e = atma::atomic_load(&hk->e);

		for (;;)
		{
			ATMA_ASSERT(r % 4 == 0);

			while (e <= r)
				e = atma::atomic_load(&hk->e);

			// read header (atomic operation on 4-byte alignment)
			uint32 rp = r % hk->buffer_size();
			uint32* hp = reinterpret_cast<uint32*>(rb + rp);
			header_t h = atma::atomic_load(hp);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			if (h.state != allocstate_t::full)
				return decoder_t{};

			// update read-pointer
			cursor_t nr = (r + header_size + h.size);
			if (atma::atomic_compare_exchange(&hk->r, r, nr, &r))
			{
				// update allocation to "mid-read"
				auto nh = h;
				nh.state = allocstate_t::mid_read;
				bool br  = atma::atomic_compare_exchange(hp, h.u32, nh.u32);
				(void)br;
				ATMA_ASSERT(br);
				break;
			}
		}

		// 2) we now have exclusive access to our range of memory
		decoder_t D{rb, r % hk->buffer_size()};

		switch (D.type())
		{
			case alloctype_t::invalid:
			case alloctype_t::normal:
				break;
			
			// jump to the encoded, larger, read-buffer
			case alloctype_t::jump:
			{
#if 0
				uint64 ptr;
				uint32 size;
				D.decode_uint64(ptr);
				D.decode_uint32(size);
				finalize(D);
				
				// we must wait until *no* read threads are mid-read before
				// switching the read-buffer out from under them
				auto oldbuf = rb;
				while (!atma::atomic_compare_exchange(&read_info_,
					atma::atomic128_t{(uint64)read_buf_, read_buf_size_ & 0x0fffffff, read_position_},
					atma::atomic128_t{ptr, size, 0}))
					;

				if (owner_)
					delete[] oldbuf;
				owner_ = true;
				return consume();
#endif
				break;
			}

			case alloctype_t::pad:
			{
				finalize(D);
				return consume();
			}
		}

		return std::move(D);
	}

	inline auto base_lockfree_queue_t::finalize(decoder_t& D) -> void
	{
		// 1) read info
		auto hk = buf_housekeeping(D.buf_);
		auto hp = (uint32*)(D.buf_ + D.op_);
		ATMA_ASSERT((uint64)hp % 4 == 0);

		// 2) zero-out memory (very required), except header
		//  - use acquire/release so that all memsets get visible
		auto size_wrap   = std::max((D.op_ + header_size + D.raw_size()) - (int64)hk->buffer_size(), 0ll);
		auto size_middle = std::max(D.raw_size() - size_wrap, 0ll);
		ATMA_ASSERT(D.op_ + size_middle <= hk->buffer_size());

		std::atomic_thread_fence(std::memory_order_acquire);
		memset(D.buf_ + (D.op_ + header_size) % hk->buffer_size(), 0, size_middle);
		memset(D.buf_, 0, size_wrap);
		std::atomic_thread_fence(std::memory_order_release);

		// atomically set the header to "ready to clear", which is encoded as an "empty jump"
		header_t h = atma::atomic_load(hp);
		header_t ch = h;
		header_t oh;
		ch.state = allocstate_t::empty;
		ch.type = alloctype_t::jump;
		auto r = atma::atomic_compare_exchange((header_t*)hp, h, ch, &oh);
		(void)r;
		ATMA_ASSERT(r);
		//ATMA_ASSERT(oh.state == allocstate_t::mid_read);

		// 3) move empty-position along
		uint32 ep = atma::atomic_load(&hk->e);
		for (uint32 epm = ep % hk->buffer_size();; epm = ep % hk->buffer_size())
		{
			auto ehp = reinterpret_cast<uint32*>(hk->buffer() + epm);
			ATMA_ASSERT((uint64)ehp % 4 == 0);

			header_t eh = atma::atomic_load(ehp);
			eh.state = allocstate_t::empty;
			eh.type = alloctype_t::jump;

			// whoever sets the header to zero gets to update ep
			if (atma::atomic_compare_exchange(ehp, eh.u32, 0))
			{
				for (uint32 orig_ep;;)
				{
					// successful moving of ep
					if (atma::atomic_compare_exchange(&hk->e, ep, ep + header_size + eh.size, &orig_ep))
					{
						ep += header_size + eh.size;
						break;
					}
					// very wrong. un-swap and try the whole thing again.
					else if ((orig_ep - ep) % hk->buffer_size() != 0)
					{
						atma::atomic_exchange(ehp, eh.u32);
						ep = atma::atomic_load(&hk->e);
						break;
					}
					// we are stale but in the same canonical place, so with a buffer-size of
					// 16, we're at pos 4 (idx 4), but hk->e is actually at pos 20 (idx 4), so
					// we've swapped a valid header, but are one buffer-size behind. update ep
					// to the currect hk->e, and try again.
					else
					{
						ep = orig_ep;
					}
				}
			}
			else
			{
				break;
			}
		}

		D.type_ = 0;
	}

	inline auto base_lockfree_queue_t::impl_allocate_default(housekeeping_t* hk, cursor_t const& w, cursor_t const& e, uint32 size, uint32 alignment, bool ct) -> allocinfo_t
	{
		ATMA_ASSERT(alignment > 0);
		ATMA_ASSERT(w % 4 == 0);

		// expand size so that:
		//  - we pad up to the requested alignment
		//  - the following allocation is at a 4-byte alignment
		size += aml::alignby(w + header_size, alignment) - w - header_size;
		size  = aml::alignby(size, 4);

		return impl_allocate(hk, w, e, size, alignment, ct);
	}

	inline auto base_lockfree_queue_t::impl_allocate_pad(housekeeping_t* hk, cursor_t const& w, cursor_t const& e) -> allocinfo_t
	{
		// assume that people won't try to allocate the entirety of the buffer...
		if (w < header_size)
			return {0, allocerr_t::invalid, 0};

		auto space = hk->buffer_size() - w - header_size;
		if ((w + header_size + space) % hk->buffer_size() == e)
			return {0, allocerr_t::invalid, 0};

		return impl_allocate(hk, w, e, space, 1, true);
	}

	inline auto base_lockfree_queue_t::impl_allocate(housekeeping_t* hk, cursor_t const& w, cursor_t const& e, uint32 size, uint32 alignment, bool ct) -> allocinfo_t
	{
		ATMA_ASSERT(alignment > 0);
		ATMA_ASSERT(w % 4 == 0);

		uint32 sz = header_size + size;

		// expand size so that:
		uint32 const bs = hk->buffer_size();

		// "original position", "new position", "padding size"
		uint32 op;
		uint32 np;
		uint32 ps = 0;

		// contiguous allocation requires compare-and-swap
		if (ct)
		{
			op = atma::atomic_load(&hk->w);

			for (uint32 npm = (op + sz) % bs;; npm = (op + sz) % bs)
			{
				if (npm != 0 && npm < header_size + size)
				{
					ps = sz - npm;
					np = op + ps + sz;
				}
				else
				{
					np = op + sz;
				}

				if (atma::atomic_compare_exchange(&hk->w, op, np, &op))
					break;
				else
					ps = 0;
			}
		}
		// non-contiguous allocation is fine-and-dandy with an add
		else
		{
			np = atma::atomic_add(&hk->w, sz);
			op = np - sz;
		}

		uint32  p = op % hk->buffer_size();
		uint32* hp = (uint32*)(hk->buffer() + p);
		header_t h = atma::atomic_load(hp);

		// if we've wrapped, we must wait for ep to wrap as well.
		uint32 ep = e;
		if (np < op)
		{
			while (np < ep && ep < op)
				atma::atomic_load(&ep, &hk->e);
		}

		// write padding allocation
		if (ps > 0)
		{
			while (ep < op + ps)
				ep = atma::atomic_load(&hk->e);

			header_t padh;
			padh.state = allocstate_t::full;
			padh.type = alloctype_t::pad;
			padh.size = ps - header_size;
			header_t padv = atma::atomic_exchange(hp, padh);

			op += ps;
			p = op % hk->buffer_size();
			hp = (uint32*)(hk->buffer() + p);
			h = atma::atomic_load(hp);
		}

		while (ep < np)
			ep = atma::atomic_load(&hk->e);

		auto nh = h;
		nh.state = allocstate_t::empty;
		nh.type  = alloctype_t::pad;
		header_t v = atma::atomic_exchange(hp, nh.u32);
		ATMA_ASSERT(v.state == allocstate_t::empty);

#if ATMA_ENABLE_ASSERTS
		for (uint i = header_size; i != header_size + size; ++i)
			ATMA_ASSERT(hk->buffer()[(p + i) % hk->buffer_size()] == byte{0});
#endif

		return {p, allocerr_t::success, size};
	}

	inline auto base_lockfree_queue_t::commit(allocation_t& a) -> void
	{
		uint32* ah = (uint32*)(a.buf_ + a.op_);

		ATMA_ASSERT((uint64)ah % 4 == 0);

		header_t h = a.header();
		ATMA_ASSERT(h.state == allocstate_t::flag_commit);
		h.state = allocstate_t::full;
		header_t v = atma::atomic_exchange(ah, h.u32);
		ATMA_ASSERT(v.state == allocstate_t::empty && v.type == alloctype_t::pad);
	}


	inline auto base_lockfree_queue_t::impl_make_allocation(byte* wb, uint32 wbs, uint32 wp, alloctype_t type, uint32 alignment, uint32 size) -> allocation_t
	{
		return allocation_t{wb, wp, allocstate_t::flag_commit, type, alignment, size};
	}

	inline auto base_lockfree_queue_t::impl_encode_jump(uint32 available, byte* wb, uint32 wbs, uint32 wp) -> void
	{
		//static_assert(sizeof(uint64) >= sizeof(void*), "pointers too large! where are you?");
		//housekeeping_t* hk;
		//hkn.bu
		//auto yay = new char[hk->buffer_size() * 2];
		//auto hk = new (yay) housekeeping_t{yay + sizeof(housekeeping_t*), hk->buffer_size() * 2, true)};
		//hk->
		

#if 0
		if (jump_command_body_size <= available)
		{
			// current write-info
			atma::atomic128_t q_write_info;
			q_write_info.ui64[0] = (uint64)wb;
			q_write_info.ui32[2] = wbs;
			q_write_info.ui32[3] = wp;



			// new-write-info
			atma::atomic128_t nwi;
			auto const nwbs = wbs * 2;
			nwi.ui64[0] = (uint64)new byte[nwbs]();
			nwi.ui32[2] = nwbs;
			nwi.ui32[3] = 0;

			// atomically change the write-info. this means no other thread is now writing
			// into the old write-buffer after us
			if (atma::atomic_compare_exchange(&write_info_, q_write_info, nwi))
			{
				// no other thread can touch the old write-buffer/write-position, so no
				// need to perform atomic operations anymore
				auto A = allocation_t{wb, wbs, wp, alloctype_t::jump, 0, jump_command_body_size};
				A.encode_uint64(nwi.ui64[0]);
				A.encode_uint32(nwbs);
				commit(A);
			}
			else
			{
				delete[] (byte*)nwi.ui64[0];
			}
		}
#endif
	}


	// headerer_t
	inline auto base_lockfree_queue_t::headerer_t::size() const -> uint32
	{
		// size after all alignment shenanigans
		uint32 ag = alignment();
		auto opa = aml::alignby(op_, ag);
		auto r = size_ - (opa - op_);
		return r;
	}

	inline auto base_lockfree_queue_t::headerer_t::alignment() const -> uint32
	{
		return 4 * aml::pow2(alignment_);
	}

	inline auto base_lockfree_queue_t::headerer_t::data() const -> void*
	{
		return buf_ + (aml::alignby(op_ + header_size, alignment()) % buffer_size());
	}

	// allocation_t
	inline base_lockfree_queue_t::allocation_t::allocation_t(byte* buf, uint32 wp, allocstate_t state, alloctype_t type, uint32 alignment, uint32 size)
		: headerer_t{buf, wp, wp, (uint32)state, (uint32)type, alignment, size}
	{
		p_ = aml::alignby(p_ + header_size, this->alignment());
		p_ %= this->buffer_size();

		ATMA_ASSERT(size <= header_size_bitmask);
	}

	inline auto base_lockfree_queue_t::allocation_t::encode_byte(byte b) -> void*
	{
		ATMA_ASSERT(p_ != (op_ + header_size + raw_size()) % buffer_size());
		ATMA_ASSERT(0 <= p_ && p_ < buffer_size());

		buf_[p_] = b;
		auto r = buf_ + p_;
		p_ = (p_ + 1) % buffer_size();
		return r;
	}

	inline auto base_lockfree_queue_t::allocation_t::encode_uint16(uint16 i) -> void
	{
		encode_byte(byte(i & 0xff));
		encode_byte(byte(i >> 8));
	}

	inline auto base_lockfree_queue_t::allocation_t::encode_uint32(uint32 i) -> void
	{
		encode_uint16(i & 0xffff);
		encode_uint16(i >> 16);
	}

	inline auto base_lockfree_queue_t::allocation_t::encode_uint64(uint64 i) -> void
	{
		encode_uint32(i & 0xffffffff);
		encode_uint32(i >> 32);
	}

	template <typename T>
	inline auto base_lockfree_queue_t::allocation_t::encode_pointer(T* p) -> void
	{
#if ATMA_POINTER_SIZE == 8
			encode_uint64((uint64)p);
#elif ATMA_POINTER_SIZE == 4
			encode_uint32((uint32)p);
#else
#  error unknown pointer size??
#endif
	}

	inline auto base_lockfree_queue_t::allocation_t::encode_data(void const* data, uint32 size) -> void
	{
		static_assert(sizeof(uint64) <= sizeof(uintptr), "pointer is greater than 64 bits??");

		encode_uint32(size);
		for (uint32 i = 0; i != size; ++i)
			encode_byte(byte(reinterpret_cast<char const*>(data)[i]));
	}

	template <typename T>
	inline auto base_lockfree_queue_t::allocation_t::encode_struct(T&& x) -> bool
	{
		// require contiugous
		if ((p_ + sizeof(x)) % buffer_size() < p_)
			return false;

		// (copy|move)-construct
		new (buf_ + p_) T{std::forward<T>(x)};

		p_ = (p_ + sizeof(x)) % buffer_size();
		return true;
	}

	inline auto base_lockfree_queue_t::allocation_t::encode_data(unique_memory_t const& data) -> void
	{
		static_assert(sizeof(uint64) <= sizeof(uintptr), "pointer is greater than 64 bits??");

		encode_uint32((uint32)data.size());
		for (byte b : data)
			encode_byte(b);
	}

	// decoder_t
	inline base_lockfree_queue_t::decoder_t::decoder_t()
		: headerer_t(nullptr, 0, 0, 0)
	{}

	inline base_lockfree_queue_t::decoder_t::decoder_t(byte* buf, uint32 rp)
		: headerer_t(buf, rp, rp, *(uint32*)(buf + rp))
	{
		p_ += header_size;
		p_ = aml::alignby(p_, alignment());
		p_ %= buffer_size();
	}

	inline base_lockfree_queue_t::decoder_t::decoder_t(decoder_t&& rhs)
		: headerer_t(rhs.buf_, rhs.op_, rhs.p_, rhs.state_, rhs.type_, rhs.alignment_, rhs.size_)
	{
		memset(&rhs, 0, sizeof(decoder_t));
	}

	inline base_lockfree_queue_t::decoder_t::~decoder_t()
	{
		ATMA_ASSERT(type_ == 0, "decoder not finalized before destructing");
	}

	inline auto base_lockfree_queue_t::decoder_t::decode_byte(byte& b) -> void
	{
		b = buf_[p_];
		p_ = (p_ + 1) % buffer_size();
	}
	
	inline auto base_lockfree_queue_t::decoder_t::decode_uint16(uint16& i) -> void
	{
		byte* bs = (byte*)&i;

		decode_byte(bs[0]);
		decode_byte(bs[1]);
	}

	inline auto base_lockfree_queue_t::decoder_t::decode_uint32(uint32& i) -> void
	{
		byte* bs = (byte*)&i;

		decode_byte(bs[0]);
		decode_byte(bs[1]);
		decode_byte(bs[2]);
		decode_byte(bs[3]);
	}

	inline auto base_lockfree_queue_t::decoder_t::decode_uint64(uint64& i) -> void
	{
		byte* bs = (byte*)&i;

		decode_byte(bs[0]);
		decode_byte(bs[1]);
		decode_byte(bs[2]);
		decode_byte(bs[3]);
		decode_byte(bs[4]);
		decode_byte(bs[5]);
		decode_byte(bs[6]);
		decode_byte(bs[7]);
	}

	template <typename T>
	inline auto base_lockfree_queue_t::decoder_t::decode_pointer(T*& p) -> void
	{
		decode_struct(p);
	}

	inline auto base_lockfree_queue_t::decoder_t::decode_data() -> unique_memory_t
	{
		uint32 size;
		decode_uint32(size);
		unique_memory_t um(atma::allocate_n, size);

		bool is_contiguous = (p_ + size) < buffer_size();

		if (is_contiguous)
		{
			memcpy(um.begin(), buf_ + p_, size);
			p_ = (p_ + size) % buffer_size();
		}
		else
		{
			for (uint32 i = 0; i != size; ++i)
				decode_byte(um.begin()[i]);
		}

		return std::move(um);
	}

	template <typename T>
	inline auto base_lockfree_queue_t::decoder_t::decode_struct(T& x) -> void
	{
		auto const size = (uint32)sizeof(T);
		for (uint32 i = 0; i != size; ++i)
			decode_byte(reinterpret_cast<byte*>(&x)[i]);
	}





	struct lockfree_queue_ii_t
		: base_lockfree_queue_t
	{
		lockfree_queue_ii_t()
			: base_lockfree_queue_t{}
		{}

		lockfree_queue_ii_t(void* buf, uint32 size)
			: base_lockfree_queue_t{buf, size}
		{}

		lockfree_queue_ii_t(uint32 size)
			: base_lockfree_queue_t{size}
		{}

		auto allocate(uint32 size, uint32 alignment = 4, bool contiguous = false) -> allocation_t
		{
			alignment = std::max(alignment, 4u);

			ATMA_ASSERT(alignment == 4 || alignment == 8 || alignment == 16 || alignment == 32);

			std::chrono::nanoseconds starvation{};

			buffer_t writebuf;
			atma::atomic_load_128(&writebuf, &writing_);
			auto whk = writebuf.housekeeping();
			
			ATMA_ASSERT(size <= whk->buffer_size(), "queue can not allocate that much");

			allocinfo_t ai;
			for (;;)
			{
				auto w = atma::atomic_load(&whk->w);
				auto e = atma::atomic_load(&whk->e);

				ai = impl_allocate_default(whk, w, e, size, alignment, contiguous);
				if (ai)
				{
					break;
				}
#if 0
				else if (ai.err == allocerr_t::invalid_contiguous && e <= w)
				{
					if (auto pai = impl_allocate_pad(whk, w, e))
					{
						auto A = impl_make_allocation(writebuf.pointer, whk->buffer_size(), pai.p, alloctype_t::pad, 4, pai.sz);
						commit(A);
					}
				}
#endif
			}

			return impl_make_allocation(writebuf.pointer, whk->buffer_size(), ai.p, alloctype_t::normal, alignment, ai.sz);
		}
	};

#if 0
	template <>
	struct lockfree_queue_ii_t<true>
		: base_lockfree_queue_t
	{
		lockfree_queue_ii_t(void* buf, uint32 size)
			: base_lockfree_queue_t{buf, size}
		{}

		lockfree_queue_ii_t(uint32 size)
			: base_lockfree_queue_t{size}
		{}

		auto allocate(uint32 size, uint32 alignment = 1, bool contiguous = false) -> allocation_t
		{
			ATMA_ASSERT(is_pow2(alignment));
			ATMA_ASSERT(size <= write_buf_size(), "queue can not allocate that much");

			alignment = std::min(std::max(alignment, 4u), 32u);

			// write-buffer, write-buffer-size, write-position
			byte* wb = nullptr;
			uint32 wbs = 0;
			uint32 wp = 0;

			// read-buffer, read-position
			byte* rb = nullptr;

			// starvation info
			size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
			std::chrono::nanoseconds starvation{};

			for (;;)
			{
				auto time_start = std::chrono::high_resolution_clock::now();
				auto starve_id = starve_gate(thread_id);

				std::tie(wb, wbs, wp) = impl_read_queue_write_info();
				std::tie(rb, std::ignore, std::ignore) = impl_read_queue_read_info();

				uint32 available = impl_calculate_available_space(rb, released_position_, wb, wbs, wp, contiguous);
				
				if (available < size && contiguous)
				{
					// we must only pad if there is left-over space for our request.
					// otherwise, we will issue a jump command immediately.
					uint32 ncta = impl_calculate_available_space(rb, released_position_, wb, wbs, wp, false);
					bool paddable = size <= ncta - available;

					if (paddable && impl_perform_pad_allocation(wb, wbs, wp, available))
					{
						auto A = impl_make_allocation(wb, wbs, wp, alloctype_t::pad, 4, available);
						commit(A);
						continue;
					}
				}
				else if (impl_perform_allocation(wb, wbs, wp, available, alignment, size))
				{
					break;
				}

				// we neither padded to the beginning of our buffer, nor allocated the
				// requested amount of space. encode a jump command
				impl_encode_jump(available, wb, wbs, wp);

				if (contiguous)
				{
					auto elapsed = std::chrono::high_resolution_clock::now() - time_start;
					starvation += elapsed;

					starve_flag(starve_id, thread_id, starvation);
				}
			}

			starve_unflag(starve_thread_, thread_id);

			return impl_make_allocation(wb, wbs, wp, alloctype_t::normal, alignment, size);
		}
	};
#endif

	struct lockfree_queue_t : lockfree_queue_ii_t
	{
		using super_type = lockfree_queue_ii_t;

		lockfree_queue_t()
			: super_type{}
		{}

		lockfree_queue_t(uint32 size)
			: super_type{size}
		{}

		lockfree_queue_t(void* buf, uint32 size)
			: super_type{buf, size}
		{}

		template <typename F>
		auto with_allocation(uint32 size, uint32 alignment, bool contiguous, F&& f) -> void
		{
			auto A = this->allocate(size, alignment, contiguous);
			f(A);
			this->commit(A);
		}

		template <typename F>
		auto with_allocation(uint32 size, F&& f) -> void
		{
			auto A = this->allocate(size);
			f(A);
			this->commit(A);
		}

		template <typename F>
		auto with_consumption(F&& f) -> bool
		{
			if (auto D = this->consume())
			{
				f(D);
				this->finalize(D);
				return true;
			}

			return false;
		}
	};

}
