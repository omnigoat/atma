#pragma once

#include <type_traits>

import atma.types;

// ebo-pair
namespace atma::detail
{
	template <
		typename First, typename Second,
		template <typename, typename> typename StorageTransformer,
		bool = std::is_empty_v<First>, bool = std::is_empty_v<Second>
	>
	struct ebo_pair_impl;

	template <typename First, typename Second, template <typename, typename> typename StorageTransformer>
	struct ebo_pair_impl<First, Second, StorageTransformer, false, false>
	{
		using first_type = typename StorageTransformer<First, Second>::first_type;
		using second_type = typename StorageTransformer<First, Second>::second_type;

		constexpr explicit(is_implicitly_default_constructible_v<First> && is_implicitly_default_constructible_v<Second>)
		ebo_pair_impl() = default;

		ebo_pair_impl(ebo_pair_impl const&) = default;
		ebo_pair_impl(ebo_pair_impl&&) = default;
		~ebo_pair_impl() = default;

		template <typename F, typename S>
		constexpr ebo_pair_impl(F&& first, S&& second)
			: first_(first), second_(second)
		{}

		first_type& first() { return first_; }
		second_type& second() { return second_; }
		first_type const& first() const { return first_; }
		second_type const& second() const { return second_; }

	private:
		first_type first_;
		second_type second_;
	};

	template <typename First, typename Second, template <typename, typename> typename StorageTransformer>
	struct ebo_pair_impl<First, Second, StorageTransformer, true, false>
		: protected First
	{
		using first_type = typename StorageTransformer<First, Second>::first_type;
		using second_type = typename StorageTransformer<First, Second>::second_type;

		constexpr ebo_pair_impl() = default;
		ebo_pair_impl(ebo_pair_impl const&) = default;
		ebo_pair_impl(ebo_pair_impl&&) = default;
		~ebo_pair_impl() = default;

		template <typename F, typename S>
		ebo_pair_impl(F&& first, S&& second)
			: First(first), second_(second)
		{}

		first_type& first() { return static_cast<first_type&>(*this); }
		second_type& second() { return second_; }
		first_type const& first() const { return static_cast<first_type const&>(*this); }
		second_type const& second() const { return second_; }

	private:
		second_type second_;
	};

	template <typename First, typename Second, template <typename, typename> typename Tr>
	struct ebo_pair_impl<First, Second, Tr, false, true>
		: protected First
	{
		using first_type = First;
		using second_type = Second;

		constexpr ebo_pair_impl() = default;
		ebo_pair_impl(ebo_pair_impl const&) = default;
		ebo_pair_impl(ebo_pair_impl&&) = default;
		~ebo_pair_impl() = default;

		template <typename F, typename S>
		ebo_pair_impl(F&& first, S&& second)
			: Second(second), first_(first)
		{}

		First& first() { return first_; }
		Second& second() { return static_cast<Second&>(*this); }
		First const& first() const { return first_; }
		Second const& second() const { return static_cast<Second const&>(*this); }

	private:
		typename Tr<First, Second>::first_type first_;
	};

	// if they're both zero-size, just inherit from second
	template <typename First, typename Second, template <typename, typename> typename Tr>
	struct ebo_pair_impl<First, Second, Tr, true, true>
		: ebo_pair_impl<First, Second, Tr, false, true>
	{};
}

namespace atma
{
	template <typename First, typename Second>
	struct default_storage_transformer_t
	{
		using first_type = First;
		using second_type = Second;
	};

	template <typename First, typename Second>
	struct first_as_reference_transformer_t
	{
		using first_type = First&;
		using second_type = Second;
	};

	template <typename First, typename Second, template <typename, typename> typename Transformer = default_storage_transformer_t>
	struct ebo_pair_t
		: detail::ebo_pair_impl<First, Second, Transformer>
	{
		using base_type = detail::ebo_pair_impl<First, Second, Transformer>;

		using base_type::base_type;

		ebo_pair_t& operator = (ebo_pair_t const& rhs)
		{
			this->first() = rhs.first();
			this->second() = rhs.second();
			return *this;
		}
	};
}


