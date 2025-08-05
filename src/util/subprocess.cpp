// Copyright (c) 2025-present The Bitcoin Knots developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#if !((defined _MSC_VER) || (defined __MINGW32__))

#if HAVE_CLOSE_RANGE_LINUX
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
    #include <linux/close_range.h>
#endif

#include <limits.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

namespace subprocess {
namespace detail {

void subprocess_close_all_fds(const int except_fd)
{
#if HAVE_CLOSE_RANGE_GENERIC || HAVE_CLOSE_RANGE_LINUX
    if (except_fd < 3) {
        if (!close_range(3, UINT_MAX, 0)) return;
    } else if (except_fd == 3) {
        if (!close_range(4, UINT_MAX, 0)) return;
    } else {
        if (!(close_range(3, except_fd - 1, 0) || close_range(except_fd + 1, UINT_MAX, 0))) return;
    }
#endif

    unsigned int max_fd;

    struct rlimit limit_fds;
    if (getrlimit(RLIMIT_NOFILE, &limit_fds) == 0 && limit_fds.rlim_cur < INT_MAX) {
        max_fd = limit_fds.rlim_cur;
    } else {
        max_fd = INT_MAX;
    }

    for (int fd = max_fd; fd > 2; --fd) {
        if (fd == except_fd) continue;
        close(fd);
    }
}

}  // namespace detail
}  // namespace subprocess

#endif // !((defined _MSC_VER) || (defined __MINGW32__))
