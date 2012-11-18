#ifndef ATMA_LOCKFREE_QUEUE_HPP
#define ATMA_LOCKFREE_QUEUE_HPP
//=====================================================================
#include <atomic>
#include <iostream>
//=====================================================================
namespace atma {
namespace lockfree {
//=====================================================================
	
	// cache padding defined as 4kb
	const unsigned int cache_line_size = 4 * 1024;

	namespace detail {
		template <typename T>
		struct queue_node_t
		{
			queue_node_t() : value(nullptr), next(nullptr) {}
			queue_node_t(const T& value) : value(new T(value)), next(nullptr) {}
			~queue_node_t() { delete value; }

			T* value;
			std::atomic<queue_node_t*> next;
			char pad[ cache_line_size - sizeof(T*) - sizeof(std::atomic<queue_node_t*>) ];
		};
	}

	template <typename T>
	struct queue_t
	{
		typedef detail::queue_node_t<T> node_t;

		queue_t() {
			head_ = tail_ = new node_t;
			producer_lock_ = consumer_lock_ = false;
		};

		~queue_t() {
			while (head_ != nullptr) {
				node_t* tmp = head_;
				head_ = tmp->next;
				delete tmp;
			}
		}

		void push_back(const T& t) {
			node_t* tmp = new node_t(t);
			while (producer_lock_.exchange(true))
				;
			
			tail_->next = tmp;
			tail_ = tmp;
			producer_lock_ = false;
		}

		bool pop(T& result)
		{
			while (consumer_lock_.exchange(true))
				;

			node_t* head = head_;
			node_t* head_next = head_->next;
			if (head_next)
			{
				T* value = head_next->value;
				head_next->value = nullptr;
				head_ = head_next;
				consumer_lock_ = false;

				result = *value;
				delete head;
				return true;
			}

			consumer_lock_ = false;
			return false;
		}

	private:
		char pad0[cache_line_size];

		node_t* head_;
		char pad1[cache_line_size - sizeof(node_t*)];

		std::atomic_bool consumer_lock_;
		char pad2[cache_line_size - sizeof(std::atomic_bool)];

		node_t* tail_;
		char pad3[cache_line_size - sizeof(node_t*)];

		std::atomic_bool producer_lock_;
		char pad4[cache_line_size - sizeof(std::atomic_bool)];
	};
	
//=====================================================================
} // namespace lockfree
} // namespace atma
//=====================================================================
#endif
//=====================================================================
