// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <util/tokenpipe.h>

#include <bitcoin-build-config.h> // IWYU pragma: keep

#ifndef WIN32

#include <cerrno>
#include <fcntl.h>
#include <optional>
#include <unistd.h>

TokenPipeEnd TokenPipe::TakeReadEnd()
{
    TokenPipeEnd res(m_fds[0]);
    m_fds[0] = -1;
    return res;
}

TokenPipeEnd TokenPipe::TakeWriteEnd()
{
    TokenPipeEnd res(m_fds[1]);
    m_fds[1] = -1;
    return res;
}

TokenPipeEnd::TokenPipeEnd(int fd) : m_fd(fd)
{
}

TokenPipeEnd::~TokenPipeEnd()
{
    Close();
}

int TokenPipeEnd::TokenWrite(uint8_t token)
{
    while (true) {
        ssize_t result = write(m_fd, &token, 1);
        if (result < 0) {
            // Failure. It's possible that the write was interrupted by a signal,
            // in that case retry.
            if (errno != EINTR) {
                return TS_ERR;
            }
        } else if (result == 0) {
            return TS_EOS;
        } else { // ==1
            return 0;
        }
    }
}

int TokenPipeEnd::TokenRead()
{
    uint8_t token;
    while (true) {
        ssize_t result = read(m_fd, &token, 1);
        if (result < 0) {
            // Failure. Check if the read was interrupted by a signal,
            // in that case retry.
            if (errno != EINTR) {
                return TS_ERR;
            }
        } else if (result == 0) {
            return TS_EOS;
        } else { // ==1
            return token;
        }
    }
    return token;
}

void TokenPipeEnd::Close()
{
    if (m_fd != -1) close(m_fd);
    m_fd = -1;
}

std::optional<TokenPipe> TokenPipe::Make()
{
    int fds[2] = {-1, -1};
#if HAVE_O_CLOEXEC && HAVE_DECL_PIPE2
    if (pipe2(fds, O_CLOEXEC) != 0) {
        return std::nullopt;
    }
#else
    if (pipe(fds) != 0) {
        return std::nullopt;
    }
#endif
    return TokenPipe(fds);
}

TokenPipe::~TokenPipe()
{
    Close();
}

void TokenPipe::Close()
{
    if (m_fds[0] != -1) close(m_fds[0]);
    if (m_fds[1] != -1) close(m_fds[1]);
    m_fds[0] = m_fds[1] = -1;
}

#endif // WIN32
