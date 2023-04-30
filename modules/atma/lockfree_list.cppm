module;

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <atomic>

export module atma.lockfree_list;

namespace atma::detail
{
	struct alignas(16) lockfree_list_pointer_t
	{

	};
}

namespace atma::detail
{
	template <typename T>
	struct lockfree_list_node_t
	{
		using buf_t = std::byte[sizeof(T)];

		lockfree_list_node_t() = default;
		lockfree_list_node_t(lockfree_list_node_t const&) = delete;

		lockfree_list_node_t(auto&& value)
		{
			new (value_buffer) T{std::forward<decltype(value)>(value)};
		}

		~lockfree_list_node_t()
		{
			// something something?
		}

		auto value_ref() -> T& { return *(T*)value_buffer; }

		// next pointer, 16-byte aligned so we can use 128-bit CAS,
		// as we need to compare against the ref-count at the same
		// time that we exchange the pointer
		struct alignas(16) pointer_t
		{
			lockfree_list_node_t<T>* alignas(8) next = nullptr;
			uint32_t tag = 0;
			uint32_t ref_count = 0;
		} next;

		// value
		alignas(alignof(T)) buf_t value_buffer;
	};
}




// lockfree-list
namespace atma
{
	template <typename T>
	struct lockfree_list_t
	{
		using node_t = detail::lockfree_list_node_t<T>;
		//using iterator = detail::lockfree_list_iterator_t<T>;

		lockfree_list_t();

		auto push_back(T const& x) -> T&;

		//auto erase(iterator const&) -> void;

		//auto begin() const -> iterator;
		//auto end() const -> iterator;

	private:
		std::atomic<node_t*> head_;
		std::atomic<node_t*> tail_;

		//template <typename> friend struct ::atma::detail::lockfree_list_iterator_t;
	};
}




#if 0
export namespace atma
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

		node_t* tail = tail_.load(); //atomic_load(&tail_);
		//while (!atomic_compare_exchange(&tail->next, (node_t*)nullptr, new_node))
		//	tail = atomic_load(&tail_);
		std::atomic_compare_exchange_strong(&tail->next)


		while (!atomic_compare_exchange(&tail_, tail, new_node))
			;

		return new_node->value_ref();
	}
}
#endif