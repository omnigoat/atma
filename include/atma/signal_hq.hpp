#ifndef ATMA_SIGNAL_HQ_HPP
#define ATMA_SIGNAL_HQ_HPP
//=====================================================================
#include <atma/lockfree/queue.hpp>
#include <vector>
#include <thread>
#include <algorithm>
//=====================================================================
namespace atma {
//=====================================================================
	
	struct signal_hq_t
	{
		signal_hq_t()
			: is_running_(true)
		{
			ATMA_ASSERT(handle_.get_id() == std::thread::id());
			
			handle_ = std::thread([&]
			{
				// continue performing all commands always and forever
				while (is_running_) {
					std::function<void()> x;
					while (queue_.pop(x)) {
						x();
					}
				}

				#if 0
				// get shutdown commands in LIFO order
				std::vector<std::function<void()>> reversed_commands;
				{
					std::function<void()> x;
					while (detail::shutdown_queue_.pop(x))
						reversed_commands.push_back(x);
				}

				// perform all shutdown commands
				for (auto const& x : reversed_commands)
					x();
				#endif
			});
		}

		~signal_hq_t()
		{
			signal([&] { is_running_ = false; });
			handle_.join();
		}

		auto reenter(std::atomic_bool const& blocked) -> void
		{
			// continue performing all commands always and forever
			std::function<void()> x;
			while (blocked && queue_.pop(x)) {
				x();
			}
		}

		auto signal(std::function<void()> const& fn) -> void
		{
			if (!is_running_)
				return;
			queue_.push(fn);
		}

		auto signal_block() -> void
		{
			if (!is_running_)
				return;

			// push blocking fn!
			std::atomic_bool blocked{true};
			queue_.push([&blocked]{ blocked = false; });

			// don't block if we're the prime thread blocking ourselves.
			if (std::this_thread::get_id() == handle_.get_id())
			{
				reenter(blocked);
				return;
			}
			else
			{
				while (blocked)
					;
			}
		}

	private:
		std::thread handle_;
		bool is_running_;
		atma::lockfree::queue_t<std::function<void()>> queue_;
	};

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

