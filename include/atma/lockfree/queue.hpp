#ifndef ATMA_LOCKFREE_QUEUE_HPP
#define ATMA_LOCKFREE_QUEUE_HPP
//=====================================================================
#include <atomic>
#include <iostream>
//=====================================================================
namespace atma {
namespace lockfree {
//=====================================================================
	
	// cache padding defined as 32 bytes
	const unsigned int cache_line_size = 32;


	//=====================================================================
	// queue_t
	//=====================================================================
	template <typename T>
	struct queue_t
	{
		queue_t();
		~queue_t();

		void push(const T& t);
		bool pop(T& result);
		
	private:
		queue_t(const queue_t&);
		queue_t(queue_t&&);
		queue_t& operator = (const queue_t&);
		queue_t& operator = (queue_t&&);

		struct node_t;

	private:
		char pad0[cache_line_size];

		node_t* head_;
		char pad1[sizeof(node_t*) % cache_line_size];

		std::atomic_bool consumer_lock_;
		char pad2[sizeof(std::atomic_bool) % cache_line_size];
		
		node_t* tail_;
		char pad3[sizeof(node_t*) % cache_line_size];

		std::atomic_bool producer_lock_;
		char pad4[sizeof(std::atomic_bool) % cache_line_size];
	};
	

	//=====================================================================
	// queue_t::node_t
	//=====================================================================
	template <typename T>
	struct queue_t<T>::node_t
	{
		node_t() : value(nullptr), next(nullptr) {}
		node_t(const T& value) : value(new T(value)), next(nullptr) {}
		~node_t() { delete value; }

		T* value;
		std::atomic<node_t*> next;
		char pad[ (sizeof(T*) + sizeof(std::atomic<node_t*>)) % cache_line_size ];
	};






	//=====================================================================
	// queue_t implementation
	//=====================================================================
	template <typename T>
	queue_t<T>::queue_t() {
		head_ = tail_ = new node_t;
		producer_lock_ = consumer_lock_ = false;
	};

	template <typename T>
	queue_t<T>::~queue_t() {
		while (head_ != nullptr) {
			node_t* tmp = head_;
			head_ = tmp->next;
			delete tmp;
		}
	}

	template <typename T>
	void queue_t<T>::push(const T& t) {
		node_t* tmp = new node_t(t);
		while (producer_lock_.exchange(true))
			;
		tail_->next = tmp;
		tail_ = tmp;
		producer_lock_ = false;
	}

	template <typename T>
	bool queue_t<T>::pop(T& result)
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



//=====================================================================
} // namespace lockfree
} // namespace atma
//=====================================================================
#endif
//=====================================================================
