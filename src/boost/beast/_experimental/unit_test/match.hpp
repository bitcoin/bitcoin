//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_UNIT_TEST_MATCH_HPP
#define BOOST_BEAST_UNIT_TEST_MATCH_HPP

#include <boost/beast/_experimental/unit_test/suite_info.hpp>
#include <string>

namespace boost {
namespace beast {
namespace unit_test {

// Predicate for implementing matches
class selector
{
public:
    enum mode_t
    {
        // Run all tests except manual ones
        all,

        // Run tests that match in any field
        automatch,

        // Match on suite
        suite,

        // Match on library
        library,

        // Match on module (used internally)
        module,

        // Match nothing (used internally)
        none
    };

private:
    mode_t mode_;
    std::string pat_;
    std::string library_;

public:
    template<class = void>
    explicit
    selector(mode_t mode, std::string const& pattern = "");

    template<class = void>
    bool
    operator()(suite_info const& s);
};

//------------------------------------------------------------------------------

template<class>
selector::selector(mode_t mode, std::string const& pattern)
    : mode_(mode)
    , pat_(pattern)
{
    if(mode_ == automatch && pattern.empty())
        mode_ = all;
}

template<class>
bool
selector::operator()(suite_info const& s)
{
    switch(mode_)
    {
    case automatch:
        // suite or full name
        if(s.name() == pat_ || s.full_name() == pat_)
        {
            mode_ = none;
            return true;
        }

        // check module
        if(pat_ == s.module())
        {
            mode_ = module;
            library_ = s.library();
            return ! s.manual();
        }

        // check library
        if(pat_ == s.library())
        {
            mode_ = library;
            return ! s.manual();
        }

        return false;

    case suite:
        return pat_ == s.name();

    case module:
        return pat_ == s.module() && ! s.manual();

    case library:
        return pat_ == s.library() && ! s.manual();

    case none:
        return false;

    case all:
    default:
        // fall through
        break;
    };

    return ! s.manual();
}

//------------------------------------------------------------------------------

// Utility functions for producing predicates to select suites.

/** Returns a predicate that implements a smart matching rule.
    The predicate checks the suite, module, and library fields of the
    suite_info in that order. When it finds a match, it changes modes
    depending on what was found:

        If a suite is matched first, then only the suite is selected. The
        suite may be marked manual.

        If a module is matched first, then only suites from that module
        and library not marked manual are selected from then on.

        If a library is matched first, then only suites from that library
        not marked manual are selected from then on.

*/
inline
selector
match_auto(std::string const& name)
{
    return selector(selector::automatch, name);
}

/** Return a predicate that matches all suites not marked manual. */
inline
selector
match_all()
{
    return selector(selector::all);
}

/** Returns a predicate that matches a specific suite. */
inline
selector
match_suite(std::string const& name)
{
    return selector(selector::suite, name);
}

/** Returns a predicate that matches all suites in a library. */
inline
selector
match_library(std::string const& name)
{
    return selector(selector::library, name);
}

} // unit_test
} // beast
} // boost

#endif
