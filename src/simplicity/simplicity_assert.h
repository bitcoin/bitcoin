#ifndef SIMPLICITY_SIMPLICITY_ASSERT_H
#define SIMPLICITY_SIMPLICITY_ASSERT_H

#include <assert.h>

/* Disable NDEBUG mode. */
#if (defined NDEBUG) && (!defined RECKLESS)
# error "Don't be RECKLESS.  Turn off NDEBUG when building.  For production builds use PRODUCTION."
#endif

/* Set a PRODUCTION_FLAG value based on whether PRODUCTION mode is enabled or not. */
#ifdef PRODUCTION
#  define PRODUCTION_FLAG 1
#else
#  define PRODUCTION_FLAG 0
#endif

/* Currently Simplicity's assert is the same a C's assert. */
#define simplicity_assert assert

/* simplicity_debug_assert is for assertions to be removed in PRODUCTION mode.
 * We use an if statement instead of conditional compilation to ensure the condition is type checked, even in PRODUCTION mode.
 */
#define simplicity_debug_assert(cond) do { if (!PRODUCTION_FLAG) { assert(cond); } } while(0)

/* Defines an UNREACHABLE macro that, if you manage to get into NDEBUG mode, calls '__builtin_unreachable' if it exists. */
#if (defined NDEBUG) && (defined __hasbuiltin)
#  if __has_builtin(__builtin_unreachable)
#    define SIMPLICITY_UNREACHABLE() __builtin_unreachable()
#  endif
#endif

#ifndef SIMPLICITY_UNREACHABLE
#  define SIMPLICITY_UNREACHABLE assert(NULL == "SIMPLICITY_UNCREACHABLE was reached")
#endif

#endif /* SIMPLICITY_SIMPLICITY_ASSERT_H */
