#pragma once

#include <atma/types.hpp>
#include <atma/atomic.hpp>
#include <atma/unique_memory.hpp>
#include <atma/assert.hpp>

#include <chrono>
#include <thread>
#include <algorithm>
#include <atomic>

namespace atma
{
	constexpr auto pow2(uint x) -> uint
	{
		return (x == 0) ? 1 : 2 * pow2(x - 1);
	}

	constexpr auto log2(uint x) -> uint
	{
		return (x <= 1) ? 0 : 1 + log2(x >> 1);
	}

	constexpr auto alignby(uint x, uint a) -> uint
	{
		return (x + a - 1) & ~(a - 1);
	}

	constexpr auto is_pow2(uint x) -> bool
	{
		return x && (x & (x - 1)) == 0;
	}


	struct base_mpsc_queue_t
	{
		struct allocation_t;
		struct decoder_t;

		base_mpsc_queue_t(void*, uint32);
		base_mpsc_queue_t(uint32);

		auto commit(allocation_t&) -> void;
		auto consume() -> decoder_t;
		auto finalize(decoder_t&) -> void;

	protected:
		struct headerer_t;
		struct queue_state_t;
		struct housekeeping_t;
		
		using atomic_uint32_t = std::atomic<uint32>;

		static_assert(sizeof(atomic_uint32_t) == 4, "assumptions");
		static_assert(alignof(atomic_uint32_t) <= 4, "assumptions");

	protected:
		// HEADER
		//
		//  2 bits: alloc-state {empty, mid_write, full, mid_read}
		//  2 bits: alloc-type {invalid, normal, jump, pad}
		//  2 bits: alignment (exponent for 4*2^x, giving us 4, 8, 16, or 32 bytes alignment)
		// 28-bits: size (allowing up to 64mb allocations)
		//
		// state:
		//  - explicit in alloc-state, except for an empty state with the alloc-type
		//    set to "jump". this is a mid-clear flag.
		//
		enum class allocstate_t : uint32
		{
			empty,
			mid_write,
			full,
			mid_read,
		};

		enum class alloctype_t : uint32
		{
			invalid = 0,
			normal = 1,
			jump = 2,
			pad = 3,
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

		static constexpr uint32 header_state_bitmask = pow2(header_state_bitsize) - 1;
		static constexpr uint32 header_type_bitmask = pow2(header_type_bitsize) - 1;
		static constexpr uint32 header_alignment_bitmask = pow2(header_alignment_bitsize) - 1;
		static constexpr uint32 header_size_bitmask = pow2(header_size_bitsize) - 1;

		static uint32 const invalid_allocation = 0xffffffff;

		std::chrono::nanoseconds const starve_timeout{5000};

		auto impl_read_queue_write_info() -> std::tuple<byte*, uint32, uint32>;
		auto impl_read_queue_read_info() -> std::tuple<byte*, uint32, uint32>;
		auto impl_calculate_available_space(queue_state_t const& w, uint32 wbs, bool ct) -> uint32;
		auto impl_perform_allocation(queue_state_t&, uint32 wbs, uint32& size, uint32 alignment, bool ct) -> uint32;
		auto impl_perform_pad_allocation(byte* wb, uint32 wbs, uint32 wp, uint32 available) -> bool;
		auto impl_make_allocation(byte* wb, uint32 wbs, uint32 wp, alloctype_t, uint32 alignment, uint32 size) -> allocation_t;
		auto impl_encode_jump(uint32 available, byte* wb, uint32 wbs, uint32 wp) -> void;
	
		auto starve_flag(size_t starve_id, size_t thread_id, std::chrono::nanoseconds const& starve_time) -> void;
		auto starve_unflag(size_t starve_id, size_t thread_id) -> void;
		auto starve_gate(size_t thread_id) -> size_t;

	private:
		base_mpsc_queue_t(void*, uint32, bool);

		auto buf_init(void*, uint32, bool) -> void*;
		auto buf_housekeeping(void*) -> housekeeping_t*;

	protected:
		struct queue_state_t
		{
			uint32 wp; // write-position
			uint32 rp; // read-position
			uint32 ep; // empty-position
			uint32 ac; // access-count

			auto available(uint32 bufsize, bool contiguous) const -> uint32
			{
				auto result = ep <= wp ? (bufsize - wp + (contiguous ? 0 : ep)) : ep - wp;

				if (result >= header_size + 1)
				{
					result -= header_size;
					result -= 1;
				}
				else
				{
					result = 0;
				}

				return result;
			}
		};

		struct housekeeping_t
		{
			housekeeping_t(uint32 buffer_size, bool requires_delete)
				: buffer_size_{buffer_size}
				, requires_delete_{requires_delete}
				, atomic_handle{}
				, starve_info_{}
			{}

			auto buffer_size() const -> uint32 { return buffer_size_; }
			auto requires_delete() const -> bool { return requires_delete_; }

		private:
			uint32 buffer_size_ = 0;
			bool requires_delete_ = false;

		public:
			union
			{
				atma::atomic128_t atomic_handle;
				queue_state_t qs;
			};

			union
			{
				atma::atomic128_t starve_info_;

				struct
				{
					uint64 starve_thread_;
					uint64 starve_time_;
				};
			};

			auto atomic_load_queue() -> queue_state_t
			{
				atma::atomic128_t r;
				atma::atomic_load_128(&r, &atomic_handle);
				return *reinterpret_cast<queue_state_t*>(&r);
			}
		};

		struct buffer_t
		{
			buffer_t() {}

			auto housekeeping() -> housekeeping_t* { return (housekeeping_t*)pointer - 1; }

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
	struct base_mpsc_queue_t::headerer_t
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

		auto header() const -> uint32 { return (type_ << 30) | (alignment_ << 28) | size_; }
		auto buffer_size() const -> uint32 { return ((housekeeping_t*)buf_ - 1)->buffer_size(); }
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
	struct base_mpsc_queue_t::allocation_t : headerer_t
	{
		operator bool() const { return buf_ != nullptr; }

		auto encode_byte(byte) -> void*;
		auto encode_uint16(uint16) -> void;
		auto encode_uint32(uint32) -> void;
		auto encode_uint64(uint64) -> void;
		template <typename T> auto encode_pointer(T*) -> void;
		auto encode_data(void const*, uint32) -> void;

	private:
		allocation_t(byte* buf, uint32 wp, allocstate_t, alloctype_t, uint32 alignment, uint32 size);

		friend struct base_mpsc_queue_t;
	};


	// decoder_t
	struct base_mpsc_queue_t::decoder_t : base_mpsc_queue_t::headerer_t
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

		auto local_copy(unique_memory_t& mem) -> void
		{
			mem.reset(size());
			for (int i = 0; i != size(); ++i)
				decode_byte(mem.begin()[i]);
		}

	private:
		decoder_t();
		decoder_t(byte* buf, uint32 rp);

		friend struct base_mpsc_queue_t;
	};






	inline base_mpsc_queue_t::base_mpsc_queue_t(void* buf, uint32 size)
		: base_mpsc_queue_t{buf, size, false}
	{}

	inline base_mpsc_queue_t::base_mpsc_queue_t(uint32 sz)
		: base_mpsc_queue_t{new byte[sz]{}, sz, true}
	{}

	inline base_mpsc_queue_t::base_mpsc_queue_t(void* buf, uint32 size, bool requires_delete)
	{
		ATMA_ASSERT(size > sizeof(housekeeping_t));

		auto nbuf = buf_init(buf, size, requires_delete);

		writing_.pointer = (byte*)nbuf;
		reading_.pointer = (byte*)nbuf;
	}

	

	inline auto base_mpsc_queue_t::commit(allocation_t& a) -> void
	{
		ATMA_ASSERT(((uint64)(a.buf_ + a.op_)) % 4 == 0);

		auto v = atma::atomic_exchange(
			(uint32*)(a.buf_ + a.op_),
			a.header());

		ATMA_ASSERT(v == 0);
	}


#if 0
#  define TMPLOG(...) __VA_ARGS__
#else
#  define TMPLOG(...)
#endif

	inline auto base_mpsc_queue_t::buf_init(void* buf, uint32 size, bool requires_delete) -> void*
	{
		size -= sizeof(housekeeping_t);
		new (buf) housekeeping_t{size, requires_delete};

		return (housekeeping_t*)buf + 1; // + sizeof(housekeeping_t);
	}

	inline auto base_mpsc_queue_t::buf_housekeeping(void* buf) -> housekeeping_t*
	{
		return (housekeeping_t*)buf - 1; //((byte*)buf - sizeof(housekeeping_t));
	}


	inline auto base_mpsc_queue_t::consume() -> decoder_t
	{
		// 1) load read-buffer, and increment use-count
		buffer_t buf;
		atma::atomic_load_128(&buf, &reading_);

#if 0 // I think we can get away with just the atomic-load of the buffer?
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
		queue_state_t qs = hk->atomic_load_queue();
		ATMA_ASSERT(qs.rp < qs.ep ? qs.rp <= qs.wp && qs.wp < qs.ep : qs.wp < qs.ep || qs.rp <= qs.wp);
		
		

		// 3) decode allocation
		//
		// NOTE: after loading qs, other read-threads can totally consume the queue,
		// and write-threads can fill it up, with differently-sized allocations, which
		// means we may dereference our "header" with a mental value. this is fine, as
		// we will discard this if rp has moved (and ac has updated)
		for (;;)
		{
			ATMA_ASSERT(qs.rp % 4 == 0);
			if (qs.rp == qs.wp)
				return decoder_t{};

			// read header (atomic operation on 4-byte alignment)
			uint32 header = *reinterpret_cast<uint32*>(rb + qs.rp);

			// allocation-type
			uint32 type = header >> 28;
			if (type == 0)
				return decoder_t{};

			// allocation-size
			uint32 size = header & header_size_bitmask;
			if (size == 0)
				return decoder_t{};

			// update queue
			auto nqs = qs;
			nqs.rp = (nqs.rp + header_size + size) % hk->buffer_size();
			nqs.ac += 1;
			if (atma::atomic_compare_exchange(&hk->qs, qs, nqs, &qs))
				break;
		}

		// 2) we now have exclusive access to our range of memory
		decoder_t D{rb, qs.rp};

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

	inline auto base_mpsc_queue_t::finalize(decoder_t& D) -> void
	{
		// 1) read info
		auto hk = buf_housekeeping(D.buf_);
		auto bp = (uint32*)(D.buf_ + D.op_);
		ATMA_ASSERT((uint64)bp % 4 == 0);

		// 2) zero-out memory (very required), except header
		//  - use acquire/release so that all memsets get visible
		auto size_wrap   = std::max((D.op_ + header_size + D.raw_size()) - (int64)hk->buffer_size(), 0ll);
		auto size_middle = std::max(D.raw_size() - size_wrap, 0ll);
		ATMA_ASSERT(D.op_ + size_middle <= hk->buffer_size());

		std::atomic_thread_fence(std::memory_order_acquire);
		memset(D.buf_ + (D.op_ + header_size) % hk->buffer_size(), 0, size_middle);
		memset(D.buf_, 0, size_wrap);
		std::atomic_thread_fence(std::memory_order_release);

		// 2.1) atomically set the header to "ready to clear"
		auto v = *bp;
		auto clear_value = 0x10000000 | D.size_;
		
		ATMA_ASSERT(v != clear_value);
		ATMA_ASSERT(v != 0);

		atma::atomic_store(bp, clear_value);

		// 3) move empty-position along
		auto qs = hk->atomic_load_queue();
		auto ep = qs.ep;

		for (auto p = D.buf_ + qs.ep;; p = D.buf_ + qs.ep)
		{
			auto atomp = reinterpret_cast<atomic_uint32_t*>(p);
			ATMA_ASSERT(qs.ep % 4 == 0);

			// atomic load for 4-byte aligned addresses
			uint32 current_value = atomp->load();
			
			if ((current_value & 0xf0000000) != 0x10000000)
				break;

			uint32 sz = current_value & 0x00ffffff;
			ATMA_ASSERT(sz != 0);

			// whoever sets the header to zero gets to update ep
			//if (atma::atomic_compare_exchange((uint32*)p, current_value, 0))
			if (atomp->compare_exchange_strong(current_value, 0))
			{
#if 1
				// for some reason this is much, much faster than the atomic_exchange.
				// I think it must be because it's reducing contention.
				for (;;)
				{
					auto nqs = qs;
					nqs.ac += 1;
					nqs.ep = (qs.ep + header_size + sz) % hk->buffer_size();
					if (atma::atomic_compare_exchange(&hk->qs, qs, nqs, &qs))
						{ qs = nqs; break; }
				}
#else
				atma::atomic_exchange(&hk->qs.ep, (qs.ep + header_size + sz) % hk->buffer_size());
#endif
			}
			else
			{
				break;
			}
		}

		ATMA_ASSERT(qs.rp < qs.ep ? qs.rp <= qs.wp && qs.wp < qs.ep : qs.wp < qs.ep || qs.rp <= qs.wp);

		D.type_ = 0;
	}

	// size of available bytes. subtract one because we must never have
	// the write-position and read-position equal the same value if the
	// buffer is full, because we can't distinguish it from being empty
	//
	// if contiguous, then ignore the [begin, read-pointer) area of the
	// buffer;
	//
	// if our buffers are differing (because we are mid-rebase), then
	// we can only allocate up to the end of the new buffer
	inline auto base_mpsc_queue_t::impl_calculate_available_space(queue_state_t const& w, uint32 wbs, bool ct) -> uint32
	{
		auto result = w.ep <= w.wp ? (wbs - w.wp + (ct ? 0 : w.ep)) : w.ep - w.wp;

		if (result >= header_size + 1)
		{
			result -= header_size;
			result -= 1;
		}
		else
		{
			result = 0;
		}

		return result;
	}

	inline auto base_mpsc_queue_t::impl_perform_allocation(queue_state_t& wqs, uint32 wbs, uint32& size, uint32 alignment, bool ct) -> uint32
	{
		ATMA_ASSERT(alignment > 0);
		ATMA_ASSERT(wqs.wp % 4 == 0);

		// expand size for initial padding required for requested alignment
		size += alignby(wqs.wp + header_size, alignment) - wqs.wp - header_size;

		// expand size so that next allocation is 4-byte aligned
		size = alignby(size, 4);

		if (wqs.available(wbs, ct) < size)
		{
			return invalid_allocation;
		}

		auto nwqs = wqs;
		nwqs.wp = (wqs.wp + header_size + size) % wbs;
		nwqs.ac += 1;

		bool r = atma::atomic_compare_exchange(
			&writing_.housekeeping()->qs,
			wqs,
			nwqs,
			&wqs);

		if (r)
		{
			for (int i = 0; i != header_size + size; ++i)
				 ATMA_ASSERT(writing_.pointer[(wqs.wp + i) % wbs] == 0);
		}

		return r ? wqs.wp : invalid_allocation;
	}

	inline auto base_mpsc_queue_t::impl_perform_pad_allocation(byte* wb, uint32 wbs, uint32 wp, uint32 space) -> bool
	{
		ATMA_ASSERT(wp % 4 == 0);

		// full allocation: header-size + the space it fills
		auto size = header_size + space;

		uint32 nwp = (wp + size) % wbs;

		return atma::atomic_compare_exchange(
			&writing_.housekeeping()->qs.wp,
			wp, nwp);
	}

	inline auto base_mpsc_queue_t::impl_make_allocation(byte* wb, uint32 wbs, uint32 wp, alloctype_t type, uint32 alignment, uint32 size) -> allocation_t
	{
		return allocation_t{wb, wp, allocstate_t::full, type, alignment, size};
	}

	inline auto base_mpsc_queue_t::impl_encode_jump(uint32 available, byte* wb, uint32 wbs, uint32 wp) -> void
	{
		static_assert(sizeof(uint64) >= sizeof(void*), "pointers too large! where are you?");

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

	inline auto base_mpsc_queue_t::starve_gate(size_t thread_id) -> size_t
	{
#if 0
		size_t st = starve_thread_;
		if (st != 0)
		{
			if (st != thread_id)
				while (starve_thread_ != 0)
					;
		}

		return st;
#endif
		return 0;
	}

	inline auto base_mpsc_queue_t::starve_flag(size_t starve_id, size_t thread_id, std::chrono::nanoseconds const& starve_time) -> void
	{
#if 0
		if (starve_time > starve_timeout && (uint64)starve_time.count() > starve_time_)
		{
			if (starve_id != thread_id)
			{
				while (!atma::atomic_compare_exchange(&starve_thread_, 0ull, thread_id))
					;
			}

			starve_time_ = starve_time.count();
		}
#endif
	}

	inline auto base_mpsc_queue_t::starve_unflag(size_t starve_id, size_t thread_id) -> void
	{
#if 0
		if (starve_thread_ == thread_id)
		{
			ATMA_ENSURE(atma::atomic_compare_exchange(&starve_thread_, (uint64)thread_id, uint64()), "shouldn't have contention over resetting starvation");
		}
#endif
	}








	// headerer_t
	inline auto base_mpsc_queue_t::headerer_t::size() const -> uint32
	{
		// size after all alignment shenanigans
		uint32 ag = alignment();
		auto opa = alignby(op_, ag);
		auto r = size_ - (opa - op_);
		return r;
	}

	inline auto base_mpsc_queue_t::headerer_t::alignment() const -> uint32
	{
		return 4 * pow2(alignment_);
	}

	inline auto base_mpsc_queue_t::headerer_t::data() const -> void*
	{
		return buf_ + (alignby(op_ + header_size, alignment()) % buffer_size());
	}

	// allocation_t
	inline base_mpsc_queue_t::allocation_t::allocation_t(byte* buf, uint32 wp, allocstate_t state, alloctype_t type, uint32 alignment, uint32 size)
		: headerer_t{buf, wp, wp, (uint32)state, (uint32)type, alignment, size}
	{
		p_ = alignby(p_ + header_size, this->alignment());
		p_ %= this->buffer_size();

		ATMA_ASSERT(size <= header_size_bitmask);
	}

	inline auto base_mpsc_queue_t::allocation_t::encode_byte(byte b) -> void*
	{
		ATMA_ASSERT(p_ != (op_ + header_size + raw_size()) % buffer_size());
		ATMA_ASSERT(0 <= p_ && p_ < buffer_size());

		buf_[p_] = b;
		auto r = buf_ + p_;
		p_ = (p_ + 1) % buffer_size();
		return r;
	}

	inline auto base_mpsc_queue_t::allocation_t::encode_uint16(uint16 i) -> void
	{
		encode_byte(i & 0xff);
		encode_byte(i >> 8);
	}

	inline auto base_mpsc_queue_t::allocation_t::encode_uint32(uint32 i) -> void
	{
		encode_uint16(i & 0xffff);
		encode_uint16(i >> 16);
	}

	inline auto base_mpsc_queue_t::allocation_t::encode_uint64(uint64 i) -> void
	{
		encode_uint32(i & 0xffffffff);
		encode_uint32(i >> 32);
	}

	template <typename T>
	inline auto base_mpsc_queue_t::allocation_t::encode_pointer(T* p) -> void
	{
#if ATMA_POINTER_SIZE == 8
			encode_uint64((uint64)p);
#elif ATMA_POINTER_SIZE == 4
			encode_uint32((uint32)p);
#else
#  error unknown pointer size??
#endif
	}

	inline auto base_mpsc_queue_t::allocation_t::encode_data(void const* data, uint32 size) -> void
	{
		static_assert(sizeof(uint64) <= sizeof(uintptr), "pointer is greater than 64 bits??");

		encode_uint32(size);
		for (uint32 i = 0; i != size; ++i)
			encode_byte(reinterpret_cast<char const*>(data)[i]);
	}

	// decoder_t
	inline base_mpsc_queue_t::decoder_t::decoder_t()
		: headerer_t(nullptr, 0, 0, 0)
	{}

	inline base_mpsc_queue_t::decoder_t::decoder_t(byte* buf, uint32 rp)
		: headerer_t(buf, rp, rp, *(uint32*)(buf + rp))
	{
		p_ += header_size;
		p_ = alignby(p_, alignment());
		p_ %= buffer_size();
	}

	inline base_mpsc_queue_t::decoder_t::decoder_t(decoder_t&& rhs)
		: headerer_t(rhs.buf_, rhs.op_, rhs.p_, rhs.state_, rhs.type_, rhs.alignment_, rhs.size_)
	{
		memset(&rhs, 0, sizeof(decoder_t));
	}

	inline base_mpsc_queue_t::decoder_t::~decoder_t()
	{
		ATMA_ASSERT(type_ == 0, "decoder not finalized before destructing");
	}

	inline auto base_mpsc_queue_t::decoder_t::decode_byte(byte& b) -> void
	{
		b = buf_[p_];
		p_ = (p_ + 1) % buffer_size();
	}
	
	inline auto base_mpsc_queue_t::decoder_t::decode_uint16(uint16& i) -> void
	{
		byte* bs = (byte*)&i;

		decode_byte(bs[0]);
		decode_byte(bs[1]);
	}

	inline auto base_mpsc_queue_t::decoder_t::decode_uint32(uint32& i) -> void
	{
		byte* bs = (byte*)&i;

		decode_byte(bs[0]);
		decode_byte(bs[1]);
		decode_byte(bs[2]);
		decode_byte(bs[3]);
	}

	inline auto base_mpsc_queue_t::decoder_t::decode_uint64(uint64& i) -> void
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
	inline auto base_mpsc_queue_t::decoder_t::decode_pointer(T*& p) -> void
	{
#if ATMA_POINTER_SIZE == 8
		decode_uint64((uint64&)p);
#elif ATMA_POINTER_SIZE == 4
		decode_uint32((uint32&)p);
#else
#  error unknown pointer size??
#endif
	}

	inline auto base_mpsc_queue_t::decoder_t::decode_data() -> unique_memory_t
	{
		uint32 size;
		decode_uint32(size);
		unique_memory_t um{size};

		for (uint32 i = 0; i != size; ++i)
			decode_byte(um.begin()[i]);

		return std::move(um);
	}






	template <bool DynamicGrowth>
	struct mpsc_queue_ii_t;

#if 1
	template <>
	struct mpsc_queue_ii_t<false>
		: base_mpsc_queue_t
	{
		mpsc_queue_ii_t(void* buf, uint32 size)
			: base_mpsc_queue_t{buf, size}
		{}

		mpsc_queue_ii_t(uint32 size)
			: base_mpsc_queue_t{size}
		{}

		auto allocate(uint32 size, uint32 alignment = 4, bool contiguous = false) -> allocation_t
		{
			ATMA_ASSERT(alignment == 4 || alignment == 8 || alignment == 16 || alignment == 32);

			auto size_orig = size;
			//ATMA_ASSERT(size <= write_buf_size_, "queue can not allocate that much");

			// write-buffer, write-buffer-size, write-pos
			uint32 wbs = 0;
			uint32 wp = 0;

			size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
			std::chrono::nanoseconds starvation{};

			buffer_t writebuf;
			atma::atomic_load_128(&writebuf, &writing_);
			auto whk = writebuf.housekeeping();
			auto wqs = whk->atomic_load_queue();
			wbs = whk->buffer_size();

			for (;;)
			{
				auto time_start = std::chrono::high_resolution_clock::now();
				auto starve_id = starve_gate(thread_id);
				
				// load write-buf, housekeeping, queue-state
				
				size = size_orig;

				ATMA_ASSERT((wqs.wp < wqs.ep && (wqs.rp <= wqs.wp || wqs.ep <= wqs.rp)) || (wqs.wp >= wqs.ep));
				wp = impl_perform_allocation(wqs, wbs, size, alignment, contiguous);
				ATMA_ASSERT((wqs.wp < wqs.ep && (wqs.rp <= wqs.wp || wqs.ep <= wqs.rp)) || (wqs.wp >= wqs.ep));

				if (wp != invalid_allocation)
				{
					break;
				}
				wqs = whk->atomic_load_queue();
				ATMA_ASSERT((wqs.wp < wqs.ep && (wqs.rp <= wqs.wp || wqs.ep <= wqs.rp)) || (wqs.wp >= wqs.ep));
#if 0
				else if (
					contiguous &&
					wqs.ep < wp &&
					wqs.available(wbs, contiguous) >= header_size &&
					impl_perform_pad_allocation(wb, wbs, wp, available)
				) {
					auto A = impl_make_allocation(wb, wbs, wp, alloctype_t::pad, 4, available);
					commit(A);
					continue;
				}
#endif

				// contiguous allocations can't flag themselves as starved, otherwise
				// it may take precedence, and never have space to allocate
				if (!contiguous)
				{
					auto elapsed = std::chrono::high_resolution_clock::now() - time_start;
					starvation += elapsed;
					starve_flag(starve_id, thread_id, starvation);
				}
			}

			//starve_unflag(starve_thread_, thread_id);

			return impl_make_allocation(writebuf.pointer, wbs, wp, alloctype_t::normal, alignment, size);
		}
	};
#endif

#if 0
	template <>
	struct mpsc_queue_ii_t<true>
		: base_mpsc_queue_t
	{
		mpsc_queue_ii_t(void* buf, uint32 size)
			: base_mpsc_queue_t{buf, size}
		{}

		mpsc_queue_ii_t(uint32 size)
			: base_mpsc_queue_t{size}
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

	template <bool DynamicGrowth>
	struct mpsc_queue_t : mpsc_queue_ii_t<DynamicGrowth>
	{
		mpsc_queue_t(uint32 size)
			: mpsc_queue_ii_t{size}
		{}

		template <typename F>
		auto with_allocation(uint32 size, uint32 alignment, bool contiguous, F&& f) -> void
		{
			auto A = allocate(size, alignment, contiguous);
			f(A);
			commit(A);
		}

		template <typename F>
		auto with_allocation(uint32 size, F&& f) -> void
		{
			auto A = allocate(size);
			f(A);
			commit(A);
		}

		template <typename F>
		auto with_consumption(F&& f) -> bool
		{
			if (auto D = consume())
			{
				f(D);
				finalize(D);
				return true;
			}

			return false;
		}
	};

}
