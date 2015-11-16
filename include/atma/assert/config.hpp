#pragma once

#if _DEBUG
#	define ATMA_ENABLE_ASSERTS 1
#else
#	define ATMA_ENABLE_ASSERTS 0
#endif

#ifdef ATMA_NO_ASSERT
#	undef ATMA_ENABLE_ASSERTS
#	define ATMA_ENABLE_ASSERTS 0
#endif
