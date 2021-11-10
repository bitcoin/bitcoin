//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_VERB_HPP
#define BOOST_BEAST_HTTP_VERB_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/string.hpp>
#include <iosfwd>

namespace boost {
namespace beast {
namespace http {

/** HTTP request method verbs

    Each verb corresponds to a particular method string
    used in HTTP request messages.
*/
enum class verb
{
    /** An unknown method.

        This value indicates that the request method string is not
        one of the recognized verbs. Callers interested in the method
        should use an interface which returns the original string.
    */
    unknown = 0,

    /// The DELETE method deletes the specified resource
    delete_,

    /** The GET method requests a representation of the specified resource.

        Requests using GET should only retrieve data and should have no other effect.
    */
    get,

    /** The HEAD method asks for a response identical to that of a GET request, but without the response body.
    
        This is useful for retrieving meta-information written in response
        headers, without having to transport the entire content.
    */
    head,

    /** The POST method requests that the server accept the entity enclosed in the request as a new subordinate of the web resource identified by the URI.

        The data POSTed might be, for example, an annotation for existing
        resources; a message for a bulletin board, newsgroup, mailing list,
        or comment thread; a block of data that is the result of submitting
        a web form to a data-handling process; or an item to add to a database
    */
    post,

    /** The PUT method requests that the enclosed entity be stored under the supplied URI.

        If the URI refers to an already existing resource, it is modified;
        if the URI does not point to an existing resource, then the server
        can create the resource with that URI.
    */
    put,

    /** The CONNECT method converts the request connection to a transparent TCP/IP tunnel.

        This is usually to facilitate SSL-encrypted communication (HTTPS)
        through an unencrypted HTTP proxy.
    */
    connect,

    /** The OPTIONS method returns the HTTP methods that the server supports for the specified URL.
    
        This can be used to check the functionality of a web server by requesting
        '*' instead of a specific resource.
    */
    options,

    /** The TRACE method echoes the received request so that a client can see what (if any) changes or additions have been made by intermediate servers.
    */
    trace,

    // WebDAV

    copy,
    lock,
    mkcol,
    move,
    propfind,
    proppatch,
    search,
    unlock,
    bind,
    rebind,
    unbind,
    acl,

    // subversion

    report,
    mkactivity,
    checkout,
    merge,

    // upnp

    msearch,
    notify,
    subscribe,
    unsubscribe,

    // RFC-5789

    patch,
    purge,

    // CalDAV

    mkcalendar,

    // RFC-2068, section 19.6.1.2

    link,
    unlink
};

/** Converts a string to the request method verb.

    If the string does not match a known request method,
    @ref verb::unknown is returned.
*/
BOOST_BEAST_DECL
verb
string_to_verb(string_view s);

/// Returns the text representation of a request method verb.
BOOST_BEAST_DECL
string_view
to_string(verb v);

/// Write the text for a request method verb to an output stream.
inline
std::ostream&
operator<<(std::ostream& os, verb v)
{
    return os << to_string(v);
}

} // http
} // beast
} // boost

#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/http/impl/verb.ipp>
#endif

#endif
