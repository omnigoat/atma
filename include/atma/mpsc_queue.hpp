#pragma once

#include <atma/types.hpp>

namespace atma
{
	struct mpsc_queue_t
	{
		struct allocation_t;
		struct decoder_t;

		mpsc_queue_t(void*, uint32);
		mpsc_queue_t(uint32);

		auto allocate(uint16 size, byte header) -> allocation_t;
		auto commit(allocation_t&) -> void;

		auto consume(decoder_t&) -> bool;

	private:
		enum class command_t : byte;

		// header is {2-bytes: size, 2-bytes: id}
		static uint32 const header_szfield_size = 2;
		static uint32 const header_idfield_size = 2;
		static uint32 const header_size = header_szfield_size + header_idfield_size;

		auto buf_encode_header(byte*, uint32 bufsize, allocation_t&, command_t, byte = 0) -> void;
		auto buf_encode_byte(byte*, uint32 bufsize, allocation_t&, byte) -> void;
		auto buf_encode_uint16(byte*, uint32 bufsize, allocation_t&, uint16) -> void;
		auto buf_encode_uint32(byte*, uint32 bufsize, allocation_t&, uint32) -> void;
		auto buf_encode_uint64(byte*, uint32 bufsize, allocation_t&, uint64) -> void;

		auto starve_flag(size_t starve_id, size_t thread_id, std::chrono::microseconds const& starve_time) -> void;
		auto starve_unflag(size_t starve_id, size_t thread_id) -> void;
		auto starve_gate(size_t thread_id) -> size_t;

	private:
		bool owner_ = false;

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

		union
		{
			struct
			{
				uint64 starve_thread_;
				uint64 starve_time_;
			};

			atma::atomic128_t starve_info_;
		};

		std::chrono::microseconds const starve_timeout{5000};
	};




	struct mpsc_queue_t::allocation_t
	{
		auto encode_byte(byte) -> void;
		auto encode_uint16(uint16) -> void;
		auto encode_uint32(uint32) -> void;
		auto encode_uint64(uint64) -> void;

	private:
		allocation_t(byte* buf, uint32 bufsize, uint32 wp, uint16 size)
			: buf(buf), bufsize(bufsize)
			, wp(wp), p(wp + header_szfield_size)
			, size(size)
		{
			ATMA_ASSERT(size > header_size, "bad command size");
		}

		byte*  buf;
		uint32 bufsize;
		uint32 wp, p;
		uint16 size;

		friend struct mpsc_queue_t;
	};




	struct mpsc_queue_t::decoder_t
	{
		decoder_t() {}

		auto id() const -> byte { return id_; }

		auto decode_byte(byte&) -> void;
		auto decode_uint16(uint16&) -> void;
		auto decode_uint32(uint32&) -> void;
		auto decode_uint64(uint64&) -> void;

	private:
		decoder_t(byte* buf, uint32 bufsize, uint32 rp)
			: buf(buf), bufsize(bufsize)
			, p(rp + header_szfield_size)
			, size(*(uint16*)(buf + rp))
		{
		}

		byte*  buf;
		uint32 bufsize;
		uint32 p;
		uint16 size;
		byte id_ = 0;

		friend struct mpsc_queue_t;
	};




	enum class mpsc_queue_t::command_t : byte
	{
		nop,
		jump,
		user
	};




	mpsc_queue_t::mpsc_queue_t(void* buf, uint32 size)
		: write_buf_((byte*)buf)
		, write_buf_size_(size)
		, write_position_()
		, read_buf_((byte*)buf)
		, read_buf_size_(size)
		, read_position_()
		, starve_thread_()
		, starve_time_()
	{}

	mpsc_queue_t::mpsc_queue_t(uint32 sz)
		: owner_(true)
		, write_buf_(new byte[sz]{})
		, write_buf_size_(sz)
		, write_position_()
		, read_buf_(write_buf_)
		, read_buf_size_(write_buf_size_)
		, read_position_()
		, starve_thread_()
		, starve_time_()
	{}

	auto mpsc_queue_t::allocate(uint16 size, byte user_header) -> allocation_t
	{
		// also need space for alloc header
		size += header_size;

		ATMA_ASSERT(size <= write_buf_size_, "queue can not allocate that much");

		size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());


		byte* wb = nullptr;
		uint32 wbs = 0;
		uint32 wp = 0;

		std::chrono::microseconds starvation{};
		for (;;)
		{
			auto time_start = std::chrono::high_resolution_clock::now();

			// starve_id is either zero or our thread-id, at any point
			// during this function-call "starve_.thread" could change
			auto starve_id = starve_gate(thread_id);

			// read 16 bytes of {write-buf, buf-size, write-pos}
			atma::atomic128_t q_write_info;
			atma::atomic_read128(&q_write_info, &write_info_);

			// write-buffer, write-buffer-size, write-position
			wb = (byte*)q_write_info.ui64[0];
			wbs = q_write_info.ui32[2];
			wp = q_write_info.ui32[3];

			// read-buffer, read-position
			atma::atomic128_t q_read_info;
			atma::atomic_read128(&q_read_info, &read_info_);
			auto rb = (byte*)q_read_info.ui64[0];
			auto rp = q_read_info.ui32[3];

			// size of available bytes. subtract one because we must never have
			// the write-position and read-position equal the same value if the
			// buffer is full, because we can't distinguish it from being empty
			//
			// if our buffers are differing (because we are mid-rebase), then
			// we can only allocate up to the end of the new buffer
			uint32 available = 0;
			if (wb == rb)
				available = (rp <= wp ? rp + wbs - wp : rp - wp) - 1;
			else
				available = wbs - wp - 1;


			// new-write-position can't be "one before" the end of the buffer,
			// as we must write the first two bytes of the header atomically
			auto nwp = (wp + size) % wbs;
			if (nwp == wbs - 1) {
				++nwp;
				++size;
			}

			if (size <= available)
			{
				if (atma::atomic_compare_exchange(&write_position_, wp, nwp))
				{
					//starve_unflag(starve_id, thread_id);
					break;
				}
			}
			// not enough space for our command. we may eventually have to block (busy-wait),
			// but first let's try to create a *new* buffer, atomically set all new writes to
			// write into that buffer, and encode a jump command in our current buffer. jump
			// commands are very small (16 bytes), so maybe that'll work.
			else
			{
				static_assert(sizeof(uint64) >= sizeof(void*), "pointers too large! where are you?");

				// new write-information
				atma::atomic128_t nwi;
				auto const nwbs = wbs * 2;
				nwi.ui64[0] = (uint64)new byte[nwbs]();
				nwi.ui32[2] = nwbs;
				nwi.ui32[3] = 0;


				uint16 const growcmd_size = 12 + 4;
				if (growcmd_size <= available)
				{
					uint32 gnwp = (wp + growcmd_size) % wbs;

					if (atma::atomic_compare_exchange(&write_position_, wp, gnwp))
					{
						auto A = allocation_t{wb, wbs, wp, growcmd_size};

						// update "known" write-position
						q_write_info.ui32[3] = gnwp;

						if (atma::atomic_compare_exchange(&write_info_, q_write_info, nwi))
						{
							// successfully (atomically) moved to new buffer
							// encode "jump" command to new buffer
							A.encode_byte((byte)command_t::jump);
							A.encode_byte(0);
							A.encode_uint64(nwi.ui64[0]);
							A.encode_uint32(nwbs);
						}
						else
						{
							// we failed to move to new buffer: encode a nop
							buf_encode_header(wb, wbs, A, command_t::nop);
							delete[](byte*)nwi.ui64[0];
						}

						commit(A);
					}
				}
				else
				{
					delete[](byte*)nwi.ui64[0];
				}
			}

			auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - time_start);
			starvation += elapsed;

			starve_flag(starve_id, thread_id, starvation);
		}

		starve_unflag(starve_thread_, thread_id);

		auto A = allocation_t{wb, wbs, wp, size};
		A.encode_byte((byte)command_t::user);
		A.encode_byte(user_header);
		return A;
	}

	auto mpsc_queue_t::commit(allocation_t& a) -> void
	{
		// we have already guaranteed that the first two bytes of the header
		// does not wrap around our buffer
		atma::atomic_exchange(a.buf + a.wp, a.size);
		a.wp = a.p = a.size = 0;
	}

	auto mpsc_queue_t::buf_encode_header(byte* buf, uint32 bufsize, allocation_t& A, command_t c, byte b) -> void
	{
		buf_encode_byte(buf, bufsize, A, (byte)c);
		buf_encode_byte(buf, bufsize, A, b);
	}

	auto mpsc_queue_t::buf_encode_byte(byte* buf, uint32 bufsize, allocation_t& A, byte b) -> void
	{
		ATMA_ASSERT(A.p != (A.wp + A.size) % bufsize);
		ATMA_ASSERT(0 <= A.p && A.p < bufsize);

		buf[A.p] = b;
		A.p = (A.p + 1) % bufsize;
	}

	auto mpsc_queue_t::buf_encode_uint16(byte* buf, uint32 bufsize, allocation_t& A, uint16 i) -> void
	{
		buf_encode_byte(buf, bufsize, A, i & 0xff);
		buf_encode_byte(buf, bufsize, A, (i & 0xff00) >> 8);
	}

	auto mpsc_queue_t::buf_encode_uint32(byte* buf, uint32 bufsize, allocation_t& A, uint32 i) -> void
	{
		buf_encode_uint16(buf, bufsize, A, i & 0xffff);
		buf_encode_uint16(buf, bufsize, A, (i & 0xffff0000) >> 16);
	}

	auto mpsc_queue_t::buf_encode_uint64(byte* buf, uint32 bufsize, allocation_t& A, uint64 i) -> void
	{
		buf_encode_uint32(buf, bufsize, A, i & 0xffffffff);
		buf_encode_uint32(buf, bufsize, A, i >> 32);
	}


	auto mpsc_queue_t::starve_gate(size_t thread_id) -> size_t
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

	auto mpsc_queue_t::starve_flag(size_t starve_id, size_t thread_id, std::chrono::microseconds const& starve_time) -> void
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

	auto mpsc_queue_t::starve_unflag(size_t starve_id, size_t thread_id) -> void
	{
		if (starve_thread_ == thread_id)
		{
			ATMA_ENSURE(atma::atomic_compare_exchange(&starve_thread_, (uint64)thread_id, uint64()), "shouldn't have contention over resetting starvation");
		}
	}

	auto mpsc_queue_t::consume(decoder_t& D) -> bool
	{
		// read first 16 bytes of header
		uint16 size = *(uint16*)(read_buf_ + read_position_);
		if (size == 0)
			return false;

		D.buf = read_buf_;
		D.bufsize = read_buf_size_;
		D.p = read_position_ + 2;
		D.size = size;

		// read second 16 bytes of header
		byte header;
		D.decode_byte(header);
		D.decode_byte(D.id_);

		read_position_ = (read_position_ + size) % read_buf_size_;

		switch ((command_t)header)
		{
			// nop means we move to the next location
			case command_t::nop:
				return consume(D);

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
				std::cout << "JUMPED TO SIZE " << std::dec << size << std::endl;
				return consume(D);
			}

			// user
			default:
				break;
		}

		return true;
	}

	auto mpsc_queue_t::allocation_t::encode_byte(byte b) -> void
	{
		ATMA_ASSERT(p != (wp + size) % bufsize);
		ATMA_ASSERT(0 <= p && p < bufsize);

		buf[p] = b;
		p = (p + 1) % bufsize;
	}

	auto mpsc_queue_t::allocation_t::encode_uint16(uint16 i) -> void
	{
		encode_byte(i & 0xff);
		encode_byte(i >> 8);
	}

	auto mpsc_queue_t::allocation_t::encode_uint32(uint32 i) -> void
	{
		encode_uint16(i & 0xffff);
		encode_uint16(i >> 16);
	}

	auto mpsc_queue_t::allocation_t::encode_uint64(uint64 i) -> void
	{
		encode_uint32(i & 0xffffffff);
		encode_uint32(i >> 32);
	}

	auto mpsc_queue_t::decoder_t::decode_byte(byte& b) -> void
	{
		b = buf[p];
		p = (p + 1) % bufsize;
	}

	auto mpsc_queue_t::decoder_t::decode_uint16(uint16& i) -> void
	{
		byte* bs = (byte*)&i;

		decode_byte(bs[0]);
		decode_byte(bs[1]);
	}

	auto mpsc_queue_t::decoder_t::decode_uint32(uint32& i) -> void
	{
		byte* bs = (byte*)&i;

		decode_byte(bs[0]);
		decode_byte(bs[1]);
		decode_byte(bs[2]);
		decode_byte(bs[3]);
	}

	auto mpsc_queue_t::decoder_t::decode_uint64(uint64& i) -> void
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

}
