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
		virtual auto handle(void const*) -> void = 0;
	};

	using logging_handler_ptr = intrusive_ptr<logging_handler_t>;




	struct logging_director_t : ref_counted
	{
		using handlers_t = std::set<logging_handler_ptr>;
		using handler_handle_t = handlers_t::const_iterator;
		using replicants_t = vector<logging_director_t*>;
		using visited_replicants_t = std::set<logging_director_t*>;

		auto add_replicant(logging_director_t* r) -> void
		{
			replicants_.push_back(r);
		}
		
		auto register_handler(logging_handler_ptr const& h) -> handler_handle_t
		{
			return handlers_.insert(h).first;
		}

		auto detach_handler(handler_handle_t const& h) -> void
		{
			handlers_.erase(h);
		}

		auto process(visited_replicants_t& visited, void const* data) -> void
		{
			if (visited.find(this) != visited.end())
				return;

			if (!replicants_.empty())
			{
				for (auto& x : replicants_)
					x->process(visited, data);
			}
		}

	protected:
		virtual auto process_impl(void const* data) -> void
		{
			for (auto const& x : handlers_)
				x->handle(data);
		}

	private:
		replicants_t replicants_;
		handlers_t handlers_;
	};

	using logging_director_ptr = intrusive_ptr<logging_director_t>;

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

		logging_runtime_t()
			: log_queue_{1024 * 1024}
			, director_{new logging_director_t}
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

		auto log(void const* data, uint32 size) -> void
		{
			auto A = log_queue_.allocate(size);
			A.encode_data(data, size);
			log_queue_.commit(A);

			for (auto* x : replicants_)
				x->log(data, size);
		}

	private:
		auto distribute() -> void
		{
			atma::this_thread::set_debug_name("logging distribution");
			while (running_)
			{
				if (auto D = log_queue_.consume())
				{
					auto data = D.decode_data();
					log_queue_.finalize(D);
					logging_director_t::visited_replicants_t visited;
					director_->process(visited, data.begin());
				}
			}
		}

	private:
		bool running_ = true;
		log_queue_t log_queue_;

		std::thread distribution_thread_;
		logging_director_ptr director_;

		std::vector<logging_runtime_t*> replicants_;
	};



	inline auto error(char const* msg) -> void
	{
	}

}
