#pragma once

// ENABLE_PRINTF_VERIFY_CHECK_ON_MSVC
//
// Microsoft doesn't provide a way to annoate custom-rolled user printf() functions for static analysis.
// Well, ok -- it does provide _Printf_format_string_ but that's only effective when using the Code Analysis
// tool which is both incrediously slow and generates a hundred false positives for every valid issue. In
// other words, useless.
//
// The upside is MSVC does perform automatic lightweight static analysis on printf() builtin functions as part
// of the regular build process.  So I did a horrible thing and I built a macro that fake-calls MSVC's snprintf()
// on the input string, implemented as the second half of a nullified conditional such that it never _actually_
// get run. All tokens from the format macro do get pasted twice as part of this process, though only one of
// the pastes is reachable.  Behavior of the __COUNTER__ macro in this case would change.  No other program
// behavior should be affected. --jstine
//

#if !defined(ENABLE_PRINTF_VERIFY_CHECK_ON_MSVC)
#	if defined(_MSC_VER)
#		define ENABLE_PRINTF_VERIFY_CHECK_ON_MSVC	1
#	else
#		define ENABLE_PRINTF_VERIFY_CHECK_ON_MSVC	0
#	endif
#endif

#if !defined(VERIFY_PRINTF_ON_MSVC)
#	if ENABLE_PRINTF_VERIFY_CHECK_ON_MSVC
#		define VERIFY_PRINTF_ON_MSVC(...)	(0 && snprintf(nullptr, 0, ## __VA_ARGS__))
#		if defined(__cplusplus)
			// in C++ we can allow macros that optionally take a format parameter, typical of macros that
			// translate an empty format into a newline or such.
			static inline int snprintf(const char* fmt, int len) {
				return 0;
			}
#		endif
#	else
#		define VERIFY_PRINTF_ON_MSVC(...)	(0)
#	endif
#endif
