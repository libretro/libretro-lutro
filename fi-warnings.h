#pragma once

// modify warning and error behavior of the compiler globally (applies to all files in project).
// These are easier to manage via header file than via makefile. Only some platforms may utilize
// this file to avoid spamming other platforms with warnings or errors related to unsupported pragmas.
// Please search the mafefile for fi-warnings.h to confirm its use.

#if defined(__GNUC__) || defined(__clang__)

#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#pragma GCC diagnostic ignored "-Wunused-parameter" 
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-value"              // happens in several lua macros.
#pragma GCC diagnostic ignored "-Wmisleading-indentation"

#pragma GCC diagnostic error "-Wreturn-type"                 // non-void function does not return a value. Should have always been an error.
#pragma GCC diagnostic error "-Wparentheses"                 // catches important operator precedence issues. More() is better, usually.

#if defined(__clang__)
#  pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

// printf format validation should be errors rather than warnings. The only possible exception is -Wformat-extra-args which itself is a
// low-risk formatting mistake that doesn't cause runtime failures, eg. extra agrs are simply unused. But in the context of string literal
// formatters, there is no reason to allow them (their use is restricted to variable or templated constexpr formatters that might opt
// to ignore some args depending on context and those would not trigger this error anyway).

#if __clang_major >= 13
#  pragma GCC diagnostic error "-Wformat"
#  pragma GCC diagnostic error "-Wformat-extra-args"
#  pragma GCC diagnostic error "-Wformat-insufficient-args"
#  pragma GCC diagnostic error "-Wformat-invalid-specifier"
#  pragma GCC diagnostic error "-Winconsistent-missing-override"
#endif

#elif defined(_MSC_VER)

#pragma warning(disable:4100)	// unreferenced formal parameter
#pragma warning(disable:4127)	// conditional expression is constant
#pragma warning(disable:4244)		// conversion from 'intmax_t' to 'lua_Number', possible loss of data

// Next section: Turn on Useful Warnings/Errors in MSC:

// 4359 highlights a situation where microsoft's pragma for packing behaves differently from gcc/clang.
#pragma warning(1      :4359) // Alignment specifier is less than actual alignment (4), and will be ignored

#pragma warning(error  :5031)	// #pragma warning(pop): likely mismatch, popping warning state pushed in different file
#pragma warning(error  :5032)	// detected #pragma warning(push) with no corresponding #pragma warning(pop)
#pragma warning(error  :4309)	// 'conversion': trucates constant value. promoted to error because these are almost always -serious- (and easy to avoid - add an explicit cast)
#pragma warning(error  :4311)	// pointer truncation from 'variable' to 'uint32_t'. Clang treats this as an error, for good reason. Can avoid with double-cast, eg. (int32_t)(intptr_t)

// Ennable all format validation warnings as errors
#pragma warning(error  :4473)	// not enough arguments passed for format string
#pragma warning(error  :4474)	// too many arguments passed for format string
#pragma warning(error  :4475)	// length modifier 'modifier' cannot be used with type field character 'character' in format specifier
#pragma warning(error  :4476)	// unknown type field character 'character' in format specifier
#pragma warning(error  :4477)	// format string '% d' requires an argument of type 'int', but variadic argument 1 has type 'uintmax_t'
#pragma warning(error  :4478)	// positional and non-positional placeholders cannot be mixed in the same format string

#endif
