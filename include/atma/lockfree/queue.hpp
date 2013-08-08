#ifndef ATMA_LOCKFREE_QUEUE_HPP
#define ATMA_LOCKFREE_QUEUE_HPP
//=====================================================================
#include <atomic>
#include <iostream>
//=====================================================================
namespace atma {
namespace lockfree {
//=====================================================================
	
	// default cache-line size will be 64 bytes
	const unsigned int cache_line_size = 64;

	//=====================================================================
	// detail::node_t
	// ----------------
	//   provides small-object optimisation
	//=====================================================================
	namespace detail
	{
		template <typename T, bool SMO = (sizeof(T) + sizeof(std::atomic_intptr_t) <= cache_line_size)>
		struct node_t;

		template <typename T>
		struct node_t<T, false>
		{
			node_t() : value(nullptr), next(nullptr) {}
			node_t(const T& value) : value(new T(value)), next(nullptr) {}
			~node_t() { delete value; }
			auto value_ptr() const -> T const* { return value; }

			T* value;
			std::atomic<node_t*> next;
			char pad[ (sizeof(T*) + sizeof(std::atomic<node_t*>)) % cache_line_size ];
		};

		template <typename T>
		struct node_t<T, true>
		{
			node_t() : next(nullptr) {}
			node_t(T const& value) : next(nullptr) { new (value_buffer) T(value); }
			~node_t() { value_ptr()->~T(); }
			auto value_ptr() const -> T const* { return reinterpret_cast<T const*>(&value_buffer[0]); }

			char value_buffer[sizeof(T)];
			std::atomic<node_t*> next;
			char pad[ (sizeof(T) + sizeof(std::atomic<node_t*>)) % cache_line_size ];
		};
	}


	//=====================================================================
	// queue_t
	//=====================================================================
	template <typename T>
	struct queue_t
	{
		struct batch_t;

		queue_t();
		~queue_t();

		auto push(const T& t) -> void;
		auto push(batch_t&) -> void;
		auto pop(T& result) -> bool;
		
	private:
		queue_t(queue_t const&);
		queue_t(queue_t&&);
		auto operator = (const queue_t&) -> queue_t&;
		auto operator = (queue_t&&) -> queue_t&;

		typedef detail::node_t<T> node_t;

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
	// queue_t::batch_t
	//=====================================================================
	template <typename T>
	struct queue_t<T>::batch_t
	{
		batch_t();
		~batch_t();

		auto push(T const&) -> batch_t&;


	public:
		typedef detail::node_t<T> node_t;

		node_t* head_, *tail_;

		friend struct queue_t<T>;
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
	void queue_t<T>::push(T const& t) {
		node_t* tmp = new node_t(t);
		while (producer_lock_.exchange(true))
			;
		tail_->next = tmp;
		tail_ = tmp;
		producer_lock_ = false;
	}

	template <typename T>
	auto queue_t<T>::push(batch_t& b) -> void {
		while (producer_lock_.exchange(true))
			;
		tail_->next = b.head_->next.load();
		tail_ = b.tail_;
		producer_lock_ = false;

		// clear batch
		b.tail_ = b.head_;
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
			T const* value = head_next->value_ptr();
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
	// queue_t::batch_t implementation
	//=====================================================================
	template <typename T>
	queue_t<T>::batch_t::batch_t()
	{
		head_ = tail_ = new node_t;
	}

	template <typename T>
	queue_t<T>::batch_t::~batch_t()
	{
		while (head_) {
			auto tmp = head_->next.load();
			delete head_;
			head_ = tmp;
		}
	}

	template <typename T>
	auto queue_t<T>::batch_t::push(T const& t) -> batch_t&
	{
		tail_->next = new node_t(t);
		tail_ = tail_->next;
		return *this;
	}




//=====================================================================
} // namespace lockfree
} // namespace atma
//=====================================================================
#endif
//=====================================================================
