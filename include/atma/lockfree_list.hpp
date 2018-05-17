#pragma once

#include <atma/atomic.hpp>

#if 0
// forward-declares
namespace atma
{
	template <typename T>
	struct lockfree_list_t;
}

namespace atma::detail
{
	template <typename T>
	struct lockfree_list_node_t;

	template <typename T>
	struct lockfree_list_iterator_t;
}



// list-node
namespace atma::detail
{
	template <typename T>
	struct lockfree_list_node_t
	{
		using buf_t = byte[sizeof(T)];

		lockfree_list_node_t() = default;
		lockfree_list_node_t(lockfree_list_node_t const&) = delete;

		template <typename TT>
		lockfree_list_node_t(TT&& value)
		{
			new (value_buffer) T{std::forward<TT>(value)};
		}

		~lockfree_list_node_t()
		{
			// something something
		}

		
		// OKAY HERE WE GO
		auto load_next() -> lockfree_list_node_t<T>*
		{

		}

		auto value_ref() -> T& { return *(T*)value_buffer; }

		// 16 bytes
		lockfree_list_node_t<T>* alignas(8) next = nullptr;
		mutable uint32 ref_count = 0;
		uint32 pad = 0;

		// value
		buf_t value_buffer;
	};
}

// lockfree-list-iterator
namespace atma::detail
{
	template <typename T>
	struct lockfree_list_iterator_t
	{
		// iterator interface 
		using difference_type_t = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::forward_iterator_tag;

		using node_t = lockfree_list_node_t<T>;

		lockfree_list_iterator_t() = default;
		lockfree_list_iterator_t(lockfree_list_iterator_t const&) = delete;
		lockfree_list_iterator_t(lockfree_list_t<T> const& owner);

		auto operator ++()    -> lockfree_list_iterator_t&;
		auto operator ++(int) -> lockfree_list_iterator_t;

	private:
		node_t* x_ = nullptr;

		template <typename> friend struct ::atma::lockfree_list_t;
	};
}

// lockfree-list
namespace atma
{
	template <typename T>
	struct lockfree_list_t
	{
		using node_t = detail::lockfree_list_node_t<T>;
		using iterator = detail::lockfree_list_iterator_t<T>;

		lockfree_list_t();

		auto push_back(T const& x) -> T&;

		auto erase(iterator const&) -> void;

		auto begin() const -> iterator;
		auto end() const -> iterator;

	private:
		node_t* head_ = nullptr;
		node_t* tail_ = nullptr;

		template <typename> friend struct ::atma::detail::lockfree_list_iterator_t;
	};
}















//
//  IMPLEMENTATION
//

// lockfree-list-iterator
namespace atma::detail
{
	template <typename T>
	lockfree_list_iterator_t<T>::lockfree_list_iterator_t(lockfree_list_t<T> const& owner)
		: x_{atomic_load(&owner.head_)}
	{}

	template <typename T>
	auto lockfree_list_iterator_t<T>::operator ++() -> lockfree_list_iterator_t<T>&
	{
		ATMA_ASSERT(x_);
		x_ = atomic_load(&x_->next);
		return *this;
	}

	template <typename T>
	auto lockfree_list_iterator_t<T>::operator ++(int) -> lockfree_list_iterator_t<T>
	{
		auto self = *this;
		this->operator ++();
		return self;
	}
}


// lockfree-list
namespace atma
{
	template <typename T>
	lockfree_list_t<T>::lockfree_list_t()
		: head_{new node_t{}}
		, tail_{head_}
	{}

	template <typename T>
	auto lockfree_list_t<T>::push_back(T const& x) -> T&
	{
		auto new_node = new detail::lockfree_list_node_t<T>{x};

		auto tail = atomic_load(&tail_);
		while (!atomic_compare_exchange(&tail->next, (node_t*)nullptr, new_node))
			tail = atomic_load(&tail_);

		while (!atomic_compare_exchange(&tail_, tail, new_node))
			;

		return new_node->value_ref();
	}

	template <typename T>
	auto lockfree_list_t<T>::erase(iterator const& w) -> bool
	{
		auto head = atomic_load(&head_);
		auto head_next = atomic_load(&head->next);

		while (head_next != nullptr && head_next != w.x_) {
			ref_counted_traits<node_t>::rm_ref(head);
			head = head_next;
			head_next = atomic_load(&head->next);
			ref_counted_traits<node_t>::add_ref(head_next);
		}

		// nope
		if (head_next == nullptr)
			return false;

		auto head_next_next = atomic_load(&head_next->next);

		if (!atomic_compare_exchange(&head->next, head_next, head_next_next))
			return false;

		ref_counted_traits<node_t>::rm_ref(head_next);
	}

	template <typename T>
	auto lockfree_list_t<T>::begin() const -> iterator
	{
		// we must 
		return iterator{*this};
	}

	template <typename T>
	auto lockfree_list_t<T>::end() const -> iterator
	{
		return iterator{};
	}
}
#endif
