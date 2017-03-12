// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "evunix.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <event2/bufferevent.h>
#include <boost/filesystem/operations.hpp>

/** Helper function to initialize a sockaddr_un from a path */
static bool sockaddr_from_path(struct sockaddr_un *addr, const boost::filesystem::path &path)
{
    std::string name = path.string();
    if (name.size() >= sizeof(addr->sun_path)) {
        /* Name too long */
        return false;
    }
    memset(addr, 0, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, name.c_str(), sizeof(addr->sun_path)-1);
    return true;
}

bool evunix_remove_socket(const boost::filesystem::path &path)
{
    if (boost::filesystem::status(path).type() == boost::filesystem::socket_file) {
        boost::system::error_code ec;
        boost::filesystem::remove(path, ec);
        if (ec) { /* error while deleting */
            return false;
        }
    }
    return true;
}

int evunix_bind_fd(const boost::filesystem::path &path)
{
    struct sockaddr_un addr;
    if (!sockaddr_from_path(&addr, path)) {
        return -1;
    }
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    /* Remove any previous sockets left behind. listen() will refuse to overwrite
     * any file or socket. Remove only sockets, not other files that happen to have
     * the same name.
     */
    if (!evunix_remove_socket(path)) {
        close(fd);
        return -1;
    }
    /* Bind and listen */
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, 5) < 0) {
        close(fd);
        return -1;
    }
    evutil_make_socket_nonblocking(fd);
    return fd;
}

int evunix_connect_fd(const boost::filesystem::path &path)
{
    struct sockaddr_un addr;
    if (!sockaddr_from_path(&addr, path)) {
        return -1;
    }
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    evutil_make_socket_nonblocking(fd);
    return fd;
}

bool evunix_is_conn_from_unix_fd(int fd)
{
    struct sockaddr_un peer_unix;
    socklen_t peer_unix_len = sizeof(peer_unix);
    if (getpeername(fd, (sockaddr*)&peer_unix, &peer_unix_len) == 0) {
        if (peer_unix.sun_family == AF_UNIX) {
            return true;
        }
    }
    return false;
}

struct bufferevent *evunix_bind(struct event_base *base, const boost::filesystem::path &path)
{
    struct bufferevent *rv;
    int fd = evunix_bind_fd(path);
    if ((rv = bufferevent_socket_new(base, fd, 0)) == NULL) {
        close(fd);
        return NULL;
    }
    return rv;
}

struct bufferevent *evunix_connect(struct event_base *base, const boost::filesystem::path &path)
{
    struct bufferevent *rv;
    int fd = evunix_connect_fd(path);
    if ((rv = bufferevent_socket_new(base, fd, 0)) == NULL) {
        close(fd);
        return NULL;
    }
    return rv;
}

bool evunix_is_conn_from_unix(struct bufferevent *bev)
{
    int fd;
    if ((fd = bufferevent_getfd(bev)) != -1) {
        return evunix_is_conn_from_unix_fd(fd);
    }
    return false;
}
