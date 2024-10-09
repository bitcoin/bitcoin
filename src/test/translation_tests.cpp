// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tinyformat.h>
#include <util/translation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(translation_tests)

BOOST_AUTO_TEST_CASE(translation_namedparams)
{
    bilingual_str arg{"original", "translated"};
    constexpr auto format{Untranslated("original [%s]")};
    bilingual_str result{strprintf(format, arg)};
    BOOST_CHECK_EQUAL(result.original, "original [original]");
    BOOST_CHECK_EQUAL(result.translated, "original [translated]");
}

BOOST_AUTO_TEST_SUITE_END()
