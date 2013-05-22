//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_ASSERT_CONFIG_HPP
#define ATMA_ASSERT_CONFIG_HPP
//=====================================================================
// setting up whether we assert or not
//=====================================================================
#if _DEBUG
#	define ATMA_ENABLE_ASSERTS
#else
#	define ATMA_ENABLE_ASSERTS
#endif

#ifdef ATMA_NO_ASSERT
#	undef ATMA_ENABLE_ASSERTS
#endif

//=====================================================================
#endif // inclusion guard
//=====================================================================
