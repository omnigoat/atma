#pragma once

#include "aligned_allocator.hpp"
#include "hash.hpp"

namespace atma
{
	namespace detail
	{
		template <typename K, typename V, typename T>
		struct hash_map_node
		{
			
		};
	}

	template <typename K, typename V>
	struct hash_map_traits_t
	{
		using hash_t   = atma::hash_t<K>;
		using eq_t     = atma::eq_t<K>;
		using allock_t = atma::aligned_allocator_t<K, 4>;
		using allocv_t = atma::aligned_allocator_t<V, 4>;

		static bool const cache_hash = false;
	};

	template <typename K, typename V, typename Traits = hash_map_traits_t<K, V>>
	struct hash_map
	{
		
	private:
		
	};
}
