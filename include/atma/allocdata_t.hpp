#pragma once

#include <type_traits>


namespace atma
{
	namespace detail
	{
		template <typename Alloc, bool Empty = std::is_empty<Alloc>::value>
		struct base_allocdata_t;

		template <typename Alloc>
		struct base_allocdata_t<Alloc, false>
		{
			base_allocdata_t()
			{}

			base_allocdata_t(Alloc& allocator)
				: allocator_(allocator)
			{}

			base_allocdata_t(base_allocdata_t&& rhs)
				: allocator_(std::move(rhs.allocator_))
			{}

			auto allocator() -> Alloc& { return allocator_; }

		private:
			Alloc allocator_;
		};

		template <typename Alloc>
		struct base_allocdata_t<Alloc, true>
			: protected Alloc
		{
			base_allocdata_t()
			{
			}

			base_allocdata_t(Alloc& allocator)
				: Alloc(allocator)
			{
			}

			base_allocdata_t(base_allocdata_t&& rhs)
				: Alloc(std::move(rhs))
			{}

			auto allocator() -> Alloc& { return static_cast<Alloc&>(*this); }
		};
	}




	template <typename Alloc>
	struct allocdata_t : detail::base_allocdata_t<Alloc>
	{
		using value_type = typename Alloc::value_type;


		allocdata_t()
		{}

		allocdata_t(Alloc& allocator)
			: detail::base_allocdata_t(allocator)
		{}

		allocdata_t(allocdata_t&& rhs)
			: detail::base_allocdata_t<Alloc>(std::move(rhs))
			, data_(rhs.detach_data())
		{
		}

		auto data() -> value_type* { return data_; }
		auto data() const -> value_type const* { return data_; }

		auto alloc(size_t capacity) -> void;
		auto deallocate() -> void;
		auto construct_default(size_t offset, size_t count) -> void;
		auto construct_copy(size_t offset, value_type const&) -> void;
		auto construct_copy_range(size_t offset, value_type const*, size_t size) -> void;
		auto construct_move(size_t offset, value_type&&) -> void;
		auto destruct(size_t offset, size_t count) -> void;

		auto memmove(size_t dest, size_t src, size_t count) -> void;
		auto memcpy(size_t dest, allocdata_t const&, size_t src, size_t count) -> void;

		auto detach_data() -> value_type*;

	private:
		value_type* data_;
	};




	template <typename A>
	auto allocdata_t<A>::alloc(size_t capacity) -> void
	{
		data_ = allocator().allocate(capacity);
	}

	template <typename A>
	auto allocdata_t<A>::deallocate() -> void
	{
		return allocator().deallocate(data_, 0);
	}

	template <typename A>
	auto allocdata_t<A>::construct_default(size_t offset, size_t count) -> void
	{
		for (auto i = offset; i != offset + count; ++i)
			allocator().construct(data_ + i);
	}

	template <typename A>
	auto allocdata_t<A>::construct_copy(size_t offset, value_type const& x) -> void
	{
		allocator().construct(data_ + offset, x);
	}

	template <typename A>
	auto allocdata_t<A>::construct_copy_range(size_t offset, value_type const* rhs, size_t size) -> void
	{
		for (auto i = 0, j = offset; i != size; ++i, ++j)
			allocator().construct(data_ + j, rhs[i]);
	}

	template <typename A>
	auto allocdata_t<A>::construct_move(size_t offset, value_type&& rhs) -> void
	{
		allocator().construct(data_ + offset, std::move(rhs));
	}

	template <typename A>
	auto allocdata_t<A>::destruct(size_t offset, size_t count) -> void
	{
		for (auto i = offset; i != offset + count; ++i)
			allocator().destroy(data_ + i);
	}

	template <typename A>
	auto allocdata_t<A>::memmove(size_t dest, size_t src, size_t count) -> void
	{
		std::memmove(data_ + dest, data_ + src, sizeof(value_type) * count);
	}

	template <typename A>
	auto allocdata_t<A>::memcpy(size_t dest, allocdata_t const& rhs, size_t src, size_t count) -> void
	{
		std::memcpy(data_ + dest, rhs.data() + src, sizeof(value_type) * count);
	}

	template <typename A>
	auto allocdata_t<A>::detach_data() -> value_type*
	{
		auto tmp = data_;
		data_ = nullptr;
		return tmp;
	}
}
