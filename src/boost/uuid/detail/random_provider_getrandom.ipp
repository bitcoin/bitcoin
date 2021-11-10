//
// Copyright (c) 2018 Andrey Semashev
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//   https://www.boost.org/LICENSE_1_0.txt)
//
// getrandom() capable platforms
//

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/predef/library/c/gnu.h>
#include <cerrno>
#include <cstddef>

#if !defined(BOOST_UUID_RANDOM_PROVIDER_GETRANDOM_IMPL_GETRANDOM)
#if BOOST_LIB_C_GNU >= BOOST_VERSION_NUMBER(2, 25, 0)
#define BOOST_UUID_RANDOM_PROVIDER_GETRANDOM_HAS_LIBC_WRAPPER
#elif defined(__has_include)
#if __has_include(<sys/random.h>)
#define BOOST_UUID_RANDOM_PROVIDER_GETRANDOM_HAS_LIBC_WRAPPER
#endif
#endif
#endif // !defined(BOOST_UUID_RANDOM_PROVIDER_GETRANDOM_IMPL_GETRANDOM)

#if defined(BOOST_UUID_RANDOM_PROVIDER_GETRANDOM_HAS_LIBC_WRAPPER)
#include <sys/random.h>
#else
#include <stddef.h> // ssize_t
#endif

namespace boost {
namespace uuids {
namespace detail {

class random_provider_base
{
public:
    //! Obtain entropy and place it into a memory location
    //! \param[in]  buf  the location to write entropy
    //! \param[in]  siz  the number of bytes to acquire
    void get_random_bytes(void *buf, std::size_t siz)
    {
        std::size_t offset = 0;
        while (offset < siz)
        {
            ssize_t sz = get_random(static_cast< char* >(buf) + offset, siz - offset, 0u);

            if (BOOST_UNLIKELY(sz < 0))
            {
                int err = errno;
                if (err == EINTR)
                    continue;
                BOOST_THROW_EXCEPTION(entropy_error(err, "getrandom"));
            }

            offset += sz;
        }
    }

private:
    static ssize_t get_random(void *buf, std::size_t size, unsigned int flags)
    {
#if defined(BOOST_UUID_RANDOM_PROVIDER_GETRANDOM_IMPL_GETRANDOM)
        return BOOST_UUID_RANDOM_PROVIDER_GETRANDOM_IMPL_GETRANDOM(buf, size, flags);
#elif defined(BOOST_UUID_RANDOM_PROVIDER_GETRANDOM_HAS_LIBC_WRAPPER)
        return ::getrandom(buf, size, flags);
#else
        return ::syscall(SYS_getrandom, buf, size, flags);
#endif
    }
};

} // detail
} // uuids
} // boost
