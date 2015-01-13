#pragma once

#include <atomic>
#include <iostream>

namespace atma { namespace lockfree {

	
	// default cache-line size will be 64 bytes
	const unsigned int cache_line_size = 64;


	//=====================================================================
	// detail::node_t
	// ----------------
	//   provides small-object-optimization
	//=====================================================================
	namespace detail
	{
		template <typename T>
		struct node_size_t {
			static size_t const padding = sizeof(T) + sizeof(std::atomic<void*>) % cache_line_size;
			static bool const is_small = sizeof(T) + sizeof(std::atomic<void*>) <= cache_line_size;
		};


		template <typename T, bool = node_size_t<T>::is_small>
		struct node_t
		{
			node_t() : value(nullptr), next(nullptr) {}
			node_t(const T& value) : value(new T(value)), next(nullptr) {}
			~node_t() { }
			auto value_ptr() -> T * { return value; }
			auto value_ptr() const -> T const* { return value; }
			auto clear_value_ptr() -> void { value = nullptr; }

			T* value;
			std::atomic<node_t*> next;
			char pad[node_size_t<T*>::padding];
		};


		template <typename T>
		struct node_t<T, true>
		{
			node_t() : next(nullptr) {}
			node_t(T const& value) : next(nullptr) {
				new (value_buffer) T(value);
			}
			~node_t() {}

			auto value_ptr() -> T * { return reinterpret_cast<T*>(value_buffer); }
			auto value_ptr() const -> T const* { return reinterpret_cast<T const*>(value_buffer); }
			auto clear_value_ptr() -> void { value_ptr()->~T(); }

			char value_buffer[sizeof(T)];
			std::atomic<node_t*> next;
			char pad[node_size_t<T>::padding];
		};
	}




	//=====================================================================
	// queue_t
	//=====================================================================
	template <typename T>
	struct queue_t
	{
		struct batch_t;
		struct iterator;

		queue_t();
		~queue_t();

		auto begin() const->iterator;
		auto end() const->iterator;
		
		auto push(T const&) -> iterator;
		auto push(batch_t&) -> void;
		auto pop(T& result) -> bool;
		auto erase(iterator) -> void;
		auto erase(T const&) -> void;
		template <typename PR>
		auto erase_if(PR pred) -> void;

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

		friend struct iterator;
	};




	//=====================================================================
	// queue_t::iterator
	//=====================================================================
	template <typename T>
	struct queue_t<T>::iterator
	{
		iterator();

		auto operator ++() -> iterator&;
		auto operator *() -> T&;

		auto operator == (iterator const& rhs) -> bool;
		auto operator != (iterator const& rhs) -> bool;

	private:
		iterator(typename queue_t<T>::node_t* node);

		typename queue_t<T>::node_t* node_;

		friend struct queue_t<T>;
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
		auto begin() const -> node_t const*;
		auto end() const -> node_t const*;

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
	queue_t<T>::~queue_t()
	{
		while (head_ != nullptr)
		{
			node_t* tmp = head_;
			node_t* tmp2 = head_->next;
			head_ = tmp2;
			if (tmp2) {
				tmp2->clear_value_ptr();
			}
			delete tmp;
		}
	}

	template <typename T>
	auto queue_t<T>::push(T const& t) -> iterator
	{
		node_t* tmp = new node_t(t);
		while (producer_lock_.exchange(true))
			;
		tail_->next = tmp;
		tail_ = tmp;
		producer_lock_ = false;
		return iterator(tmp);
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
		b.head_->next = nullptr;
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
			head_next->clear_value_ptr();

			delete head;
			return true;
		}

		consumer_lock_ = false;
		return false;
	}

	template <typename T>
	auto queue_t<T>::begin() const -> iterator
	{
		return iterator(head_->next);
	}

	template <typename T>
	auto queue_t<T>::end() const -> iterator
	{
		return iterator(nullptr);
	}

	template <typename T>
	auto queue_t<T>::erase(iterator i) -> void
	{
		node_t* pr = nullptr;
		auto n = head_->next.load();
		while (n)
		{
			if (n == i.node_) {
				if (pr)
					pr->next.store(n->next.load());
				else
					head_ = n->next;
				break;
			}

			pr = n;
			n = n->next;
		}
	}




	//=====================================================================
	// queue_t::iterator implementation
	//=====================================================================
	template <typename T>
	queue_t<T>::iterator::iterator()
		: node_()
	{
	}

	template <typename T>
	queue_t<T>::iterator::iterator(typename queue_t::node_t* node)
		: node_(node)
	{
	}

	template <typename T>
	auto queue_t<T>::iterator::operator ++() -> iterator&
	{
		node_ = node_->next;
		return *this;
	}

	template <typename T>
	auto queue_t<T>::iterator::operator *() -> T&
	{
		return *node_->value_ptr();
	}

	template <typename T>
	inline auto queue_t<T>::iterator::operator == (iterator const& rhs) -> bool
	{
		return node_ == rhs.node_;
	}

	template <typename T>
	inline auto queue_t<T>::iterator::operator != (iterator const& rhs) -> bool
	{
		return node_ != rhs.node_;
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

	template <typename T>
	auto queue_t<T>::batch_t::begin() const -> node_t const*
	{
		return head_->next;
	}


	template <typename T>
	auto queue_t<T>::batch_t::end() const -> node_t const*
	{
		return nullptr;
	}


} }

