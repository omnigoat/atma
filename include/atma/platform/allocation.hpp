#pragma once

#include <atma/config/platform.hpp>

import atma.types;

namespace atma { namespace platform {

	inline auto allocate_aligned_memory(size_t align, size_t size) -> void*
	{
		//ATMA_ASSERT(align >= sizeof(void*));
		
		if (size == 0) {
			return nullptr;
		}

#ifdef ATMA_PLATFORM_WINDOWS
		return _aligned_malloc(size, align);
#else
		return nullptr;
#endif
	}


	inline auto deallocate_aligned_memory(void *ptr) -> void
	{
#ifdef ATMA_PLATFORM_WINDOWS
		_aligned_free(ptr);
#endif
	}

} }
