#pragma once
//=====================================================================
#include <atma/config/platform.hpp>

#ifdef ATMA_PLATFORM_WIN32
#	define ATMA_DEBUGBREAK() DebugBreak()
#endif

#include <atma/assert/config.hpp>
#include <atma/assert/basic.hpp>
#include <atma/assert/ensure.hpp>
#include <atma/assert/handling.hpp>
