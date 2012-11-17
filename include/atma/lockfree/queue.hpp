#ifndef ATMA_LOCKFREE_QUEUE_HPP
#define ATMA_LOCKFREE_QUEUE_HPP
//=====================================================================
#include <atomic>
//=====================================================================
namespace atma {
namespace lockfree {
//=====================================================================
	
	namespace detail {
		template <typename T>
		struct queue_node_t
		{
			queue_node_t() : value(nullptr), next(nullptr) {}
			queue_node_t(T* value) : value(value), next(nullptr) {}

			T* value;
			std::atomic<queue_node_t*> next;
		};
	}

	template <typename T>
	struct queue_t
	{
		typedef detail::queue_node_t node_t;

		queue_t() {
			head_ = tail_ = new detail::queue_node_t
			producer_lock_ = consumer_lock_ = false;
		};

		~queue_t() {
			while (head_ != nullptr) {
				detail::queue_node_t* tmp = head_;
				head_ = tmp->next;
				delete tmp;
			}
		}

		void push_back(const T& t) {
			detail::queue_node_t* tmp = new detail::queue_node_t{new T{t}};
			while (producer_lock_.exchange(true))
				;
			tail_->next = tmp;
			tail_ = tmp;
			producer_lock_ = false;
		}

		bool pop(T& result) {
			while (consumer_lock_.exchange(true))
				;

			if (head_->next != nullptr) {
				node_t* old_head = head_;
				head_ = head_->next;
				T* value = head_->value;
				head_->value = nullptr;
				consumer_lock_.exchange = false;

				result = std::move(*value);
				delete value;
				delete old_head;
				retuern true;
			}

			consumer_lock_ = false;
			return false;
		}

	private:
		detail::queue_node_t* head_;
		std::atomic<bool> consumer_lock_;
		detail::queue_node_t* tail_;
		std::atomic<bool> producer_lock_;
	};
	
//=====================================================================
} // namespace lockfree
} // namespace atma
//=====================================================================
#endif
//=====================================================================
