//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#include <boost/beast/_experimental/unit_test/amount.hpp>
#include <boost/beast/_experimental/unit_test/dstream.hpp>
#include <boost/beast/_experimental/unit_test/global_suites.hpp>
#include <boost/beast/_experimental/unit_test/match.hpp>
#include <boost/beast/_experimental/unit_test/reporter.hpp>
#include <boost/beast/_experimental/unit_test/suite.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <vector>

#ifdef BOOST_MSVC
# ifndef WIN32_LEAN_AND_MEAN // VC_EXTRALEAN
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
# else
#  include <windows.h>
# endif
#endif

// Simple main used to produce stand
// alone executables that run unit tests.
int main(int ac, char const* av[])
{
    using namespace std;
    using namespace boost::beast::unit_test;

    dstream log(std::cerr);
    std::unitbuf(log);

#ifdef BOOST_MSVC
    {
        int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        flags |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(flags);
    }
#endif

    if(ac == 2)
    {
        std::string const s{av[1]};
        if(s == "-h" || s == "--help")
        {
            log <<
                "Usage:\n"
                "  " << av[0] << ": { <suite-name>... }" <<
                std::endl;
            return EXIT_SUCCESS;
        }
    }

    reporter r(log);
    bool failed;
    if(ac > 1)
    {
        std::vector<selector> v;
        v.reserve(ac - 1);
        for(int i = 1; i < ac; ++i)
            v.emplace_back(selector::automatch, av[i]);
        auto pred =
            [&v](suite_info const& si) mutable
            {
                for(auto& p : v)
                    if(p(si))
                        return true;
                return false;
            };
        failed = r.run_each_if(global_suites(), pred);
    }
    else
    {
        failed = r.run_each(global_suites());
    }
    if(failed)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
