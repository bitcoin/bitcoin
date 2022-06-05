// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_BOOST_TEST_BOOSTS_H
#define BITCOIN_TEST_UTIL_BOOST_TEST_BOOSTS_H

// This file contain macros and other constructs that "boost" the Boost Test
// Framework - additional capabilities, or existing capabilities made more
// usable with wrappers

////////////////////////////////////////////////////////////////////////////////
// Test that an `assert` was triggered
////////////////////////////////////////////////////////////////////////////////
//
// The following macro provides "death" tests: The test succeeds iff it causes
// a crash.  (The name comes from the popular Googletest unit test framework.)
//
// These tests use the Boost Test `execution_monitor` facility to trap signals,
// specifically: SIGABRT (which is raised by the `assert` statement - iff Linux!).
// The execution monitor will trap signals and reflect them as exceptions.  (So
// these aren't really "full" death tests à la Googletest as it is not trapping
// hard faults like calling through a null pointer.  But we don't actually need
// that so it's fine.)
//
// `assert` statement signals SIGABRT - the macro `BOOST_CHECK_SIGABRT` succeeds
// iff that SIGABRT is raised. (Could be a false positive if code signals SIGABRT
// for _some other reason than calling `assert`; also if some _other_
// `system_error` is caused ...)) (There doesn't appear to be any way to check
// the actual `assert` message to distinguish between different asserts, and the
// line number field of the exception is not set either.)
//
// Each successful call of the macro will cause a message to be logged, along the
// lines of "... Assertion ... failed."  This is _expected_ - the message is
// emitted by `assertion_fail()` (`src/util/check.{h,cpp}`). The test _succeeds_
// iff the assert is triggered (and thus prints that message, in failing).
//
// N.B.: These tests are _only_ run if the OS is _not_ Windows _and_ the Thread
// Sanitizer is _not_ being used.  So protect all such death tests by checking
// that `OK_TO_TEST_ASSERT_FUNCTION` is defined:
//
//     ```
//     #if defined(OK_TO_TEST_ASSERT_FUNCITON)
//     BOOST_AUTO_TEST_CASE(ensure_method_foo_triggers_assert)
//     { ... }
//     #endif
//     ```
//
//     N.B.: Apparently doesn't work with MSVC or MINGW.  Looking at Boost's
//     `execution_monitor.ipp` it seems like it _should_ work: the code there
//     seems to take the structured exception from the `assert` and change it to
//     a `boost::execution_exception`.  But, no, apparently it doesn't: the
//     Bitcoin repository CI pipeline for `win64 [unit tests, no gui tests,
//     no boost::process, no functional tests]` prints the assert and then
//     aborts.  Also `win64 [native]`.
//
//     N.B.: Doesn't work with the ThreadSanitizer, which doesn't like an unsafe
//     call inside of the Boost Test signal handler.

#define BOOST_CHECK_SIGABRT(expr)                                                  \
    {                                                                              \
        ::boost::execution_monitor exmon;                                          \
        BOOST_TEST_MESSAGE("!!! This is a 'death test': It is expected that "      \
                           "'Assertion ... failed' message will follow .. and if " \
                           "it does the test will _succeed_");                     \
        BOOST_CHECK_EXCEPTION(                                                     \
            exmon.vexecute([&]() { (void)(expr); }),                               \
            boost::execution_exception,                                            \
            [](boost::execution_exception const& ex) {                             \
                return ex.code() == boost::execution_exception::system_error;      \
            });                                                                    \
    }

// Tests that check whether an `assert` function is hit can only run when _not_
// under Windows _and not_ under the Thread Sanitizer.
#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define THREAD_SANITIZER_IN_PLAY 1
#endif
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#define OS_IS_WINDOWS 1
#endif

#if !defined(THREAD_SANITIZER_IN_PLAY) && !defined(OS_IS_WINDOWS)
#define OK_TO_TEST_ASSERT_FUNCTION 1
#endif

#undef THREAD_SANITIZER_IN_PLAY
#undef OS_IS_WINDOWS

#endif // BITCOIN_TEST_UTIL_BOOST_TEST_BOOSTS_H
