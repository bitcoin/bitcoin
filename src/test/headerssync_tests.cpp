// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <headerssync.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <iterator>

BOOST_FIXTURE_TEST_SUITE(headerssync_tests, BasicTestingSetup)

// Test vectors checking that ComputeHeadersSyncParamsInner() reproduces the reference optimizer,
// each a tuple containing:
// - inputs: max_headers, minchainwork_headers, attack_headers
// - expected outputs: period, bufsize
//
// These were created by starting from a random starting point:
// - timespan: log-normal distributed with mode 12 years and mean 18 years
// - minchainwork_headers: exponentially distributed with mean 1000000
// - attack_headers: log-normal distributed with median = LIMIT_FRACTION * minchainwork_headers and
//   sigma = 1.5
// And then walking in a random direction in 3D log-space, collecting 100 transition points where
// the optimal (period, bufsize) changes, and then picking the one whose max_headers and
// minchainwork_headers are closest to integers among them. The result is a set of test vectors that
// are close to values where the results change. Filtering is applied to remove cases that are too
// close to machine precision, as those are genuinely too ambiguous.

BOOST_AUTO_TEST_CASE(compute_headers_sync_params)
{
    const struct {
        int64_t max_headers;
        int64_t minchainwork_headers;
        double attack_headers;
        int64_t expected_period;
        int64_t expected_bufsize;
    } cases[]{
        {1719079112, 2486165, 2.4371139965488073e-05, 437, 10253},
        {1849920928, 1375438, 0.00010047802426348386, 473, 10199},
        {1811416960, 7221184, 0.0003834358670986813, 489, 9628},
        {7677261228, 752770, 7.657623409916339e-06, 869, 23031},
        {1971752357, 1225799, 1.7845208713901033e-05, 462, 11091},
        {3159905880, 2665516, 6.735316426018734e-06, 565, 14578},
        {2683034630, 193996, 1.719177483252805e-05, 536, 13059},
        {1697590446, 848276, 1.139595371908207e-05, 425, 10414},
        {2228709291, 1595505, 0.00046448453932950096, 544, 10681},
        {2088183178, 1397130, 0.0009937322357444738, 542, 10045},
        {2848303623, 2429248, 0.0009830357473239024, 628, 11820},
        {2755700321, 1725832, 3.079889435377424e-05, 552, 13016},
        {1573880545, 1264386, 6.55879555496996e-05, 432, 9502},
        {2448458334, 306585, 6.341304654686745e-06, 499, 12802},
        {4006740955, 536513, 9.862654648507075e-05, 684, 15275},
        {3222917775, 11128, 4.1587518500733085e-07, 532, 15806},
        {2595931233, 109205, 2.345792746509269e-06, 500, 13547},
        {1547460218, 342682, 5.786285545229493e-06, 399, 10123},
        {3412396570, 857000, 9.055283299520372e-06, 591, 15058},
        {1608565500, 1047750, 2.9746757899990667e-05, 426, 9850},
        {6751831959, 3420, 1.9796744871262838e-07, 748, 23538},
        {2112492775, 949804, 3.878638517429712e-05, 489, 11245},
        {5323388613, 226195, 1.259247706141301e-05, 738, 18782},
        {2516536860, 1823375, 0.0005282972243682878, 579, 11330},
        {4153605296, 3153542, 7.208345736641362e-05, 689, 15710},
        {1392991392, 2455336, 1.4467550657002429e-05, 389, 9333},
        {3045192620, 387654, 0.00028714231249025725, 621, 12789},
        {2470936704, 374758, 2.304932998416373e-06, 488, 13211},
        {5432386284, 1767951, 0.0004270020432999718, 829, 17080},
        {2575424700, 428736, 4.513652725339203e-05, 541, 12418},
        {2081820804, 1384033, 3.4110799029667034e-05, 484, 11217},
        {2140791604, 25777, 1.6633229346223927e-08, 406, 13727},
        {1676484364, 1560700, 9.011220006487238e-06, 420, 10410},
        {1542576120, 965810, 0.00011890792700529461, 436, 9229},
        {2038026484, 1779007, 6.22198825015376e-05, 487, 10873},
        {2600282368, 1746143, 2.558733122710413e-05, 534, 12693},
        {6469970160, 57485, 1.540887312697366e-07, 728, 23138},
        {3393786462, 656909, 1.6802005293828858e-05, 599, 14741},
        {1923909893, 8086, 1.5806088698106541e-07, 405, 12385},
        {4359755744, 1081995, 0.0011488135311153401, 773, 14703},
        {3329013904, 723851, 0.0016408648354929998, 689, 12598},
        {2785727394, 1049862, 3.914501343290894e-05, 559, 12996},
        {3794021233, 643680, 2.7740369347053645e-06, 602, 16387},
        {5346173856, 345137, 7.285354474681436e-06, 729, 19094},
        {3350561084, 1051927, 9.397334309598376e-05, 627, 13934},
        {9319364840, 1244089, 0.00011545830655088754, 1029, 23605},
        {4216249682, 971771, 0.00025973759887733955, 723, 15204},
        {1105318683, 222719, 0.00010566643251054803, 370, 7783},
        {6168635190, 1335470, 2.876811767724857e-05, 811, 19828},
        {1709740236, 57056, 3.177749891774726e-07, 389, 11475},
    };
    // Temporary: the random search the optimizer uses at this point is too slow to run every case
    // on each test run, so for now only check one randomly chosen case. A later commit replaces
    // the random search with a much faster algorithm, making it fast enough to check all of them.
    const auto& cas = cases[m_rng.randrange(std::size(cases))];
    const auto [period, bufsize] = ComputeHeadersSyncParamsInner(cas.max_headers, cas.minchainwork_headers, cas.attack_headers);
    BOOST_CHECK_EQUAL(period, cas.expected_period);
    BOOST_CHECK_EQUAL(bufsize, cas.expected_bufsize);
}

BOOST_AUTO_TEST_SUITE_END()
