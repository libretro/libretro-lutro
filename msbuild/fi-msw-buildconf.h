#pragma once

#define PLATFORM_MSW   1

#if defined(_MSC_VER) && !defined(_ITERATOR_DEBUG_LEVEL)
#	define _ITERATOR_DEBUG_LEVEL   0
#endif

#if defined(_MSC_VER) 
#define _CRT_NONSTDC_NO_WARNINGS   1
#endif

// putting some clangcl specific warning controls here rather than fi-warnings since they're exclusively clangcl (msbuild).
#if defined(__clang__)
#	pragma GCC diagnostic ignored "-Wmicrosoft-include"
#endif

