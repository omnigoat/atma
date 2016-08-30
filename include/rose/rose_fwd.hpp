#pragma once

#include <atma/types.hpp>
#include <atma/bitmask.hpp>


namespace rose
{
	enum class file_access_t : uint
	{
		read,
		write,
		exclusive,
		nonbacked,
	};

	ATMA_BITMASK(file_access_mask_t, file_access_t);
}
