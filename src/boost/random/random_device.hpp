/* boost random/random_device.hpp header file
 *
 * Copyright Jens Maurer 2000
 * Copyright Steven Watanabe 2010-2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id$
 *
 * Revision history
 *  2000-02-18  Portability fixes (thanks to Beman Dawes)
 */

//  See http://www.boost.org/libs/random for documentation.


#ifndef BOOST_RANDOM_RANDOM_DEVICE_HPP
#define BOOST_RANDOM_RANDOM_DEVICE_HPP

#include <string>
#include <boost/config.hpp>
#include <boost/noncopyable.hpp>
#include <boost/random/detail/auto_link.hpp>
#include <boost/system/config.hpp> // force autolink to find Boost.System

namespace boost {
namespace random {

/**
 * Class \random_device models a \nondeterministic_random_number_generator.
 * It uses one or more implementation-defined stochastic processes to
 * generate a sequence of uniformly distributed non-deterministic random
 * numbers. For those environments where a non-deterministic random number
 * generator is not available, class random_device must not be implemented. See
 *
 *  @blockquote
 *  "Randomness Recommendations for Security", D. Eastlake, S. Crocker,
 *  J. Schiller, Network Working Group, RFC 1750, December 1994
 *  @endblockquote
 *
 * for further discussions. 
 *
 * @xmlnote
 * Some operating systems abstract the computer hardware enough
 * to make it difficult to non-intrusively monitor stochastic processes.
 * However, several do provide a special device for exactly this purpose.
 * It seems to be impossible to emulate the functionality using Standard
 * C++ only, so users should be aware that this class may not be available
 * on all platforms.
 * @endxmlnote
 *
 * <b>Implementation Note for Linux</b>
 *
 * On the Linux operating system, token is interpreted as a filesystem
 * path. It is assumed that this path denotes an operating system
 * pseudo-device which generates a stream of non-deterministic random
 * numbers. The pseudo-device should never signal an error or end-of-file.
 * Otherwise, @c std::ios_base::failure is thrown. By default,
 * \random_device uses the /dev/urandom pseudo-device to retrieve
 * the random numbers. Another option would be to specify the /dev/random
 * pseudo-device, which blocks on reads if the entropy pool has no more
 * random bits available.
 *
 * <b>Implementation Note for Windows</b>
 *
 * On the Windows operating system, token is interpreted as the name
 * of a cryptographic service provider.  By default \random_device uses
 * MS_DEF_PROV.
 *
 * <b>Performance</b>
 *
 * The test program <a href="\boost/libs/random/performance/nondet_random_speed.cpp">
 * nondet_random_speed.cpp</a> measures the execution times of the
 * random_device.hpp implementation of the above algorithms in a tight
 * loop. The performance has been evaluated on an
 * Intel(R) Core(TM) i7 CPU Q 840 \@ 1.87GHz, 1867 Mhz with
 * Visual C++ 2010, Microsoft Windows 7 Professional and with gcc 4.4.5,
 * Ubuntu Linux 2.6.35-25-generic.
 *
 * <table cols="2">
 *   <tr><th>Platform</th><th>time per invocation [microseconds]</th></tr>
 *   <tr><td> Windows </td><td>2.9</td></tr>
 *   <tr><td> Linux </td><td>1.7</td></tr>
 * </table>
 *
 * The measurement error is estimated at +/- 1 usec.
 */
class random_device : private noncopyable
{
public:
    typedef unsigned int result_type;
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

    /** Returns the smallest value that the \random_device can produce. */
    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
    /** Returns the largest value that the \random_device can produce. */
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () { return ~0u; }

    /** Constructs a @c random_device, optionally using the default device. */
    BOOST_RANDOM_DECL random_device();
    /** 
     * Constructs a @c random_device, optionally using the given token as an
     * access specification (for example, a URL) to some implementation-defined
     * service for monitoring a stochastic process. 
     */
    BOOST_RANDOM_DECL explicit random_device(const std::string& token);

    BOOST_RANDOM_DECL ~random_device();

    /**
     * Returns: An entropy estimate for the random numbers returned by
     * operator(), in the range min() to log2( max()+1). A deterministic
     * random number generator (e.g. a pseudo-random number engine)
     * has entropy 0.
     *
     * Throws: Nothing.
     */
    BOOST_RANDOM_DECL double entropy() const;
    /** Returns a random value in the range [min, max]. */
    BOOST_RANDOM_DECL unsigned int operator()();

    /** Fills a range with random 32-bit values. */
    template<class Iter>
    void generate(Iter begin, Iter end)
    {
        for(; begin != end; ++begin) {
            *begin = (*this)();
        }
    }

private:
    class impl;
    impl * pimpl;
};

} // namespace random

using random::random_device;

} // namespace boost

#endif /* BOOST_RANDOM_RANDOM_DEVICE_HPP */
