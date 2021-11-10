/*
 *             Copyright Andrey Semashev 2020.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#if !defined(BOOST_ATOMIC_ENABLE_WARNINGS)

#if defined(BOOST_MSVC)

#pragma warning(pop)

#elif defined(BOOST_GCC) && BOOST_GCC >= 40600

#pragma GCC diagnostic pop

#elif defined(BOOST_CLANG)

#pragma clang diagnostic pop

#endif

#endif // !defined(BOOST_ATOMIC_ENABLE_WARNINGS)
