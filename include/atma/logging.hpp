#pragma once

#include <atma/types.hpp>
#include <atma/mpsc_queue.hpp>
#include <atma/threading.hpp>
#include <atma/assert.hpp>

#include <atomic>
#include <thread>
#include <set>


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

	struct logbuf_t
	{
	
	private:
		char const* name_;
		log_level_t level_;
	};




	struct logging_handler_t : ref_counted
	{
		virtual auto handle(log_level_t, unique_memory_t const&) -> void = 0;
	};

	using logging_handler_ptr = intrusive_ptr<logging_handler_t>;




	struct logging_runtime_t;

	namespace detail
	{
		inline auto current_runtime() -> logging_runtime_t*&
		{
			static logging_runtime_t* R = nullptr;
			return R;
		}
	}

	struct logging_runtime_t
	{
		using log_queue_t = mpsc_queue_t<false>;
		using handlers_t = std::set<logging_handler_t*>;
		using replicants_t = vector<logging_runtime_t*>;
		using visited_replicants_t = std::set<logging_runtime_t*>;

		enum class command_t : uint32
		{
			connect_replicant,
			disconnect_replicant,
			attach_handler,
			detach_handler,
			log,
		};

		logging_runtime_t(uint32 size = 1024 * 1024)
			: log_queue_{size}
		{
			ATMA_ASSERT(detail::current_runtime() == nullptr);

			distribution_thread_ = std::thread(
				atma::bind(&logging_runtime_t::distribute, this));

			detail::current_runtime() = this;
		}

		~logging_runtime_t()
		{
			detail::current_runtime() = nullptr;
			running_ = false;
			distribution_thread_.join();
		}

		auto connect_replicant(logging_runtime_t* r) -> void
		{
			auto A = log_queue_.allocate(sizeof(command_t) + sizeof(void*));
			A.encode_uint32((uint32)command_t::connect_replicant);
			A.encode_pointer(r);
			log_queue_.commit(A);
		}

		auto disconnect_replicant(logging_runtime_t* r) -> void
		{
			auto A = log_queue_.allocate(sizeof(command_t) + sizeof(void*));
			A.encode_uint32((uint32)command_t::disconnect_replicant);
			A.encode_pointer(r);
			log_queue_.commit(A);
		}

		auto attach_handler(logging_handler_t* h) -> void
		{
			auto A = log_queue_.allocate(sizeof(command_t) + sizeof(void*));
			A.encode_uint32((uint32)command_t::attach_handler);
			A.encode_pointer(h);
			log_queue_.commit(A);
		}

		auto detach_handler(logging_handler_t* h) -> void
		{
			auto A = log_queue_.allocate(sizeof(command_t) + sizeof(void*));
			A.encode_uint32((uint32)command_t::detach_handler);
			A.encode_pointer(h);
			log_queue_.commit(A);
		}

		auto log(log_level_t level, void const* data, uint32 size) -> void
		{
			auto A = log_queue_.allocate(sizeof(command_t) + sizeof(uint32) + sizeof(log_level_t) + sizeof(uint32) + size);
			A.encode_uint32((uint32)command_t::log);
			A.encode_uint32(0);
			//A.encode_pointer(this);
			A.encode_uint32((uint32)level);
			A.encode_data(data, size);
			log_queue_.commit(A);
		}

	private:
		auto distribute() -> void
		{
			atma::this_thread::set_debug_name("logging distribution");

			while (running_)
			{
				if (auto D = log_queue_.consume())
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

						case command_t::log:
						{
							log_level_t level;

							auto visited = D.decode_data();
							D.decode_uint32((uint32&)level);
							auto data = D.decode_data();

							dist_log(visited, level, data);
							break;
						}
					}

					log_queue_.finalize(D);
				}
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

			for (auto* x : replicants_)
			{
				auto A = x->log_queue_.allocate(
					sizeof(command_t) + 
					sizeof(uint32) + ((uint32)visited.size() + 1) * sizeof(this) + 
					sizeof(log_level_t) + sizeof(uint32) + (uint32)data.size());

				A.encode_uint32((uint32)command_t::log);
				A.encode_uint32((uint32)visited.size() + 1);
				for (auto const* x : visited)
					A.encode_pointer(x);
				A.encode_pointer(this);
				A.encode_uint32((uint32)level);
				A.encode_data(data.begin(), (uint32)data.size());
				x->log_queue_.commit(A);
			}
		}

	private:
		bool running_ = true;
		log_queue_t log_queue_;

		std::thread distribution_thread_;

		replicants_t replicants_;
		handlers_t handlers_;
	};



	inline auto error(char const* msg) -> void
	{
	}

}
