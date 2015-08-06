#pragma once

#include "aligned_allocator.hpp"
#include "hash.hpp"

namespace atma
{
	template <typename T>
	struct eq_t
	{
		auto operator ()(T const& lhs, T const& rhs) const -> bool
		{
			return lhs == rhs;
		}
	};

	namespace detail
	{
		template <typename K, typename V, typename T>
		struct hash_map_node_t
		{
			hash_map_node_t* next;
			K key;
			T value;
		};
	}

	template <typename K, typename V>
	struct hash_map_traits_t
	{
		using hash_t   = atma::hash_t<K>;
		using eq_t     = atma::eq_t<K>;
		using allock_t = atma::aligned_allocator_t<K, 4>;
		using allocv_t = atma::aligned_allocator_t<V, 4>;

		static int  const bucket_size = 8;

		static bool const cache_hash = false;
	};

	template <typename K, typename V, typename Traits = hash_map_traits_t<K, V>>
	struct hash_map
	{
		hash_map();

		auto add(K const& k, V const& v) -> void
		{
			auto hash = atma::hash(k);
			auto idx = hash % 64;
			
		}

	private:
		using node_t = detail::hash_map_node_t<K, V, Traits>;
		node_t* elements_;
	};


	template <typename K, typename V, typename T>
	inline hash_map<K, V, T>::hash_map()
		: elements_(new node_t[64 * 8])
	{}
}
