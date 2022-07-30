#pragma once

#include <atma/lockfree_queue.hpp>
#include <atma/threading.hpp>
#include <atma/streams.hpp>

#include <atomic>
#include <thread>
#include <set>
#include <thread>
#include <sstream>

import atma.bind;
import atma.types;
import atma.vector;
import atma.intrusive_ptr;

namespace atma
{
	enum class log_level_t
	{
		verbose,
		info,
		debug,
		warn,
		error
	};

	enum class log_style_t : uchar
	{
		oneline,
		pretty_print,
	};

	enum class log_instruction_t : uchar
	{
		pad,
		text,
		color,
	};

	struct logbuf_t
	{
	
	private:
		char const* name_;
		log_level_t level_;
	};

	struct colorbyte
	{
		constexpr explicit colorbyte()
			: value()
		{}

		constexpr explicit colorbyte(uchar v)
			: value(v)
		{}

		uchar const value;
	};

	struct logging_handler_t : ref_counted
	{
		virtual auto handle(log_level_t, unique_memory_t const&) -> void = 0;
	};

	using logging_handler_ptr = intrusive_ptr<logging_handler_t>;


	struct logging_runtime_t
	{
		using log_queue_t = lockfree_queue_t;
		using handlers_t = std::set<logging_handler_t*>;
		using replicants_t = vector<logging_runtime_t*>;
		using visited_replicants_t = std::set<logging_runtime_t*>;

		enum class command_t : uint32
		{
			connect_replicant,
			disconnect_replicant,
			attach_handler,
			detach_handler,
			send,
			flush,
		};

		logging_runtime_t(uint32 size = 1024 * 1024)
			: log_queue_{size}
		{
			distribution_thread_ = std::thread(
				atma::bind(&logging_runtime_t::distribute, this));
		}

		~logging_runtime_t()
		{
			running_ = false;
			distribution_thread_.join();
		}

		auto connect_replicant(logging_runtime_t* r) -> void
		{
			log_queue_.with_allocation(sizeof(command_t) + sizeof(void*), [&](auto& A){
				A.encode_uint32((uint32)command_t::connect_replicant);
				A.encode_pointer(r);
			});
		}

		auto disconnect_replicant(logging_runtime_t* r) -> void
		{
			uint32 sz = sizeof(command_t) + sizeof(void*);
			log_queue_.with_allocation(sz, [&](auto& A){
				A.encode_uint32((uint32)command_t::disconnect_replicant);
				A.encode_pointer(r);
			});
		}

		auto attach_handler(logging_handler_t* h) -> void
		{
			uint32 sz = sizeof(command_t) + sizeof(void*);
			log_queue_.with_allocation(sz, [&](auto& A) {
				A.encode_uint32((uint32)command_t::attach_handler);
				A.encode_pointer(h);
			});
		}

		auto detach_handler(logging_handler_t* h) -> void
		{
			uint32 sz = sizeof(command_t) + sizeof(void*);
			log_queue_.with_allocation(sz, [&](auto& A) {
				A.encode_uint32((uint32)command_t::detach_handler);
				A.encode_pointer(h);
			});
		}

		auto log(log_level_t level, void const* data, uint32 size) -> void
		{
			uint32 sz = sizeof(command_t) + sizeof(uint32) + sizeof(log_level_t) + sizeof(uint32) + size;
			log_queue_.with_allocation(sz, [&](auto& A) {
				A.encode_uint32((uint32)command_t::send);
				A.encode_uint32(0u);
				A.encode_uint32((uint32)level);
				A.encode_data(data, size);
			});
		}

		auto flush() -> void
		{
			uint32 sz = sizeof(command_t) + sizeof(bool*);
			bool blocked = true;

			log_queue_.with_allocation(sz, 4, true, [&](auto& A){
				A.encode_uint32((uint32)command_t::flush);
				A.encode_pointer(&blocked);
			});

			while (blocked)
				;
		}

	private:
		auto distribute() -> void
		{
			atma::this_thread::set_debug_name("logging distribution");

			while (running_)
			{
				log_queue_.with_consumption([&](auto& D)
				{
					command_t id;
					D.decode_uint32((uint32&)id);

					switch (id)
					{
						case command_t::connect_replicant:
						{
							logging_runtime_t* r;
							D.decode_pointer(r);
							dist_connect_replicant(r);
							break;
						}

						case command_t::disconnect_replicant:
						{
							logging_runtime_t* r;
							D.decode_pointer(r);
							dist_disconnect_replicant(r);
							break;
						}

						case command_t::attach_handler:
						{
							logging_handler_t* h;
							D.decode_pointer(h);
							dist_attach_handler(h);
							break;
						}

						case command_t::detach_handler:
						{
							logging_handler_t* h;
							D.decode_pointer(h);
							dist_detach_handler(h);
							break;
						}

						case command_t::send:
						{
							log_level_t level;

							auto visited = D.decode_data();
							D.decode_uint32((uint32&)level);
							auto data = D.decode_data();

							dist_log(visited, level, data);
							break;
						}

						case command_t::flush:
						{
							bool* blocked = nullptr;
							D.decode_pointer(blocked);
							*blocked = false;
							break;
						}
					}
				});
			}
		}

	private:
		auto dist_connect_replicant(logging_runtime_t* r) -> void
		{
			replicants_.push_back(r);
		}

		auto dist_disconnect_replicant(logging_runtime_t* r) -> void
		{
			replicants_.erase(
				std::find(replicants_.begin(), replicants_.end(), r),
				replicants_.end());
		}

		auto dist_attach_handler(logging_handler_t* h) -> void
		{
			handlers_.insert(h);
		}

		auto dist_detach_handler(logging_handler_t* h) -> void
		{
			handlers_.erase(h);
		}

		auto dist_log(unique_memory_t const& visited_data, log_level_t level, unique_memory_t const& data) -> void
		{
			memory_view_t<logging_runtime_t* const> visited{visited_data};

			for (auto const* x : visited)
				if (x == this)
					return;
			
			for (auto* handler : handlers_)
				handler->handle(level, data);

			if (replicants_.empty())
				return;

			uint32 sz = (uint32)sizeof(command_t) + sizeof(uint32) +
				(uint32)sizeof(void*) * (uint32)(visited.size() + 1) +
				(uint32)sizeof(log_level_t) + sizeof(uint32) + (uint32)data.size();

			for (auto* x : replicants_)
			{
				x->log_queue_.with_allocation(sz, [&](auto& A)
				{
					A.encode_uint32((uint32)command_t::send);
					A.encode_uint32((uint32)visited.size() + 1);
					for (auto const* x : visited)
						A.encode_pointer(x);
					A.encode_pointer(this);
					A.encode_uint32((uint32)level);
					A.encode_data(data);
				});
			}
		}

	private:
		bool running_ = true;
		log_queue_t log_queue_;

		std::thread distribution_thread_;

		replicants_t replicants_;
		handlers_t handlers_;
	};

	inline auto default_logging_runtime() -> logging_runtime_t*
	{
		static logging_runtime_t default_logging_runtime;
		return &default_logging_runtime;
	}


	struct logging_encoder_t
	{
		logging_encoder_t(output_bytestream_ptr const& dest)
			: dest_{dest}
		{}

		auto encode_header(log_style_t) -> size_t;
		auto encode_color(colorbyte) -> size_t;
		auto encode_cstr(char const*, size_t) -> size_t;

		template <typename... Args>
		auto encode_sprintf(char const*, Args&&...) -> size_t;

		template <typename... Args>
		auto encode_all(Args&&... args) -> size_t
		{
			return encode_all_impl(std::forward<Args>(args)...);
		}

	private:
		auto encode_all_impl() -> size_t
		{
			return 0;
		}

		template <typename... Args>
		auto encode_all_impl(colorbyte color, Args&&... args) -> size_t
		{
			return encode_color(color) + encode_all_impl(std::forward<Args>(args)...);
		}

		template <typename... Args>
		auto encode_all_impl(char const* str, Args const&... args) -> size_t
		{
			auto len = strlen(str);
			return encode_cstr(str, len) + encode_all_impl(args...);
		}

		template <typename... Args, int I>
		auto encode_all_impl(char (&str)[I], Args const&... args) -> size_t
		{
			return encode_cstr(str, I) + encode_all_impl(args...);
		}

		template <typename... Args>
		auto encode_all_impl(std::thread::id id, Args const&... args) -> size_t
		{
			std::stringstream ss;
			ss << id;
			auto str = ss.str();
			return encode_cstr(str.data(), str.size()) + encode_all_impl(args...);
		}

		template <typename... Args>
		auto encode_all_impl(unsigned int const& i, Args const&... args) -> size_t
		{
			auto str = std::to_string(i);
			return encode_cstr(str.data(), str.size()) + encode_all_impl(args...);
		}

	private:
		output_bytestream_ptr dest_;
	};

	inline auto logging_encoder_t::encode_header(log_style_t style) -> size_t
	{
		uchar data[] = {(uchar)style};
		return dest_->write(data, 1).bytes_written;
	}

	inline auto logging_encoder_t::encode_color(colorbyte color) -> size_t
	{
		uchar data[] = {(uchar)log_instruction_t::color, color.value};
		return dest_->write(data, 2).bytes_written;
	}

	inline auto logging_encoder_t::encode_cstr(char const* str, size_t size) -> size_t
	{
		uchar header[] = {(uchar)log_instruction_t::text, (uchar)size, (uchar)(size >> 8)};
		auto R = dest_->write(header, 3).bytes_written;
		R += dest_->write(str, size).bytes_written;
		return R;
	}

	template <typename... Args>
	inline auto logging_encoder_t::encode_sprintf(char const* fmt, Args&&... args) -> size_t
	{
		uchar buf[2048];
		int R = sprintf((char*)buf + 3, fmt, std::forward<Args>(args)...);
		buf[0] = (uchar)log_instruction_t::text;
		buf[1] = (uchar)R;
		buf[2] = (uchar)(R >> 8);
		auto R2 = dest_->write(buf, R + 3);
		return R2.bytes_written;
	}

	template <typename MF, typename CF, typename TF>
	inline auto decode_logging_data(unique_memory_t const& memory, MF&& mf, CF&& cf, TF&& tf) -> void
	{
		std::byte const* data = memory.begin();
		size_t p = 0;

		mf((log_style_t)data[0]);
		p = 1;

		while (p != memory.size())
		{
			// decode header
			log_instruction_t id = (log_instruction_t)*(uchar*)(data + p);
			p += 1;

			if (id == log_instruction_t::color)
			{
				uchar color = *(uchar*)(data + p);
				p += 1;

				cf(color);
			}
			else if (id == log_instruction_t::text)
			{
				uint16 size = *(uint16*)(data + p);
				p += 2;

				tf((char const*)data + p, size);
				p += size;
			}
		}
	}

	inline auto logging_encode_color(void* dest, uchar color) -> size_t
	{
		*((uchar*&)dest)++ = (int)log_instruction_t::color;
		*((uchar*&)dest)++ = color;

		return 3;
	}

	template <typename... Args>
	inline auto logging_encode_string(void* dest, char const* format, Args&&... args) -> size_t
	{
		auto r = sprintf((char*)dest + 3, format, std::forward<Args>(args)...);
		*(uchar*)((uchar*)dest) = (int)log_instruction_t::text;
		*(uint16*)((uchar*)dest + 1) = r;
		return r + 3;
	}



	template <typename... Args>
	inline auto send_log(logging_runtime_t* R, log_level_t level, char const* title, char const* filename, int line, Args&&... args) -> void
	{
		if (R == nullptr)
			return;

		// allocate static buffer & encoder
		size_t const bufsize = 8 * 1024;
		char buf[bufsize];
		auto memstream = intrusive_ptr<memory_bytestream_t>::make(buf, bufsize);
		logging_encoder_t encoder{memstream};

		log_style_t styles[] = {
			log_style_t::oneline,
			log_style_t::oneline,
			log_style_t::oneline,
			log_style_t::pretty_print,
			log_style_t::pretty_print,
		};

		colorbyte colors[] = {
			colorbyte{0x08},
			colorbyte{0x1f},
			colorbyte{0x8f},
			colorbyte{0xe4},
			colorbyte{0xcf},
		};

		colorbyte location_colors[] = {
			colorbyte{0x07},
			colorbyte{0x07},
			colorbyte{0x07},
			colorbyte{0x0e},
			colorbyte{0x0c},
		};

		char const* caption[] = {
			"Trace:",
			"[info]",
			"[debug]",
			"[Warning]",
			"[ERROR]"
		};


		size_t p = 0;
		p += encoder.encode_header(styles[(int)level]);
		p += encoder.encode_color(colors[(int)level]);
		p += encoder.encode_sprintf("%s", caption[(int)level]);

		if (title)
			p += encoder.encode_sprintf(" (%s) ", title);

		p += encoder.encode_color(location_colors[(int)level]);

		if ((int)level >= (int)log_level_t::warn)
		{
			p += encoder.encode_sprintf("\n%s:%d\n", filename, line);
			p += encoder.encode_color(colorbyte{0x07});
		}
		else
		{
			p += encoder.encode_cstr(" ", 1);
		}

		p += encoder.encode_all(std::forward<Args>(args)...);

		R->log(level, buf, (uint32)p);
	}
}

