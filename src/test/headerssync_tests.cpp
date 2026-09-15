// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <headerssync.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>

BOOST_FIXTURE_TEST_SUITE(headerssync_tests, BasicTestingSetup)

// Test vectors checking that ComputeHeadersSyncParamsInner() finds the optimal (period, bufsize)
// configuration, each a tuple containing:
// - inputs: max_headers, minchainwork_headers, attack_headers
// - expected outputs: period, bufsize
//
// The inputs were created by starting from a random starting point:
// - timespan: log-normal distributed with mode 12 years and mean 18 years
// - minchainwork_headers: exponentially distributed with mean 1000000
// - attack_headers: log-normal distributed with median = LIMIT_FRACTION * minchainwork_headers and
//   sigma = 1.5
// And then walking in a random direction in 3D log-space, sampling 100 points along the way, and
// picking the one whose max_headers and minchainwork_headers are closest to integers among them.
// Filtering is applied to remove cases that are too close to machine precision, as those are
// genuinely too ambiguous.
//
// The expected outputs are the exact optima for these inputs, found by an exhaustive search over
// all periods (independent of the search strategy the implementation uses), with the minimality
// of each bufsize verified by direct summation of the terms of the attack rate.

BOOST_AUTO_TEST_CASE(compute_headers_sync_params)
{
    const struct {
        int64_t max_headers;
        int64_t minchainwork_headers;
        double attack_headers;
        int64_t expected_period;
        int64_t expected_bufsize;
    } cases[]{
        {1719079112, 2486165, 2.4371139965488073e-05, 426, 10498},
        {1849920928, 1375438, 0.00010047802426348386, 460, 10447},
        {1811416960, 7221184, 0.0003834358670986813, 475, 9891},
        {7677261228, 752770, 7.657623409916339e-06, 855, 23362},
        {1971752357, 1225799, 1.7845208713901033e-05, 451, 11356},
        {3159905880, 2665516, 6.735316426018734e-06, 553, 14861},
        {2683034630, 193996, 1.719177483252805e-05, 524, 13336},
        {1697590446, 848276, 1.139595371908207e-05, 415, 10666},
        {2228709291, 1595505, 0.00046448453932950096, 529, 10952},
        {2088183178, 1397130, 0.0009937322357444738, 527, 10330},
        {2848303623, 2429248, 0.0009830357473239024, 611, 12115},
        {2755700321, 1725832, 3.079889435377424e-05, 539, 13284},
        {1573880545, 1264386, 6.55879555496996e-05, 420, 9744},
        {2448458334, 306585, 6.341304654686745e-06, 488, 13069},
        {4006740955, 536513, 9.862654648507075e-05, 669, 15577},
        {3222917775, 11128, 4.1587518500733085e-07, 522, 16082},
        {2595931233, 109205, 2.345792746509269e-06, 489, 13802},
        {1547460218, 342682, 5.786285545229493e-06, 389, 10343},
        {3412396570, 857000, 9.055283299520372e-06, 579, 15354},
        {1608565500, 1047750, 2.9746757899990667e-05, 415, 10094},
        {6751831959, 3420, 1.9796744871262838e-07, 737, 23866},
        {2112492775, 949804, 3.878638517429712e-05, 477, 11515},
        {5323388613, 226195, 1.259247706141301e-05, 725, 19117},
        {2516536860, 1823375, 0.0005282972243682878, 564, 11625},
        {4153605296, 3153542, 7.208345736641362e-05, 675, 16026},
        {1392991392, 2455336, 1.4467550657002429e-05, 379, 9563},
        {3045192620, 387654, 0.00028714231249025725, 606, 13086},
        {2470936704, 374758, 2.304932998416373e-06, 477, 13458},
        {5432386284, 1767951, 0.0004270020432999718, 812, 17415},
        {2575424700, 428736, 4.513652725339203e-05, 528, 12707},
        {2081820804, 1384033, 3.4110799029667034e-05, 472, 11475},
        {2140791604, 25777, 1.6633229346223927e-08, 399, 13992},
        {1676484364, 1560700, 9.011220006487238e-06, 409, 10643},
        {1542576120, 965810, 0.00011890792700529461, 424, 9479},
        {2038026484, 1779007, 6.22198825015376e-05, 475, 11140},
        {2600282368, 1746143, 2.558733122710413e-05, 522, 12979},
        {6469970160, 57485, 1.540887312697366e-07, 718, 23479},
        {3393786462, 656909, 1.6802005293828858e-05, 587, 15052},
        {1923909893, 8086, 1.5806088698106541e-07, 397, 12628},
        {4359755744, 1081995, 0.0011488135311153401, 755, 15031},
        {3329013904, 723851, 0.0016408648354929998, 671, 12904},
        {2785727394, 1049862, 3.914501343290894e-05, 546, 13281},
        {3794021233, 643680, 2.7740369347053645e-06, 591, 16700},
        {5346173856, 345137, 7.285354474681436e-06, 716, 19432},
        {3350561084, 1051927, 9.397334309598376e-05, 613, 14238},
        {9319364840, 1244089, 0.00011545830655088754, 1013, 23957},
        {4216249682, 971771, 0.00025973759887733955, 707, 15526},
        {1105318683, 222719, 0.00010566643251054803, 359, 8001},
        {6168635190, 1335470, 2.876811767724857e-05, 797, 20175},
        {1709740236, 57056, 3.177749891774726e-07, 381, 11713},
    };
    for (const auto& cas : cases) {
        const auto [period, bufsize] = ComputeHeadersSyncParamsInner(cas.max_headers, cas.minchainwork_headers, cas.attack_headers);
        BOOST_CHECK_EQUAL(period, cas.expected_period);
        BOOST_CHECK_EQUAL(bufsize, cas.expected_bufsize);
    }
}

BOOST_AUTO_TEST_SUITE_END()
