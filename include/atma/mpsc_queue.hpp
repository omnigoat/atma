#pragma once

#include <atma/types.hpp>

namespace atma
{
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
		enum class command_t : byte;

		// header is {2-bits: id, 30-bits: size}, making the header-size 4 bytes
		static uint32 const header_idfield_bitsize = 2;
		static uint32 const header_szfield_bitsize = 30;
		static uint32 const header_size = (header_idfield_bitsize + header_szfield_bitsize) / 8;

		auto impl_read_queue_write_info() -> std::tuple<byte*, uint32, uint32>;
		auto impl_read_queue_read_info() -> std::tuple<byte*, uint32>;
		auto impl_calculate_available_space(byte* rb, uint32 rp, byte* wb, uint32 wbs, uint32 wp) -> uint32;
		auto impl_perform_allocation(byte* wb, uint32 wbs, uint32 wp, uint32 available, uint32& size) -> bool;
		auto impl_make_allocation(byte* wb, uint32 wbs, uint32 wp, uint32 size, command_t) -> allocation_t;
		auto impl_encode_jump(uint32 available, byte* wb, uint32 wbs, uint32 wp) -> void;

	private:
		

		auto buf_encode_header(byte*, uint32 bufsize, allocation_t&, command_t, byte = 0) -> void;
		auto buf_encode_byte(byte*, uint32 bufsize, allocation_t&, byte) -> void;
		auto buf_encode_uint16(byte*, uint32 bufsize, allocation_t&, uint16) -> void;
		auto buf_encode_uint32(byte*, uint32 bufsize, allocation_t&, uint32) -> void;
		auto buf_encode_uint64(byte*, uint32 bufsize, allocation_t&, uint64) -> void;

		

	private:
		union
		{
			struct
			{
				byte*  write_buf_;
				uint32 write_buf_size_;
				uint32 write_position_;
			};

			atma::atomic128_t write_info_;
		};

		union
		{
			struct
			{
				byte* read_buf_;
				uint32 read_buf_size_;
				uint32 read_position_;
			};

			atma::atomic128_t read_info_;
		};

		

		bool owner_ = false;
	};

	struct base_mpsc_queue_starvation_t
	{
		std::chrono::nanoseconds const starve_timeout{5000};

		base_mpsc_queue_starvation_t()
			: starve_thread_()
			, starve_time_()
		{}

	protected:
		auto starve_flag(size_t starve_id, size_t thread_id, std::chrono::nanoseconds const& starve_time) -> void;
		auto starve_unflag(size_t starve_id, size_t thread_id) -> void;
		auto starve_gate(size_t thread_id) -> size_t;

	private:
		union
		{
			struct
			{
				uint64 starve_thread_;
				uint64 starve_time_;
			};

			atma::atomic128_t starve_info_;
		};
	};

	struct base_mpsc_queue_t::allocation_t
	{
		auto size() const -> uint32 { return header & 0x3fffffff; }

		auto encode_byte(byte) -> void;
		auto encode_uint16(uint16) -> void;
		auto encode_uint32(uint32) -> void;
		auto encode_uint64(uint64) -> void;

	private:
		allocation_t(byte* buf, uint32 bufsize, uint32 wp, uint32 size, command_t);

		byte*  buf;
		uint32 bufsize;
		uint32 wp, p;
		uint32 header;

		friend struct base_mpsc_queue_t;
	};




	struct base_mpsc_queue_t::decoder_t
	{
		operator bool() const { return buf != nullptr; }

		auto decode_byte(byte&) -> void;
		auto decode_uint16(uint16&) -> void;
		auto decode_uint32(uint32&) -> void;
		auto decode_uint64(uint64&) -> void;

	private:
		decoder_t()
			: buf(), bufsize()
			, size(), p()
		{}

		decoder_t(byte* buf, uint32 bufsize, uint32 rp)
			: buf(buf), bufsize(bufsize)
			, size(*(uint32*)(buf + rp) & 0x3fffffff)
			, p((rp + header_size) % bufsize)
		{
		}

		byte*  buf;
		uint32 bufsize;
		uint32 size;
		uint32 p;

		friend struct base_mpsc_queue_t;
	};




	enum class base_mpsc_queue_t::command_t : byte
	{
		nop,
		jump,
		user
	};




	base_mpsc_queue_t::base_mpsc_queue_t(void* buf, uint32 size)
		: write_buf_((byte*)buf)
		, write_buf_size_(size)
		, write_position_()
		, read_buf_((byte*)buf)
		, read_buf_size_(size)
		, read_position_()
	{}

	base_mpsc_queue_t::base_mpsc_queue_t(uint32 sz)
		: owner_(true)
		, write_buf_(new byte[sz]{})
		, write_buf_size_(sz)
		, write_position_()
		, read_buf_(write_buf_)
		, read_buf_size_(write_buf_size_)
		, read_position_()
	{}

	inline auto base_mpsc_queue_t::impl_read_queue_write_info() -> std::tuple<byte*, uint32, uint32>
	{
		atma::atomic128_t q_write_info;
		atma::atomic_read128(&q_write_info, &write_info_);
		return std::make_tuple((byte*)q_write_info.ui64[0], q_write_info.ui32[2], q_write_info.ui32[3]);
	}

	inline auto base_mpsc_queue_t::impl_read_queue_read_info() -> std::tuple<byte*, uint32>
	{
		atma::atomic128_t q_read_info;
		atma::atomic_read128(&q_read_info, &read_info_);
		return std::make_tuple((byte*)q_read_info.ui64[0], q_read_info.ui32[3]);
	}

	// size of available bytes. subtract one because we must never have
	// the write-position and read-position equal the same value if the
	// buffer is full, because we can't distinguish it from being empty
	//
	// if our buffers are differing (because we are mid-rebase), then
	// we can only allocate up to the end of the new buffer
	inline auto base_mpsc_queue_t::impl_calculate_available_space(byte* rb, uint32 rp, byte* wb, uint32 wbs, uint32 wp) -> uint32
	{
		if (wb == rb)
			return (rp <= wp ? rp + wbs - wp : rp - wp) - 1;
		else
			return wbs - wp - 1;
	}

	inline auto base_mpsc_queue_t::impl_perform_allocation(byte* wb, uint32 wbs, uint32 wp, uint32 available, uint32& size) -> bool
	{
		if (available < size)
			return false;

		// new-write-position can't be "one before" the end of the buffer,
		// as we must write the first two bytes of the header atomically
		uint32 nwp = (wp + size) % wbs;
		if (nwp + header_size >= wbs && (size + (wbs - nwp)) <= available) {
			size += (wbs - nwp);
			nwp = 0;
		};

		atma::atomic128_t oldwi{(uint64)wb, wbs, wp};
		atma::atomic128_t newwi{(uint64)wb, wbs, nwp};

		return atma::atomic_compare_exchange(&write_info_, oldwi, newwi);
		//return atma::atomic_compare_exchange(&write_position_, wp, nwp);
	}

	constexpr auto pow2(uint x) -> uint
	{
		return (x == 0) ? 1 : 2 * pow2(x - 1);
	}

	inline auto base_mpsc_queue_t::impl_make_allocation(byte* wb, uint32 wbs, uint32 wp, uint32 size, command_t c) -> allocation_t
	{
		return allocation_t{wb, wbs, wp, size, c};
	}

	inline auto base_mpsc_queue_t::impl_encode_jump(uint32 available, byte* wb, uint32 wbs, uint32 wp) -> void
	{
		static_assert(sizeof(uint64) >= sizeof(void*), "pointers too large! where are you?");

		uint32 const growcmd_size = sizeof(uint64) + sizeof(uint32) + header_size;

		uint32 gnwp = (wp + growcmd_size) % wbs;

		if (growcmd_size <= available)
		{
			// allocate enough space for the jump command
			uint32 gnwp = (wp + growcmd_size) % wbs;
			if (atma::atomic_compare_exchange(&write_position_, wp, gnwp))
			{
				auto A = allocation_t{wb, wbs, wp, growcmd_size, command_t::jump};

				// current write-info
				atma::atomic128_t q_write_info;
				q_write_info.ui64[0] = (uint64)wb;
				q_write_info.ui32[2] = wbs;
				q_write_info.ui32[3] = gnwp;

				// new-write-info
				atma::atomic128_t nwi;
				auto const nwbs = wbs * 2;
				nwi.ui64[0] = (uint64)new byte[nwbs]();
				nwi.ui32[2] = nwbs;
				nwi.ui32[3] = 0;

				if (atma::atomic_compare_exchange(&write_info_, q_write_info, nwi))
				{
					// successfully (atomically) moved to new buffer encode "jump" command to new buffer
					A.encode_uint64(nwi.ui64[0]);
					A.encode_uint32(nwbs);
				}
				else
				{
					// we failed to move to new buffer: encode a nop
					A.header = A.size() | ((int)command_t::nop << 30);
					delete[] (byte*)nwi.ui64[0];
				}

				commit(A);
			}
		}
	}

	
	auto base_mpsc_queue_t::commit(allocation_t& a) -> void
	{
		ATMA_ASSERT(((uint64)(a.buf + a.wp)) % 4 == 0);

		// we have already guaranteed that the header does not wrap around our buffer
		atma::atomic_exchange<uint32>(a.buf + a.wp, a.header);
		a.wp = a.p = a.header = 0;
	}

	auto base_mpsc_queue_t::buf_encode_header(byte* buf, uint32 bufsize, allocation_t& A, command_t c, byte b) -> void
	{
		buf_encode_byte(buf, bufsize, A, (byte)c);
		buf_encode_byte(buf, bufsize, A, b);
	}

	auto base_mpsc_queue_t::buf_encode_byte(byte* buf, uint32 bufsize, allocation_t& A, byte b) -> void
	{
		ATMA_ASSERT(A.p != (A.wp + A.size()) % bufsize);
		ATMA_ASSERT(0 <= A.p && A.p < bufsize);

		buf[A.p] = b;
		A.p = (A.p + 1) % bufsize;
	}

	auto base_mpsc_queue_t::buf_encode_uint16(byte* buf, uint32 bufsize, allocation_t& A, uint16 i) -> void
	{
		buf_encode_byte(buf, bufsize, A, i & 0xff);
		buf_encode_byte(buf, bufsize, A, (i & 0xff00) >> 8);
	}

	auto base_mpsc_queue_t::buf_encode_uint32(byte* buf, uint32 bufsize, allocation_t& A, uint32 i) -> void
	{
		buf_encode_uint16(buf, bufsize, A, i & 0xffff);
		buf_encode_uint16(buf, bufsize, A, (i & 0xffff0000) >> 16);
	}

	auto base_mpsc_queue_t::buf_encode_uint64(byte* buf, uint32 bufsize, allocation_t& A, uint64 i) -> void
	{
		buf_encode_uint32(buf, bufsize, A, i & 0xffffffff);
		buf_encode_uint32(buf, bufsize, A, i >> 32);
	}

	

	auto base_mpsc_queue_starvation_t::starve_gate(size_t thread_id) -> size_t
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

	auto base_mpsc_queue_starvation_t::starve_flag(size_t starve_id, size_t thread_id, std::chrono::nanoseconds const& starve_time) -> void
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

	auto base_mpsc_queue_starvation_t::starve_unflag(size_t starve_id, size_t thread_id) -> void
	{
		if (starve_thread_ == thread_id)
		{
			ATMA_ENSURE(atma::atomic_compare_exchange(&starve_thread_, (uint64)thread_id, uint64()), "shouldn't have contention over resetting starvation");
		}
	}

	auto base_mpsc_queue_t::consume() -> decoder_t
	{
		auto header = *(uint32*)(read_buf_ + read_position_);
		auto size = header & 0x3fffffff;
		if (size == 0)
			return decoder_t();

		auto command = (command_t)((header & 0xc0000000) >> 30);

		decoder_t D{read_buf_, read_buf_size_, read_position_};

		switch (command)
		{
			// nop means we move to the next location
			case command_t::nop:
				finalize(D);
				return consume();

			// jump to the encoded, larger, read-buffer
			case command_t::jump:
			{
				uint64 ptr;
				uint32 size;
				D.decode_uint64(ptr);
				D.decode_uint32(size);
				delete[] read_buf_;
				read_buf_ = (byte*)ptr;
				read_buf_size_ = size;
				read_position_ = 0;
				std::cout << "JUMP" << std::endl;
				return consume();
			}

			// user
			default:
				break;
		}

		return D;
	}

	inline auto base_mpsc_queue_t::finalize(decoder_t& D) -> void
	{
		read_position_ = (read_position_ + D.size) % read_buf_size_;
	}

	inline base_mpsc_queue_t::allocation_t::allocation_t(byte* buf, uint32 bufsize, uint32 wp, uint32 size, command_t c)
		: buf(buf)
		, bufsize(bufsize)
		, wp(wp)
		, p((wp + header_size) % bufsize)
		, header(((byte)c << 30) | size)
	{
		ATMA_ASSERT((uint)c < pow2(2));
		ATMA_ASSERT(size < pow2(30));
	}

	auto base_mpsc_queue_t::allocation_t::encode_byte(byte b) -> void
	{
		ATMA_ASSERT(p != (wp + size()) % bufsize);
		ATMA_ASSERT(0 <= p && p < bufsize);

		buf[p] = b;
		p = (p + 1) % bufsize;
	}

	auto base_mpsc_queue_t::allocation_t::encode_uint16(uint16 i) -> void
	{
		encode_byte(i & 0xff);
		encode_byte(i >> 8);
	}

	auto base_mpsc_queue_t::allocation_t::encode_uint32(uint32 i) -> void
	{
		encode_uint16(i & 0xffff);
		encode_uint16(i >> 16);
	}

	auto base_mpsc_queue_t::allocation_t::encode_uint64(uint64 i) -> void
	{
		encode_uint32(i & 0xffffffff);
		encode_uint32(i >> 32);
	}

	auto base_mpsc_queue_t::decoder_t::decode_byte(byte& b) -> void
	{
		b = buf[p];
		p = (p + 1) % bufsize;
	}

	auto base_mpsc_queue_t::decoder_t::decode_uint16(uint16& i) -> void
	{
		byte* bs = (byte*)&i;

		decode_byte(bs[0]);
		decode_byte(bs[1]);
	}

	auto base_mpsc_queue_t::decoder_t::decode_uint32(uint32& i) -> void
	{
		byte* bs = (byte*)&i;

		decode_byte(bs[0]);
		decode_byte(bs[1]);
		decode_byte(bs[2]);
		decode_byte(bs[3]);
	}

	auto base_mpsc_queue_t::decoder_t::decode_uint64(uint64& i) -> void
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

	template <bool StarvationPrevention, bool DynamicGrowth>
	struct basic_mpsc_queue_t;

	template <>
	struct basic_mpsc_queue_t<false, false>
		: base_mpsc_queue_t
	{
		basic_mpsc_queue_t(void* buf, uint32 size)
			: base_mpsc_queue_t{buf, size}
		{}

		basic_mpsc_queue_t(uint32 size)
			: base_mpsc_queue_t{size}
		{}

		auto allocate(uint32 size) -> allocation_t
		{
			// also need space for alloc header
			size += base_mpsc_queue_t::header_size;

			//ATMA_ASSERT(size <= write_buf_size_, "queue can not allocate that much");

			// write-buffer, write-buffer-size, write-pos
			byte* wb = nullptr;
			uint32 wbs = 0;
			uint32 wp = 0;

			// read-buffer, read-position
			byte* rb = nullptr;
			uint32 rp = 0;

			for (;;)
			{
				std::tie(wb, wbs, wp) = impl_read_queue_write_info();
				std::tie(rb, rp) = impl_read_queue_read_info();

				uint32 available = impl_calculate_available_space(rb, rp, wb, wbs, wp);
				
				if (impl_perform_allocation(wb, wbs, wp, available, size))
					break;
			}

			return impl_make_allocation(wb, wbs, wp, size, command_t::user);
		}
	};

	template <>
	struct basic_mpsc_queue_t<true, false>
		: base_mpsc_queue_t
		, base_mpsc_queue_starvation_t
	{
		basic_mpsc_queue_t(void* buf, uint32 size)
			: base_mpsc_queue_t{buf, size}
			, base_mpsc_queue_starvation_t{}
		{}

		basic_mpsc_queue_t(uint32 size)
			: base_mpsc_queue_t{size}
		{}

		auto allocate(uint32 size) -> allocation_t
		{
			// also need space for alloc header
			size += base_mpsc_queue_t::header_size;

			//ATMA_ASSERT(size <= write_buf_size_, "queue can not allocate that much");

			// write-buffer, write-buffer-size, write-pos
			byte* wb = nullptr;
			uint32 wbs = 0;
			uint32 wp = 0;

			// read-buffer, read-position
			byte* rb = nullptr;
			uint32 rp = 0;

			size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
			std::chrono::nanoseconds starvation{};
			for (;;)
			{
				auto time_start = std::chrono::high_resolution_clock::now();
				auto starve_id = starve_gate(thread_id);

				std::tie(wb, wbs, wp) = impl_read_queue_write_info();
				std::tie(rb, rp) = impl_read_queue_read_info();

				uint32 available = impl_calculate_available_space(rb, rp, wb, wbs, wp);
				
				if (impl_perform_allocation(wb, wbs, wp, available, size))
					break;

				auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time_start);
				starvation += elapsed;

				starve_flag(starve_id, thread_id, starvation);
			}

			starve_unflag(starve_thread_, thread_id);

			return impl_make_allocation(wb, wbs, wp, size, command_t::user);
		}
	};

	template <>
	struct basic_mpsc_queue_t<false, true>
		: base_mpsc_queue_t
	{
		basic_mpsc_queue_t(void* buf, uint32 size)
			: base_mpsc_queue_t{buf, size}
		{}

		basic_mpsc_queue_t(uint32 size)
			: base_mpsc_queue_t{size}
		{}

		auto allocate(uint32 size) -> allocation_t
		{
			// also need space for alloc header
			size += base_mpsc_queue_t::header_size;

			//ATMA_ASSERT(size <= write_buf_size_, "queue can not allocate that much");

			// write-buffer, write-buffer-size, write-position
			byte* wb = nullptr;
			uint32 wbs = 0;
			uint32 wp = 0;

			// read-buffer, read-position
			byte* rb = nullptr;
			uint32 rp = 0;

			for (;;)
			{
				std::tie(wb, wbs, wp) = impl_read_queue_write_info();
				std::tie(rb, rp) = impl_read_queue_read_info();

				uint32 available = impl_calculate_available_space(rb, rp, wb, wbs, wp);

				if (impl_perform_allocation(wb, wbs, wp, available, size))
					break;
				else
					impl_encode_jump(available, wb, wbs, wp);
			}

			return impl_make_allocation(wb, wbs, wp, size, command_t::user);
		}
	};
}
