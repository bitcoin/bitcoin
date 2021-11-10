/* boost uuid/detail/random_provider_posix implementation
*
* Copyright Jens Maurer 2000
* Copyright 2007 Andy Tompkins.
* Copyright Steven Watanabe 2010-2011
* Copyright 2017 James E. King III
*
* Distributed under the Boost Software License, Version 1.0. (See
* accompanying file LICENSE_1_0.txt or copy at
* https://www.boost.org/LICENSE_1_0.txt)
*
* $Id$
*/

#include <boost/config.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/move/core.hpp>
#include <boost/throw_exception.hpp>
#include <boost/uuid/entropy_error.hpp>
#include <cerrno>
#include <cstddef>
#include <fcntl.h>    // open
#include <sys/stat.h>
#include <sys/types.h>
#if defined(BOOST_HAS_UNISTD_H)
#include <unistd.h>
#endif

#ifndef BOOST_UUID_RANDOM_PROVIDER_POSIX_IMPL_CLOSE
#define BOOST_UUID_RANDOM_PROVIDER_POSIX_IMPL_CLOSE ::close
#endif
#ifndef BOOST_UUID_RANDOM_PROVIDER_POSIX_IMPL_OPEN
#define BOOST_UUID_RANDOM_PROVIDER_POSIX_IMPL_OPEN ::open
#endif
#ifndef BOOST_UUID_RANDOM_PROVIDER_POSIX_IMPL_READ
#define BOOST_UUID_RANDOM_PROVIDER_POSIX_IMPL_READ ::read
#endif

namespace boost {
namespace uuids {
namespace detail {

class random_provider_base
{
    BOOST_MOVABLE_BUT_NOT_COPYABLE(random_provider_base)

public:
    random_provider_base()
      : fd_(-1)
    {
        int flags = O_RDONLY;
#if defined(O_CLOEXEC)
        flags |= O_CLOEXEC;
#endif
        fd_ = BOOST_UUID_RANDOM_PROVIDER_POSIX_IMPL_OPEN("/dev/urandom", flags);

        if (BOOST_UNLIKELY(-1 == fd_))
        {
            int err = errno;
            BOOST_THROW_EXCEPTION(entropy_error(err, "open /dev/urandom"));
        }
    }

    random_provider_base(BOOST_RV_REF(random_provider_base) that) BOOST_NOEXCEPT : fd_(that.fd_)
    {
        that.fd_ = -1;
    }

    random_provider_base& operator= (BOOST_RV_REF(random_provider_base) that) BOOST_NOEXCEPT
    {
        destroy();
        fd_ = that.fd_;
        that.fd_ = -1;
        return *this;
    }

    ~random_provider_base() BOOST_NOEXCEPT
    {
        destroy();
    }

    //! Obtain entropy and place it into a memory location
    //! \param[in]  buf  the location to write entropy
    //! \param[in]  siz  the number of bytes to acquire
    void get_random_bytes(void *buf, std::size_t siz)
    {
        std::size_t offset = 0;
        while (offset < siz)
        {
            ssize_t sz = BOOST_UUID_RANDOM_PROVIDER_POSIX_IMPL_READ(
                fd_, static_cast<char *>(buf) + offset, siz - offset);

            if (BOOST_UNLIKELY(sz < 0))
            {
                int err = errno;
                if (err == EINTR)
                    continue;
                BOOST_THROW_EXCEPTION(entropy_error(err, "read"));
            }

            offset += sz;
        }
    }

private:
    void destroy() BOOST_NOEXCEPT
    {
        if (fd_ >= 0)
        {
            boost::ignore_unused(BOOST_UUID_RANDOM_PROVIDER_POSIX_IMPL_CLOSE(fd_));
        }
    }

private:
    int fd_;
};

} // detail
} // uuids
} // boost
