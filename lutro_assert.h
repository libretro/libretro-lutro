#ifndef LUTRO_ASSERT_H
#define LUTRO_ASSERT_H

// setup some sane defaults for assertions - these should ideally be specified explicitly by the build system.

// Disabling Errors is strongly discouraged. This option only exists for build system troubleshooting.
#ifndef LUTRO_ENABLE_ERROR
#define LUTRO_ENABLE_ERROR   1
#endif

// Recommended: Disable alerts on Player Builds.
#ifndef LUTRO_ENABLE_ALERT
#define LUTRO_ENABLE_ALERT   1
#endif

#ifndef LUTRO_ENABLE_ASSERT_TOOL
#define LUTRO_ENABLE_ASSERT_TOOL    1
#endif

#if !defined(LUTRO_ENABLE_ASSERT_DBG)
#  if defined(NDEBUG)
#    define LUTRO_ENABLE_ASSERT_DBG   0
#  else
#    define LUTRO_ENABLE_ASSERT_DBG   1
#  endif
#endif

#if !defined(__clang__)
#  if defined(_MSC_VER)
#     define __builtin_assume(cond)    __assume(cond)
#  else
      // GCC has __builtin_unreachable, but it's not really a 1:1 match for __builtin_assume since the assume
      // directive has special rules about not evaluating conditions that might have side effects.
#     define __builtin_assume(cond)    ((void)(0 && (cond)))
#  endif
#endif

#if defined(__clang__)
#   define VERIFY_FORMAT(cmd,fmt,valist) __attribute__((format(cmd,fmt,valist)))
#else
#   define VERIFY_FORMAT(cmd,fmt,valist)
#endif

typedef enum
{
   AssertUnrecoverable,
   AssertIgnorable,
   AssertErrorOnly
} AssertionType;

// Some macro engineering notes:
//  - the disabled versions of assertions still include the cond, gated behind `0 &&` which makes the cond unreachable.
//    this is done to enforce syntax checking of the contents of the assertion conditional even in release builds.

extern int _lutro_assertf_internal(int ignorable, const char *fmt, ...) VERIFY_FORMAT(printf, 2, 3);

#define _base_hard_error()                            ((void)(          ((_lutro_assertf_internal(AssertUnrecoverable, __FILE__ "(%d): unrecoverable error "        "\n", __LINE__                        ),1) && (abort(), 0))))
#define _base_hard_errorf(msg, ...)                   ((void)(          ((_lutro_assertf_internal(AssertUnrecoverable, __FILE__ "(%d): unrecoverable error "    msg "\n", __LINE__,         ## __VA_ARGS__),1) && (abort(), 0))))
#define _base_soft_alert()                            ((void)(          ((_lutro_assertf_internal(AssertIgnorable,     __FILE__ "(%d): alert "                      "\n", __LINE__                        )  ) && (abort(), 0))))
#define _base_soft_alertf(msg, ...)                   ((void)(          ((_lutro_assertf_internal(AssertIgnorable,     __FILE__ "(%d): alert "                  msg "\n", __LINE__,         ## __VA_ARGS__)  ) && (abort(), 0))))
#define _base_soft_error()                            ((void)(          ((_lutro_assertf_internal(AssertErrorOnly,     __FILE__ "(%d): error "                      "\n", __LINE__                        ),0)                )))
#define _base_soft_errorf(msg, ...)                   ((void)(          ((_lutro_assertf_internal(AssertErrorOnly,     __FILE__ "(%d): error "                  msg "\n", __LINE__,         ## __VA_ARGS__),0)                )))
#define _base_cond_error(cond)                        ((void)((cond) || ((_lutro_assertf_internal(AssertErrorOnly,     __FILE__ "(%d): assertion `%s` failed. "     "\n", __LINE__, #cond                 ),0)                )))
#define _base_cond_errorf(cond, msg, ...)             ((void)((cond) || ((_lutro_assertf_internal(AssertErrorOnly,     __FILE__ "(%d): assertion `%s` failed. " msg "\n", __LINE__, #cond , ## __VA_ARGS__),0)                )))
                                                                                                                                       
#define _base_hard_assert(cond)                       ((void)((cond) || ((_lutro_assertf_internal(AssertUnrecoverable, __FILE__ "(%d): assertion `%s` failed. "     "\n", __LINE__, #cond                 ),1) && (abort(), 0))))
#define _base_hard_assertf(cond, msg, ...)            ((void)((cond) || ((_lutro_assertf_internal(AssertUnrecoverable, __FILE__ "(%d): assertion `%s` failed. " msg "\n", __LINE__, #cond , ## __VA_ARGS__),1) && (abort(), 0))))
#define _base_soft_assert(cond)                       ((void)((cond) || ((_lutro_assertf_internal(AssertIgnorable,     __FILE__ "(%d): assertion `%s` failed. "     "\n", __LINE__, #cond                 )  ) && (abort(), 0))))
#define _base_soft_assertf(cond, msg, ...)            ((void)((cond) || ((_lutro_assertf_internal(AssertIgnorable,     __FILE__ "(%d): assertion `%s` failed. " msg "\n", __LINE__, #cond , ## __VA_ARGS__)  ) && (abort(), 0))))

// lutro error reporting. Errors come in two forms:
//  - error / alert /fail (no condition is provided in the paramters)
//  - assertions (a condition is provided in the parameters)
// 
// Assertions are meants to be compiled out in Release/Player builds. These constructs are generally only useful
// for internal debugging and some tooling debugging uses. They are meant to be used when the condition check
// itself would cause degredation of performance unnecessarily for the user (Player builds).
// 
// Errors are unconditional, in the sense that the condition must be specified normally by the programmer.
// The conditional behavior does not change depending on build type. Errors are useful in game development as
// the majority of "errors" are non-fatal situations that impact gameplay in varying degrees, ranging from
// simple missing visual/audio fx to perhaps buggy gameplay. It sbetter to just keep playing than to crash.
// 
// Summary of Macros:
//  - lutro_errorf is a logging construct only. It does not stop play in any build. It intentionally does not
//    include built-in conditional checks because it is expected the condition is always performed and that
//    execution continues past the error.
//
//  - lutro_alertf stops execution and issues a popup to the developer in Tool/Debug builds, which can be ignored
//    or turned into a crash report. In player builds it logs and (optionally) may record telemetry info to
//    allow debugging and fixing "misbehaves" later on.
//
//  - lutro_failf stops execution in ALL builds. It issues a popup to the developer in Tool/Debug builds, and 
//    goes straight to a crash dump report in player builds.
//

#if LUTRO_ENABLE_ERROR
#  define lutro_error()                      _base_soft_error  ()
#  define lutro_errorf(msg, ...)             _base_soft_errorf (msg, ## __VA_ARGS__)
#  define lutro_fail()                       _base_hard_error  ()     
#  define lutro_failf(msg, ...)              _base_hard_errorf (msg, ## __VA_ARGS__)
#else
#  define lutro_error()                      ((void)0)
#  define lutro_errorf(msg, ...)             ((void)0)
#  define lutro_fail()                       (__builtin_assume(0))
#  define lutro_failf(msg, ...)              (__builtin_assume(0))
#endif

#if LUTRO_ENABLE_ALERT
#  define lutro_alert()                      _base_soft_alert   ()
#  define lutro_alertf(msg, ...)             _base_soft_alertf  (       msg, ## __VA_ARGS__)
#elif LUTRO_ENABLE_ERROR
#  define lutro_alert()                      _base_soft_error   ()
#  define lutro_alertf(msg, ...)             _base_soft_errorf  (       msg, ## __VA_ARGS__)
#else
#  define lutro_alert()                      ((void)0))
#  define lutro_alertf(msg, ...)             ((void)0))
#endif

// tool_assert is meant to be conditionally compiled out on player builds. The main justifications to use tool_assert:
//  - for performance reasons, eg. if a play_errorf is known to be a performance issue
//  - to avoid surfacing errors that only make sense from the context of content creation, or simply don't
//    make sense in the context of the player
//
// tool_assume is a "hardened" version of assert, and is not ignorable.
#if LUTRO_ENABLE_ASSERT_TOOL
#  define tool_assert(cond)                 _base_soft_assert (cond)
#  define tool_assertf(cond, msg, ...)      _base_soft_assertf(cond, msg, ## __VA_ARGS__)
#  define tool_assume(cond)                 _base_hard_assert (cond)
#  define tool_assumef(cond, msg, ...)      _base_hard_assertf(cond, msg, ## __VA_ARGS__)
#else                                       
#  define tool_assert(cond)                 (__builtin_assume(cond))
#  define tool_assertf(cond, msg, ...)      (__builtin_assume(cond))
#  define tool_assume(cond)                 (__builtin_assume(cond))
#  define tool_assumef(cond, msg, ...)      (__builtin_assume(cond))
#endif

// dbg_assert is enabled only in debug builds, and is for use only in performance-critical code paths where
// the overhead of the assertion check should be avoided. Use of this macro should be limited only to specific
// situations where a tool_assert is actually observed to be a performance problem, or when it's plainly
// obvious that it might do so (a good example would be assertions within the audio mixer loop).
//
// dbg_assume is a "hardened" version of assert, and is not ignorable.
#if LUTRO_ENABLE_ASSERT_DBG
#  define dbg_assert(cond)                  _base_soft_assert (cond)
#  define dbg_assertf(cond, msg, ...)       _base_soft_assertf(cond, msg, ## __VA_ARGS__)
#  define dbg_assume(cond)                  _base_hard_assert (cond)
#  define dbg_assumef(cond, msg, ...)       _base_hard_assertf(cond, msg, ## __VA_ARGS__)
#else
#  define dbg_assert(cond)                  (__builtin_assume(cond))
#  define dbg_assertf(cond, msg, ...)       (__builtin_assume(cond))
#  define dbg_assume(cond)                  (__builtin_assume(cond))
#  define dbg_assumef(cond, msg, ...)       (__builtin_assume(cond))
#endif

#endif
