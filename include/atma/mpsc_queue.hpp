#pragma once

#include <atma/types.hpp>
#include <atma/atomic.hpp>
#include <atma/unique_memory.hpp>
#include <atma/assert.hpp>

#include <chrono>
#include <thread>
#include <algorithm>


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

		enum class alloctype_t : uint32
		{
			invalid = 0,
			normal = 1,
			jump = 2,
			pad = 3,
		};

		// header is {2-bits: alloc-type (invalid, normal, jump, pad), 2-bits: alignment, 28-bits: size}, making the header-size 4 bytes
		//  - alignment is a two-bit number representing a pow-2 exponent, which is multiplied by 4
		//      thus: 0b00 -> pow2(0) == 1  ->  1 * 4 ==  4,  4-byte alignment
		//      thus: 0b01 -> pow2(1) == 2  ->  2 * 4 ==  8,  8-byte alignment
		//      thus: 0b10 -> pow2(2) == 4  ->  4 * 4 == 16, 16-byte alignment
		//      thus: 0b11 -> pow2(3) == 8  ->  8 * 4 == 32, 32-byte alignment
		//
		//  - jump command with a zero size is waiting for a clear
		//
		static uint32 const header_padflag_bitsize = 1;
		static uint32 const header_jumpflag_bitsize = 1;
		static uint32 const header_alignment_bitsize = 2;
		static uint32 const header_size_bitsize = 28;
		static uint32 const header_size = 4;
		static uint32 const jump_command_body_size = sizeof(void*) + sizeof(uint32);

		static uint32 const header_type_bitmask = pow2(header_padflag_bitsize + header_jumpflag_bitsize) - 1;
		static uint32 const header_alignment_bitmask = pow2(header_alignment_bitsize) - 1;
		static uint32 const header_size_bitmask = pow2(header_size_bitsize) - 1;

		std::chrono::nanoseconds const starve_timeout{5000};

		auto impl_read_queue_write_info() -> std::tuple<byte*, uint32, uint32>;
		auto impl_read_queue_read_info() -> std::tuple<byte*, uint32, uint32>;
		auto impl_calculate_available_space(byte* rb, uint32 rp, byte* wb, uint32 wbs, uint32 wp, bool ct) -> uint32;
		auto impl_perform_allocation(byte* wb, uint32 wbs, uint32 wp, uint32 available, uint32 alignment, uint32& size) -> bool;
		auto impl_perform_pad_allocation(byte* wb, uint32 wbs, uint32 wp, uint32 available) -> bool;
		auto impl_make_allocation(byte* wb, uint32 wbs, uint32 wp, alloctype_t, uint32 alignment, uint32 size) -> allocation_t;
		auto impl_encode_jump(uint32 available, byte* wb, uint32 wbs, uint32 wp) -> void;
	
		auto starve_flag(size_t starve_id, size_t thread_id, std::chrono::nanoseconds const& starve_time) -> void;
		auto starve_unflag(size_t starve_id, size_t thread_id) -> void;
		auto starve_gate(size_t thread_id) -> size_t;

		auto write_buf_size() { return write_buf_size_; }

	protected:
		union
		{
			struct
			{
				byte*  write_buf_;
				uint32 write_buf_size_ : 28;
				uint32 write_buf_uses_ :  4;
				uint32 write_position_;
			};

			atma::atomic128_t write_info_;
		};

		union
		{
			struct
			{
				byte* read_buf_;
				uint32 read_buf_size_ : 28;
				uint32 read_buf_uses_ :  4;
				uint32 read_position_;
			};

			atma::atomic128_t read_info_;
		};

		uint32 released_position_ = 0;

		union
		{
			struct
			{
				uint64 starve_thread_;
				uint64 starve_time_;
			};

			atma::atomic128_t starve_info_;
		};

		bool owner_ = false;
	};

	// headerer_t
	struct base_mpsc_queue_t::headerer_t
	{
		auto size() const -> uint32;
		auto alignment() const -> uint32;
		auto data() const -> void*;

	protected:
		headerer_t(byte* buf, uint32 bufsize, uint32 op, uint32 p, uint32 type, uint32 alignment, uint32 size)
			: buf(buf)
			, reader_count(bufsize >> 28)
			, bufsize(bufsize & 0x0fffffff)
			, op(op), p(p)
			, type_(type), alignment_(alignment), size_(size)
		{}

		headerer_t(byte* buf, uint32 bufsize, uint32 op, uint32 p, uint32 header)
			: headerer_t{buf, bufsize, op, p,
				header >> (header_size_bitsize + header_alignment_bitsize),
				(header >> header_size_bitsize) & header_alignment_bitmask,
				header & header_size_bitmask}
		{}

		auto header() const -> uint32 { return (type_ << 30) | (alignment_ << 28) | size_; }
		auto type() const -> alloctype_t { return (alloctype_t)type_; }
		auto raw_size() const -> uint32 { return size_; }

	protected:
		byte*  buf;
		uint32 reader_count : 4;
		uint32 bufsize : 28;
		uint32 op, p;

		uint32 type_      :  2;
		uint32 alignment_ :  2;
		uint32 size_      : 28;
	};

	// allocation_t
	struct base_mpsc_queue_t::allocation_t : headerer_t
	{
		operator bool() const { return buf != nullptr; }

		auto encode_byte(byte) -> void*;
		auto encode_uint16(uint16) -> void;
		auto encode_uint32(uint32) -> void;
		auto encode_uint64(uint64) -> void;
		template <typename T> auto encode_pointer(T*) -> void;
		auto encode_data(void const*, uint32) -> void;

	private:
		allocation_t(byte* buf, uint32 bufsize, uint32 wp, alloctype_t type, uint32 alignment, uint32 size);

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
		decoder_t(byte* buf, uint32 bufsize, uint32 rp);

		friend struct base_mpsc_queue_t;
	};








	inline base_mpsc_queue_t::base_mpsc_queue_t(void* buf, uint32 size)
		: write_buf_((byte*)buf)
		, write_buf_uses_()
		, write_buf_size_(size)
		, write_position_()
		, read_buf_((byte*)buf)
		, read_buf_uses_()
		, read_buf_size_(size)
		, read_position_()
		, starve_thread_()
		, starve_time_()
	{}

	inline base_mpsc_queue_t::base_mpsc_queue_t(uint32 sz)
		: owner_(true)
		, write_buf_(new byte[sz]{})
		, write_buf_uses_()
		, write_buf_size_(sz)
		, write_position_()
		, read_buf_(write_buf_)
		, read_buf_uses_()
		, read_buf_size_(write_buf_size_)
		, read_position_()
		, starve_thread_()
		, starve_time_()
	{}

	inline auto base_mpsc_queue_t::commit(allocation_t& a) -> void
	{
		ATMA_ASSERT(((uint64)(a.buf + a.op)) % 4 == 0);

		// we have already guaranteed that the header does not wrap around our buffer
		atma::atomic_exchange<uint32>(reinterpret_cast<uint32*>(a.buf + a.op), a.header());
	}


#if 0
#  define TMPLOG(...) __VA_ARGS__
#else
#  define TMPLOG(...)
#endif

	inline auto base_mpsc_queue_t::consume() -> decoder_t
	{
		// 1) read the read information, try and move past as fast as possible
		byte* rb = nullptr;
		uint32 rbs = 0;
		uint32 rp = 0;
		uint32 header = 0;
		uint32 size = 0;

		std::tie(rb, rbs, rp) = impl_read_queue_read_info();

		for (;;)
		{
			TMPLOG(std::cout << "[" << std::this_thread::get_id() << "] " << "rb: " << (uint64)rb << ", rbs: " << rbs << std::endl;)
			ATMA_ASSERT(rp % 4 == 0);

			header = *reinterpret_cast<uint32*>(rb + rp);
			size   = header & header_size_bitmask;

			// if header is zero (nothing allocated), we'll fall-through correctly
			if (header == 0)
				break;

			auto rbsc = rbs >> 28;
			auto rbss = rbs & 0x0ffffff;
			ATMA_ASSERT(rbsc != 7, "out of reader-thread space");
			//std::cout << "[" << std::hex << std::this_thread::get_id() << "] " << "dealloc rbsc: " << rbsc << " -> " << (rbsc + 1) << std::endl;
			auto nrbs = ((rbsc + 1) << 28) | rbss;
			auto nrp  = (rp + header_size + size) % rbss;

			atma::atomic128_t read_value;
			if (atma::atomic_compare_exchange(&read_info_,
				atma::atomic128_t{(uint64)rb, rbs, rp},
				atma::atomic128_t{(uint64)rb, nrbs, nrp},
				&read_value))
			{
				//std::cout << "[" << std::hex << std::this_thread::get_id() << "] " << "dealloc success" << std::endl;
				break;
			}
			else
			{
				//std::tie(rb, rbs, rp) = read_value;
				rb  = (byte*)read_value.ui64[0];
				rbs = read_value.ui32[2];
				rp  = read_value.ui32[3];
			}

			//std::cout << "[" << std::hex << std::this_thread::get_id() << "] " << "dealloc failure" << std::endl;
		}

		// 2) we now have exclusive access to our range of memory
		decoder_t D{rb, rbs, rp};

		switch (D.type())
		{
			case alloctype_t::invalid:
				//finalize(D);
				break;

			case alloctype_t::normal:
				break;
			
			// jump to the encoded, larger, read-buffer
			case alloctype_t::jump:
			{
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
		//std::cout << "[" << std::hex << std::this_thread::get_id() << "] " << "finalizing" << std::endl;
		// 1) read info
		

		//  01234567
		//  --------
		//    ###       {2,3} : [2,5)
		//  ##    ##    {6,4} : [0,2)  [6,8)
		//

		// 2) zero-out memory (very required), except header
		auto size_wrap   = std::max((D.op + D.raw_size()) - (int64)D.bufsize, 0ll);
		auto size_middle = D.raw_size() - size_wrap; // std::max((D.op + header_size + D.raw_size()) % (int64)rbss, 0ll);
		memset(D.buf + D.op + header_size, 0, size_middle);
		memset(D.buf, 0, size_wrap);

		// 2.1) atomically set the header to "ready to clear"
		auto clear_value = 0x10000000 | D.size_;

		atma::atomic_exchange(
			(uint32*)(D.buf + D.op),
			clear_value);

		// 3) move release-position along
		for (auto p = D.buf + released_position_;)
		{
			uint32 current_value = *(uint32*)p;
			uint32 sz = current_value & 0x00ffffff;

			if ((current_value & 0xf0000000) != 0x10000000)
				break;

			if (!atma::atomic_compare_exchange(
				(uint32*)p,
				current_value,
				0))
			{ break; }

			released_position_ += sz + header_size;
		}

		byte* rb;
		uint32 rbs, rp;
		std::tie(rb, rbs, rp) = impl_read_queue_read_info();
		ATMA_ASSERT(rp % 4 == 0);

		// 4) decrement use-count
		for (;;)
		{
			uint32 rbss = rbs & 0x0fffffff;
			uint32 rbsc = rbs >> 28;

			ATMA_ASSERT(rbsc != 0, "bad readers count");
			auto nrbs = ((rbsc - 1) << 28) | rbss;

			//std::cout << "[" << std::hex << std::this_thread::get_id() << "] finalize rbsc: " << rbsc << " -> " << (rbsc - 1) << std::endl;

			// if we successfully swapped, then all's good
			atma::atomic128_t u;
			if (atma::atomic_compare_exchange(
				&read_info_,
				atma::atomic128_t{(uint64)rb, rbs, rp},
				atma::atomic128_t{(uint64)rb, nrbs, rp},
				&u))
			{
				//std::cout << "[" << std::hex << std::this_thread::get_id() << "] " << "finalize success" << std::endl;
				break;
			}
			else
			{
				rbs = u.ui32[2];
				rp = u.ui32[3];
				rbss = rbs & 0x0fffffff;
				rbsc = rbs >> 28;
				//std::cout << "[" << std::hex << std::this_thread::get_id() << "] " << "finalize failure" << std::endl;
			}

			//std::cout << "[" << std::hex << std::this_thread::get_id() << "] " << "finalize going again" << std::endl;
		}

		D.type_ = 0;
	}

	inline auto base_mpsc_queue_t::impl_read_queue_write_info() -> std::tuple<byte*, uint32, uint32>
	{
		atma::atomic128_t q_write_info;
		atma::atomic_load_128(&q_write_info, &write_info_);
		return std::make_tuple((byte*)q_write_info.ui64[0], q_write_info.ui32[2], q_write_info.ui32[3]);
	}

	inline auto base_mpsc_queue_t::impl_read_queue_read_info() -> std::tuple<byte*, uint32, uint32>
	{
		atma::atomic128_t q_read_info;
		atma::atomic_load_128(&q_read_info, &read_info_);
		return std::make_tuple((byte*)q_read_info.ui64[0], q_read_info.ui32[2], q_read_info.ui32[3]);
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
	inline auto base_mpsc_queue_t::impl_calculate_available_space(byte* rb, uint32 rp, byte* wb, uint32 wbs, uint32 wp, bool ct) -> uint32
	{
		auto result = (wb == rb) 
			? (rp <= wp ? (wbs - wp + (ct ? 0 : rp)) : rp - wp)
			: wbs - wp;

		result -= header_size;
		result -= 1;

		return result;
	}

	inline auto base_mpsc_queue_t::impl_perform_allocation(byte* wb, uint32 wbs, uint32 wp, uint32 available, uint32 alignment, uint32& size) -> bool
	{
		ATMA_ASSERT(alignment > 0);
		ATMA_ASSERT(wp % 4 == 0);

		// expand size for initial padding required for requested alignment
		size += alignby(wp + header_size, alignment) - wp - header_size;

		// expand size so that next allocation is 4-byte aligned
		size = alignby(size, 4);

		if (available < size + header_size)
			return false;

		uint32 nwp = (wp + header_size + size) % wbs;
		
		return atma::atomic_compare_exchange(
			&write_info_,
			atma::atomic128_t{(uint64)wb, wbs, wp},
			atma::atomic128_t{(uint64)wb, wbs, nwp});
	}

	inline auto base_mpsc_queue_t::impl_perform_pad_allocation(byte* wb, uint32 wbs, uint32 wp, uint32 space) -> bool
	{
		ATMA_ASSERT(wp % 4 == 0);

		// full allocation: header-size + the space it fills
		auto size = header_size + space;

		uint32 nwp = (wp + size) % wbs;

		return atma::atomic_compare_exchange(
			&write_info_,
			atma::atomic128_t{(uint64)wb, wbs, wp},
			atma::atomic128_t{(uint64)wb, wbs, nwp});
	}

	inline auto base_mpsc_queue_t::impl_make_allocation(byte* wb, uint32 wbs, uint32 wp, alloctype_t type, uint32 alignment, uint32 size) -> allocation_t
	{
		return allocation_t{wb, wbs, wp, type, alignment, size};
	}

	inline auto base_mpsc_queue_t::impl_encode_jump(uint32 available, byte* wb, uint32 wbs, uint32 wp) -> void
	{
		static_assert(sizeof(uint64) >= sizeof(void*), "pointers too large! where are you?");

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
	}

	inline auto base_mpsc_queue_t::starve_gate(size_t thread_id) -> size_t
	{
		size_t st = starve_thread_;
		if (st != 0)
		{
			if (st != thread_id)
				while (starve_thread_ != 0)
					;
		}

		return st;
	}

	inline auto base_mpsc_queue_t::starve_flag(size_t starve_id, size_t thread_id, std::chrono::nanoseconds const& starve_time) -> void
	{
		if (starve_time > starve_timeout && (uint64)starve_time.count() > starve_time_)
		{
			if (starve_id != thread_id)
			{
				while (!atma::atomic_compare_exchange(&starve_thread_, 0ull, thread_id))
					;
			}

			starve_time_ = starve_time.count();
		}
	}

	inline auto base_mpsc_queue_t::starve_unflag(size_t starve_id, size_t thread_id) -> void
	{
		if (starve_thread_ == thread_id)
		{
			ATMA_ENSURE(atma::atomic_compare_exchange(&starve_thread_, (uint64)thread_id, uint64()), "shouldn't have contention over resetting starvation");
		}
	}








	// headerer_t
	inline auto base_mpsc_queue_t::headerer_t::size() const -> uint32
	{
		// size after all alignment shenanigans
		uint32 ag = alignment();
		auto opa = alignby(op, ag);
		auto r = size_ - (opa - op);
		return r;
	}

	inline auto base_mpsc_queue_t::headerer_t::alignment() const -> uint32
	{
		return 4 * pow2(alignment_);
	}

	inline auto base_mpsc_queue_t::headerer_t::data() const -> void*
	{
		return buf + (alignby(op + header_size, alignment()) % bufsize);
	}

	// allocation_t
	inline base_mpsc_queue_t::allocation_t::allocation_t(byte* buf, uint32 bufsize, uint32 wp, alloctype_t type, uint32 alignment, uint32 size)
		: headerer_t{buf, bufsize, wp, wp, (uint32)type, alignment, size}
	{
		p = alignby(p + header_size, this->alignment());
		p %= this->bufsize;

		ATMA_ASSERT(size <= header_size_bitmask);
	}

	inline auto base_mpsc_queue_t::allocation_t::encode_byte(byte b) -> void*
	{
		ATMA_ASSERT(p != (op + header_size + raw_size()) % bufsize);
		ATMA_ASSERT(0 <= p && p < bufsize);

		buf[p] = b;
		auto r = buf + p;
		p = (p + 1) % bufsize;
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
		: headerer_t(nullptr, 0, 0, 0, 0)
	{}

	inline base_mpsc_queue_t::decoder_t::decoder_t(byte* buf, uint32 bufsize, uint32 rp)
		: headerer_t(buf, bufsize, rp, rp, *(uint32*)(buf + rp))
	{
		p += header_size;
		p = alignby(p, alignment());
		p %= this->bufsize;
	}

	inline base_mpsc_queue_t::decoder_t::decoder_t(decoder_t&& rhs)
		: headerer_t(rhs.buf, rhs.bufsize, rhs.op, rhs.p, rhs.type_, rhs.alignment_, rhs.size_)
	{
		memset(&rhs, 0, sizeof(decoder_t));
	}

	inline base_mpsc_queue_t::decoder_t::~decoder_t()
	{
		ATMA_ASSERT(type_ == 0, "decoder not finalized before destructing");
	}

	inline auto base_mpsc_queue_t::decoder_t::decode_byte(byte& b) -> void
	{
		b = buf[p];
		p = (p + 1) % bufsize;
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
			ATMA_ASSERT(size <= write_buf_size_, "queue can not allocate that much");

			// write-buffer, write-buffer-size, write-pos
			byte* wb = nullptr;
			uint32 wbs = 0;
			uint32 wp = 0;

			// read-buffer, read-position
			byte* rb = nullptr;

			size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
			std::chrono::nanoseconds starvation{};
			for (;;)
			{
				auto time_start = std::chrono::high_resolution_clock::now();
				auto starve_id = starve_gate(thread_id);

				std::tie(wb, wbs, wp) = impl_read_queue_write_info();
				std::tie(rb, std::ignore, std::ignore) = impl_read_queue_read_info();
				uint32 rp = released_position_;

				size = size_orig;
				uint32 available = impl_calculate_available_space(rb, rp, wb, wbs, wp, contiguous);

				if (available < size && contiguous)
				{
					if (available >= header_size && rp < wp && impl_perform_pad_allocation(wb, wbs, wp, available))
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

				// contiguous allocations can't flag themselves as starved, otherwise
				// it may take precedence, and never have space to allocate
				if (!contiguous)
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

#if 1
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
