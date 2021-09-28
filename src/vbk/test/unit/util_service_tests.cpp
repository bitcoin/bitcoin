// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <vbk/util.hpp>
#include <veriblock/pop.hpp>


BOOST_AUTO_TEST_SUITE(util_service_tests)

BOOST_AUTO_TEST_CASE(id_conversions)
{
    auto A = altintegration::ATV{};
    auto B = altintegration::VbkBlock{};

    auto uint1 = VeriBlock::IdToUint256<altintegration::ATV>(A.getId());
    auto id1 = VeriBlock::Uint256ToId<altintegration::ATV>(uint1);

    BOOST_CHECK(A.getId() == id1);

    auto uint2 = VeriBlock::IdToUint256<altintegration::VbkBlock>(B.getId());
    auto id2 = VeriBlock::Uint256ToId<altintegration::VbkBlock>(uint2);

    BOOST_CHECK(B.getId() == id2);

}

BOOST_AUTO_TEST_SUITE_END()
