// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>

#include <limits>

using namespace util;

BOOST_AUTO_TEST_SUITE(util_string_tests)

constexpr unsigned INVALID = std::numeric_limits<unsigned>::max();

// Helper to allow compile-time sanity checks
consteval void ValidFormatSpecifiers(std::string_view str, int num_params)
{
    // Execute compile-time check again at run-time to get code coverage stats
    CheckFormatSpecifiers(str, num_params);
}

BOOST_AUTO_TEST_CASE(ConstevalFormatString_CompileTimePass)
{
    ValidFormatSpecifiers("", 0);
    ValidFormatSpecifiers("%%", 0);
    ValidFormatSpecifiers("%s", 1);
    ValidFormatSpecifiers("%%s", 0);
    ValidFormatSpecifiers("s%%", 0);
    ValidFormatSpecifiers("%%%s", 1);
    ValidFormatSpecifiers("%s%%", 1);
    ValidFormatSpecifiers(" 1$s", 0);
    ValidFormatSpecifiers("%1$s", 1);
    ValidFormatSpecifiers("%1$s%1$s", 1);
    ValidFormatSpecifiers("%2$s", 2);
    ValidFormatSpecifiers("%2$s 4$s %2$s", 2);
    ValidFormatSpecifiers("%129$s 999$s %2$s", 129);
    ValidFormatSpecifiers("%02d", 1);
    ValidFormatSpecifiers("%+2s", 1);
    ValidFormatSpecifiers("%.6i", 1);
    ValidFormatSpecifiers("%5.2f", 1);
    ValidFormatSpecifiers("%#x", 1);
    ValidFormatSpecifiers("%1$5i", 1);
    ValidFormatSpecifiers("%1$-5i", 1);
    ValidFormatSpecifiers("%1$.5i", 1);
    // tinyformat accepts almost any "type" spec, even '%', or '_', or '\n'.
    ValidFormatSpecifiers("%123%", 1);
    ValidFormatSpecifiers("%123%s", 1);
    ValidFormatSpecifiers("%_", 1);
    ValidFormatSpecifiers("%\n", 1);

    // The `*` specifier behavior is unsupported and can lead to runtime
    // errors when used in a ConstevalFormatString. Please refer to the
    // note in the ConstevalFormatString docs.
    ValidFormatSpecifiers("%*c", 1);
    ValidFormatSpecifiers("%2$*3$d", 2);
    ValidFormatSpecifiers("%.*f", 1);
}

BOOST_AUTO_TEST_CASE(ConstevalFormatString_RuntimeFail)
{
    HasReason err_mix{"Format specifiers must be all positional or all non-positional!"};
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%s%1$s", 1), const char*, err_mix);

    HasReason err_num{"Format specifier count must match the argument count!"};
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("", 1), const char*, err_num);
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%s", 0), const char*, err_num);
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%s", 2), const char*, err_num);
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%1$s", 0), const char*, err_num);
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%1$s", 2), const char*, err_num);

    HasReason err_0_pos{"Positional format specifier must have position of at least 1"};
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%$s", INVALID), const char*, err_0_pos);
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%$", INVALID), const char*, err_0_pos);
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%0$", INVALID), const char*, err_0_pos);
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%0$s", INVALID), const char*, err_0_pos);

    HasReason err_term{"Format specifier incorrectly terminated by end of string"};
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%", INVALID), const char*, err_term);
    BOOST_CHECK_EXCEPTION(CheckFormatSpecifiers("%1$", INVALID), const char*, err_term);
}

BOOST_AUTO_TEST_SUITE_END()
