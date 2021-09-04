#pragma once

#include "hash.hpp"
#include "math/functions.hpp"
#include "function_traits.hpp"
#include "meta.hpp"
#include <atma/assert.hpp>

#include <functional>
#include <array>

import atma.memory;

#if USE_SPP_HASH_MAP

#include <greg7mdp/sparsepp.hpp>

namespace atma
{
	template <
		typename Key,
		typename Value,
		typename HashFn = hash_t<Key>,
		typename EqFn = std::equal_to<Key>,
		typename Alloc = aligned_allocator_t<std::pair<Key const, Value>>>
	using hash_map = spp::sparse_hash_map<Key, Value, HashFn, EqFn, Alloc>;

	template <
		typename Value,
		typename HashFn = hash_t<Value>,
		typename EqFn = std::equal_to<Value>,
		typename Alloc = aligned_allocator_t<Value>>
	using hash_set = spp::sparse_hash_set<Value, HashFn, EqFn, Alloc>;
}


#else

namespace atma::detail
{
	struct use_self_t
	{
		template <typename T>
		decltype(auto) operator () (T&& x)
		{
			return std::forward<T>(x);
		}
	};

	struct use_first_t
	{
		template <typename T>
		decltype(auto) operator () (T&& x)
		{
			return (std::forward<T>(x).first);
		}
	};

	struct use_second_t
	{
		template <typename T>
		decltype(auto) operator () (T&& x)
		{
			return (x.second);
		}
	};


	template <
		typename key_type_tx,
		typename value_type_tx,
		typename key_extractor_tx,
		typename value_extractor_tx,
		typename hasher_tx,
		typename equalifier_tx,
		typename allocator_tx
	>
	struct hash_table
	{
		using key_t = key_type_tx;
		using value_t = value_type_tx;
		using key_extractor_t = key_extractor_tx;
		using value_extractor_t = value_extractor_tx;
		using hasher_t = hasher_tx;
		using equalifier_t = equalifier_tx;
		using allocator_type = allocator_tx;
		using value_type = value_type_tx;
		using payload_t = std::remove_reference_t<decltype(value_extractor_t{}(std::declval<value_t>()))>;
		using replacer_t = std::function<bool(payload_t&, payload_t const&)>;
		using insert_result_t = std::pair<payload_t*, bool>;

		hash_table(size_t buckets, size_t bucket_size, key_extractor_t, value_extractor_t, hasher_t = hash_t<key_t>{}, allocator_type const& = allocator_type());

		void reset(size_t buckets, size_t bucket_size, hasher_t = hash_t<key_t>{});

		void set_replacement_function(replacer_t const&);

		auto insert(value_type const&) -> insert_result_t;

		template <typename Replacer>
		auto insert_or_replace_with(value_type const&, Replacer&&) -> insert_result_t;

		auto find(key_t const&) -> payload_t*;

		template <typename F>
		auto for_all_in_same_bucket(key_t const&, F&&) -> void;

	private:
		using bucket_chain_elements_t = std::array<byte, sizeof(value_type) * 16>;

		struct bucket_chain_t;
		using bucket_chain_ptr = std::unique_ptr<bucket_chain_t>;

		struct bucket_chain_t
		{
			bucket_chain_t()
			{
				memset(elements.data(), 0, sizeof(value_type) * 16);
			}

			bucket_chain_elements_t elements;
			bucket_chain_ptr next_chain;
			uint16 filled = 0;

			auto at(int i) -> value_type& { return reinterpret_cast<value_type*>(elements.data())[i]; }
		};

		// rebind provided allocator for byte allocations
		using bucket_allocator_t = typename allocator_type::template rebind<bucket_chain_ptr>::other;

	private:
		key_extractor_t key_extractor_;
		value_extractor_t value_extractor_;
		hasher_t hasher_;
		equalifier_t equalifier_;
		size_t bucket_count_ = 0;
		size_t bucket_size_ = 0;
		size_t bucket_bitmask_ = 0;
		replacer_t replacer_;
		atma::basic_unique_memory_t<bucket_chain_ptr, bucket_allocator_t> buckets_;
	};

}




namespace atma::detail
{
#define hash_table_template_declaration template <typename K, typename V, typename KX, typename VX, typename H, typename E, typename A>
#define hash_table_type_instantiated hash_table<K,V,KX,VX,H,E,A>

	hash_table_template_declaration
	inline hash_table_type_instantiated::hash_table(size_t buckets, size_t bucket_size, key_extractor_t key_extractor, value_extractor_t value_extractor, hasher_t hasher, allocator_type const& alloc)
		: key_extractor_(key_extractor)
		, value_extractor_(value_extractor)
		, hasher_(std::move(hasher))
		, bucket_count_(buckets)
		, bucket_size_(bucket_size)
		, bucket_bitmask_(math::log2((uint)bucket_count_))
		, buckets_(bucket_count_ * sizeof(bucket_chain_ptr), alloc)
	{
		//buckets_.memory_operations().memzero(0, bucket_count_ * sizeof(bucket_chain_ptr));
		//memory::memzero()
		ATMA_ASSERT(math::is_pow2(buckets), "must use power-of-two for number of buckets");
	}

	hash_table_template_declaration
	inline auto hash_table_type_instantiated::reset(size_t buckets, size_t bucket_size, hasher_t hasher) -> void
	{
		// don't support anything except a 
		ATMA_ASSERT(buckets <= buckets_.size() / sizeof(bucket_chain_ptr) && bucket_size >= bucket_size_);

		// assign new values to memset the minimum required
		bucket_count_ = buckets;
		bucket_size_ = bucket_size;
		bucket_bitmask_ = math::log2((uint)bucket_count_);

		for (auto& bucket : atma::memory_view_t<bucket_chain_ptr>(buckets_, 0, bucket_count_ * sizeof(bucket_chain_ptr)))
		{
			if (!bucket)
				continue;

			memset(bucket->elements.data(), 0, sizeof(value_type) * 16);
			bucket->filled = 0;
			for (auto next = &bucket->next_chain; *next; next = &(*next)->next_chain)
			{
				memset((*next)->elements.data(), 0, sizeof(value_type) * 16);
				(*next)->filled = 0;
			}
		}
	}

	hash_table_template_declaration
	inline auto hash_table_type_instantiated::insert(value_type const& x) -> insert_result_t
	{
		return insert_or_replace_with(x, [](auto&&...) { return false; });
	}

	hash_table_template_declaration
	template <typename Replacer>
	inline auto hash_table_type_instantiated::insert_or_replace_with(value_type const& x, Replacer&& replacer) -> insert_result_t
	{
		auto&& key = key_extractor_(x);

		auto const hash = hasher_(key);
		auto const slot_idx = hash & bucket_bitmask_;
		ATMA_ASSERT(slot_idx < bucket_count_, "how did this happen?");

		bucket_chain_ptr* chain_ptr_ptr = buckets_.begin() + slot_idx;
		
		// first we must see if it compares to other values
		bucket_chain_ptr* available_chain = nullptr;
		int available_element_idx = -1;

		// begin scanning the bucket for matching entries
		int exhausted = 0;
		while (*chain_ptr_ptr)
		{
			auto& chain_ptr = *chain_ptr_ptr;

			for (int b = 1, element_idx = 0; b != 65536; b <<= 1, ++element_idx)
			{
				// we have a filled slot
				if ((chain_ptr->filled & b) == b)
				{
					// if we have a match...
					if (auto& candidate = chain_ptr->at(element_idx); equalifier_(key_extractor_(candidate), key) )
					{
						// replace the value and return the result, either by return-value or just false
						if constexpr (std::is_same_v<decltype(std::invoke(std::forward<Replacer>(replacer), value_extractor_(candidate), value_extractor_(x))), void>)
						{
							auto& value = value_extractor_(candidate);
							std::invoke(std::forward<Replacer>(replacer), value, value_extractor_(x));
							return {&value, false};
						}
						else
						{
							auto& value = value_extractor_(candidate);
							auto r = std::invoke(std::forward<Replacer>(replacer), value, value_extractor_(x));
							return {&value, r};
						}
					}

					++exhausted;
				}
				// we found an empty slot, let's record this in case we need to insert
				else if (available_chain == nullptr)
				{
					available_chain = &chain_ptr;
					available_element_idx = element_idx;
				}
			}

			chain_ptr_ptr = &chain_ptr->next_chain;
		}

		if (exhausted >= bucket_size_)
		{
			return {nullptr, false};
		}

		// insert the value to a location we earlier determined was empty
		value_type* addr = nullptr;
		if (available_chain != nullptr)
		{
			addr = &(*available_chain)->at(available_element_idx);
			new (addr) value_type(x);
			(*available_chain)->filled |= (1 << available_element_idx);
		}
		else
		{
			// we should have exhausted our search then
			ATMA_ASSERT(*chain_ptr_ptr == nullptr, "chain search was not exhaustive");

			// create new page in the chain and insert element at index 0
			chain_ptr_ptr->reset(new bucket_chain_t);
			addr = &(*chain_ptr_ptr)->at(0);
			new (addr) value_type(x);
			(*chain_ptr_ptr)->filled = 1;
		}

		return {&value_extractor_(*addr), true};
	}

	hash_table_template_declaration
	inline auto hash_table_type_instantiated::set_replacement_function(replacer_t const& replacer) -> void
	{
		replacer_ = replacer;
	}



	hash_table_template_declaration
	inline auto hash_table_type_instantiated::find(key_t const& key) -> payload_t*
	{
		auto const hash = hasher_(key);
		auto const slot_idx = hash & bucket_bitmask_;
		ATMA_ASSERT(slot_idx < bucket_count_, "how did this happen?");

		bucket_chain_ptr* chain_ptr_ptr = buckets_.begin() + slot_idx;

		// begin scanning the bucket for matching entries
		while (*chain_ptr_ptr)
		{
			auto& chain_ptr = *chain_ptr_ptr;

			for (int b = 1, element_idx = 0; b != 65536; b << 1, ++element_idx)
			{
				// we have a filled slot
				if ((chain_ptr->filled & b) == b)
				{
					// if we have a match...
					if (auto& candidate = chain_ptr->at(element_idx); equalifier_(key_extractor_(candidate), key))
					{
						auto& payload = value_extractor_(candidate);
						return &payload;
					}
				}
			}

			chain_ptr_ptr = &chain_ptr->next_chain;
		}

		return nullptr;
	}

	hash_table_template_declaration
	template <typename F>
	inline auto hash_table_type_instantiated::for_all_in_same_bucket(key_t const& key, F&& f) -> void
	{
		auto const hash = hasher_(key);
		auto const slot_idx = hash & bucket_bitmask_;
		ATMA_ASSERT(slot_idx < bucket_count_, "how did this happen?");

		bucket_chain_ptr* chain_ptr_ptr = buckets_.begin() + slot_idx;

		while (*chain_ptr_ptr)
		{
			auto& chain_ptr = *chain_ptr_ptr;

			for (int b = 1, element_idx = 0; b != 65536; b <<= 1, ++element_idx)
			{
				// we have a filled slot
				if ((chain_ptr->filled & b) == b)
				{
					if constexpr (std::is_invocable<F>)
					{
						std::invoke(std::forward<F>(f));
					}
					else if constexpr (std::is_invocable_r<void, F, value_type&>)
					{
						std::invoke(std::forward<F>(f), chain_ptr->at(element_idx));
					}
					else
					{
						static_assert(std::is_convertible_v<bool, std::invoke_result_t<F, value_type&>>,
							"result-type must be convertible to bool (return true to continue iteration)");

						if (!std::invoke(std::forward<F>(f), chain_ptr->at(element_idx)))
							return;
					}

				}
			}

			chain_ptr_ptr = &chain_ptr->next_chain;
		}
	}

}
























namespace atma
{
	template <
		typename value_tx,
		typename hasher_tx = hash_t<value_tx>,
		typename equalifier_tx = std::equal_to<value_tx>,
		typename allocator_tx = aligned_allocator_t<value_tx>
	>
	struct hash_set
		: detail::hash_table<value_tx, value_tx, detail::use_self_t, detail::use_self_t, hasher_tx, equalifier_tx, allocator_tx>
	{
		using base = detail::hash_table<value_tx, value_tx, detail::use_self_t, detail::use_self_t, hasher_tx, equalifier_tx, allocator_tx>;

		using typename base::key_t;
		using typename base::value_t;
		using typename base::key_extractor_t;
		using typename base::hasher_t;
		using typename base::equalifier_t;
		using typename base::allocator_type;
		using typename base::value_type;
		using typename base::replacer_t;

		hash_set(size_t buckets, size_t bucket_size, hasher_t hasher = hash_t<key_t>(), allocator_type const& allocator = allocator_type())
			: base(buckets, bucket_size, detail::use_self_t{}, detail::use_self_t{}, hasher, allocator)
		{}
	};


	template <
		typename key_tx,
		typename value_tx,
		typename hasher_tx = hash_t<key_tx>,
		typename equalifier_tx = std::equal_to<key_tx>,
		typename allocator_tx = aligned_allocator_t<std::pair<key_tx const, value_tx>>
	>
	struct hash_map
		: detail::hash_table<key_tx, std::pair<const key_tx, value_tx>, detail::use_first_t, detail::use_second_t, hasher_tx, equalifier_tx, allocator_tx>
	{
		using base = detail::hash_table<key_tx, std::pair<const key_tx, value_tx>, detail::use_first_t, detail::use_second_t, hasher_tx, equalifier_tx, allocator_tx>;

		using typename base::key_t;
		using typename base::value_t;
		using typename base::key_extractor_t;
		using typename base::hasher_t;
		using typename base::equalifier_t;
		using typename base::allocator_type;
		using typename base::value_type;
		using typename base::replacer_t;

		hash_map(size_t buckets, size_t bucket_size, hasher_t hasher = hash_t<key_t>(), allocator_type const& allocator = allocator_type())
			: base(buckets, bucket_size, detail::use_first_t{}, detail::use_second_t{}, hasher, allocator)
		{}
	};
}


#undef hash_table_template_declaration
#undef hash_table_type_instantiated

#endif
