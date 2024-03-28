// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/feefrac.h>
#include <random.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(feefrac_tests)

BOOST_AUTO_TEST_CASE(feefrac_operators)
{
    FeeFrac p1{1000, 100}, p2{500, 300};
    FeeFrac sum{1500, 400};
    FeeFrac diff{500, -200};
    FeeFrac empty{0, 0};
    FeeFrac zero_fee{0, 1}; // zero-fee allowed

    BOOST_CHECK(empty == FeeFrac{}); // same as no-args

    BOOST_CHECK(p1 == p1);
    BOOST_CHECK(p1 + p2 == sum);
    BOOST_CHECK(p1 - p2 == diff);

    FeeFrac p3{2000, 200};
    BOOST_CHECK(p1 != p3); // feefracs only equal if both fee and size are same
    BOOST_CHECK(p2 != p3);

    FeeFrac p4{3000, 300};
    BOOST_CHECK(p1 == p4-p3);
    BOOST_CHECK(p1 + p3 == p4);

    // Fee-rate comparison
    BOOST_CHECK(p1 > p2);
    BOOST_CHECK(p1 >= p2);
    BOOST_CHECK(p1 >= p4-p3);
    BOOST_CHECK(!(p1 >> p3)); // not strictly better
    BOOST_CHECK(p1 >> p2); // strictly greater feerate

    BOOST_CHECK(p2 < p1);
    BOOST_CHECK(p2 <= p1);
    BOOST_CHECK(p1 <= p4-p3);
    BOOST_CHECK(!(p3 << p1)); // not strictly worse
    BOOST_CHECK(p2 << p1); // strictly lower feerate

    // "empty" comparisons
    BOOST_CHECK(!(p1 >> empty)); // << will always result in false
    BOOST_CHECK(!(p1 << empty));
    BOOST_CHECK(!(empty >> empty));
    BOOST_CHECK(!(empty << empty));

    // empty is always bigger than everything else
    BOOST_CHECK(empty > p1);
    BOOST_CHECK(empty > p2);
    BOOST_CHECK(empty > p3);
    BOOST_CHECK(empty >= p1);
    BOOST_CHECK(empty >= p2);
    BOOST_CHECK(empty >= p3);

    // check "max" values for comparison
    FeeFrac oversized_1{4611686000000, 4000000};
    FeeFrac oversized_2{184467440000000, 100000};

    BOOST_CHECK(oversized_1 < oversized_2);
    BOOST_CHECK(oversized_1 <= oversized_2);
    BOOST_CHECK(oversized_1 << oversized_2);
    BOOST_CHECK(oversized_1 != oversized_2);

    // Tests paths that use double arithmetic
    FeeFrac busted{(static_cast<int64_t>(INT32_MAX)) + 1, INT32_MAX};
    BOOST_CHECK(!(busted < busted));

    FeeFrac max_fee{2100000000000000, INT32_MAX};
    BOOST_CHECK(!(max_fee < max_fee));
    BOOST_CHECK(!(max_fee > max_fee));
    BOOST_CHECK(max_fee <= max_fee);
    BOOST_CHECK(max_fee >= max_fee);

    FeeFrac max_fee2{1, 1};
    BOOST_CHECK(max_fee >= max_fee2);

    // Examples where the cross-product only differs in the bottom 15 bits.
    BOOST_CHECK(FeeFrac(-354472897251355648, 296578183) >> FeeFrac(-499408498440516581, 417842002));
    BOOST_CHECK(FeeFrac(-370259345901664256, 575268238) >> FeeFrac(-403083894849742659, 626267411));
    BOOST_CHECK(FeeFrac(-4790828124043530874, 260938616) >> FeeFrac(-6785608372001660928, 369586888));
    BOOST_CHECK(FeeFrac(2052931238304952064, 1470459185) >> FeeFrac(1216033443768841520, 871011904));
    BOOST_CHECK(FeeFrac(3328196932075804, 13968015) >> FeeFrac(26663087684063549, 111901554));
    BOOST_CHECK(FeeFrac(649845842452623488, 705513551) >> FeeFrac(12357483567011782, 13416062));
    BOOST_CHECK(FeeFrac(-33570210247385060, 1158347843) << FeeFrac(-29357491397710948, 1012987008));
    BOOST_CHECK(FeeFrac(13516860620760744, 75363309) << FeeFrac(65117457547567521, 363062638));
    BOOST_CHECK(FeeFrac(127024547014822368, 10966520) >> FeeFrac(4591008642845568660, 396359517));
    BOOST_CHECK(FeeFrac(641206262541017349, 1601111971) >> FeeFrac(857646369645534208, 2141569647));
    BOOST_CHECK(FeeFrac(319248918395533404, 890947597) << FeeFrac(24513477590719404, 68411270));
    BOOST_CHECK(FeeFrac(-416336398673581514, 645275698) << FeeFrac(-19608809251366772, 30391501));
    BOOST_CHECK(FeeFrac(24877589752415396, 31056925) >> FeeFrac(1545275773492956040, 1929106247));
    BOOST_CHECK(FeeFrac(5450186678047674969, 799212602) >> FeeFrac(3144100200355222528, 461049254));
    BOOST_CHECK(FeeFrac(-6016636089170922, 70085349) << FeeFrac(-67775518923202977, 789489480));
    BOOST_CHECK(FeeFrac(1195652256495478170, 1894547911) << FeeFrac(115565133119112560, 183116521));
    BOOST_CHECK(FeeFrac(538333091032708, 4942329) << FeeFrac(226341450249867987, 2077995821));
    BOOST_CHECK(FeeFrac(2605011908834424255, 185052093) >> FeeFrac(175076062790674176, 12436869));
    BOOST_CHECK(FeeFrac(-4243705771917119569, 1848575776) >> FeeFrac(-447774047484587648, 195052226));
    BOOST_CHECK(FeeFrac(35701633923808156, 6812845) << FeeFrac(5427797217432359344, 1035771674));
    BOOST_CHECK(FeeFrac(-140501801028154240, 53635742) >> FeeFrac(-335303893132483452, 128000303));
    BOOST_CHECK(FeeFrac(131884519785687286, 34291038) << FeeFrac(1925475179304749056, 500639064));
    BOOST_CHECK(FeeFrac(-4295973306120651264, 344500120) << FeeFrac(-6678848995410997318, 535586261));
    BOOST_CHECK(FeeFrac(-200335923062347948, 79853205) >> FeeFrac(-52131062654134512, 20779261));
    BOOST_CHECK(FeeFrac(4359277869142369574, 1564739848) << FeeFrac(94311916336558496, 33852766));
    BOOST_CHECK(FeeFrac(6494990158669444, 143815522) << FeeFrac(10296636879881354, 227993603));
    BOOST_CHECK(FeeFrac(-290844406386311935, 337077230) >> FeeFrac(-272464809536509, 315776));
    BOOST_CHECK(FeeFrac(-376120311989284236, 1237996701) >> FeeFrac(-252395657734714400, 830758089));
    BOOST_CHECK(FeeFrac(7087461507220493307, 121324193) >> FeeFrac(511024007272958400, 8747783));
    BOOST_CHECK(FeeFrac(793359042451610, 320303) << FeeFrac(36760933975599752, 14841499));
    BOOST_CHECK(FeeFrac(79385399933498354, 817170152) << FeeFrac(100449099836562208, 1033993735));
    BOOST_CHECK(FeeFrac(-807972341776901574, 107767562) >> FeeFrac(-5590827257436576, 745706));
}

BOOST_AUTO_TEST_CASE(build_diagram_test)
{
    FeeFrac p1{1000, 100};
    FeeFrac empty{0, 0};
    FeeFrac zero_fee{0, 1};
    FeeFrac oversized_1{4611686000000, 4000000};
    FeeFrac oversized_2{184467440000000, 100000};

    // Diagram-building will reorder chunks
    std::vector<FeeFrac> chunks;
    chunks.push_back(p1);
    chunks.push_back(zero_fee);
    chunks.push_back(empty);
    chunks.push_back(oversized_1);
    chunks.push_back(oversized_2);

    // Caller in charge of sorting chunks in case chunk size limit
    // differs from cluster size limit
    std::sort(chunks.begin(), chunks.end(), [](const FeeFrac& a, const FeeFrac& b) { return a > b; });

    // Chunks are now sorted in reverse order (largest first)
    BOOST_CHECK(chunks[0] == empty); // empty is considered "highest" fee
    BOOST_CHECK(chunks[1] == oversized_2);
    BOOST_CHECK(chunks[2] == oversized_1);
    BOOST_CHECK(chunks[3] == p1);
    BOOST_CHECK(chunks[4] == zero_fee);

    std::vector<FeeFrac> generated_diagram{BuildDiagramFromChunks(chunks)};
    BOOST_CHECK(generated_diagram.size() == 1 + chunks.size());

    // Prepended with an empty, then the chunks summed in order as above
    BOOST_CHECK(generated_diagram[0] == empty);
    BOOST_CHECK(generated_diagram[1] == empty);
    BOOST_CHECK(generated_diagram[2] == oversized_2);
    BOOST_CHECK(generated_diagram[3] == oversized_2 + oversized_1);
    BOOST_CHECK(generated_diagram[4] == oversized_2 + oversized_1 + p1);
    BOOST_CHECK(generated_diagram[5] == oversized_2 + oversized_1 + p1 + zero_fee);
}

BOOST_AUTO_TEST_SUITE_END()
