#ifndef H_SUBSIDYLEVELS
#define H_SUBSIDYLEVELS 1


#include <vector>

/**
 * This file contains pre-calculated subsidies for all subsidies given until
 * subsidies goes to zero.
 *
 * Bit-shifting subsidies for bitcoin, yes, clean and so on.
 *
 * This pre-calculated table describes an S-curve that creates a rising
 * subsidy for the first 140 000 blocks, which is approximately three years.
 *
 * After that the subsidy shrinks so that around 90 percent of all coins
 * have been created at block 540 000, which is approximately ten years
 * at a block rate of one block per ten minutes.
 *
 * The formula to calculate the subsidies starts with a subsidy of 40 * COIN
 * for block zero.
 *
 * Subsidy is then updated every 20 000 block with a factor 1.15.
 *
 * Once block 160 000 i reached, the subsidy starts decreasing.
 * The subsidy is decreased with a factor of 1.09 until it goes to zero.
 * This will happen at block 4 300 000, which is approximately 80 years
 * from the genesis block
 *
 *All values have been more or less adjusted to fit into an even range.
 *
 * Everything listed above is an ideal situation where no external coins are
 * claimed. All external claims will push the subsidy further along the S-curve.
 */

class SubsidyLevel {
public:
	int nEstimatedBlockHeight;
	uint64_t nTotalMonetaryBase;
	uint64_t nSubsidyUpdateTo;
	SubsidyLevel (int nEstimatedBlockHeight, uint64_t nTotalMonetaryBase, uint64_t nSubsidyUpdateTo) {
		this->nEstimatedBlockHeight = nEstimatedBlockHeight;
		this->nTotalMonetaryBase = nTotalMonetaryBase;
		this->nSubsidyUpdateTo = nSubsidyUpdateTo;
	}
};

inline std::vector<SubsidyLevel> createMainSubsidyLevels(){
    std::vector<SubsidyLevel> levels;
    levels.push_back(SubsidyLevel(0, 0, 4000000000));
    levels.push_back(SubsidyLevel(20000, 80000000000000, 4500000000));
    levels.push_back(SubsidyLevel(40000, 170000000000000, 5000000000));
    levels.push_back(SubsidyLevel(60000, 270000000000000, 5600000000));
    levels.push_back(SubsidyLevel(80000, 382000000000000, 6200000000));
    levels.push_back(SubsidyLevel(100000, 506000000000000, 7200000000));
    levels.push_back(SubsidyLevel(120000, 650000000000000, 8300000000));
    levels.push_back(SubsidyLevel(140000, 816000000000000, 10000000000));
    levels.push_back(SubsidyLevel(160000, 1016000000000000, 9000000000));
    levels.push_back(SubsidyLevel(180000, 1196000000000000, 8200000000));
    levels.push_back(SubsidyLevel(200000, 1360000000000000, 7500000000));
    levels.push_back(SubsidyLevel(220000, 1510000000000000, 6900000000));
    levels.push_back(SubsidyLevel(240000, 1648000000000000, 6400000000));
    levels.push_back(SubsidyLevel(260000, 1776000000000000, 5800000000));
    levels.push_back(SubsidyLevel(280000, 1892000000000000, 5300000000));
    levels.push_back(SubsidyLevel(300000, 1998000000000000, 4800000000));
    levels.push_back(SubsidyLevel(320000, 2094000000000000, 4400000000));
    levels.push_back(SubsidyLevel(340000, 2182000000000000, 4000000000));
    levels.push_back(SubsidyLevel(360000, 2262000000000000, 3600000000));
    levels.push_back(SubsidyLevel(380000, 2334000000000000, 3300000000));
    levels.push_back(SubsidyLevel(400000, 2400000000000000, 3000000000));
    levels.push_back(SubsidyLevel(420000, 2460000000000000, 2700000000));
    levels.push_back(SubsidyLevel(440000, 2514000000000000, 2400000000));
    levels.push_back(SubsidyLevel(460000, 2562000000000000, 2200000000));
    levels.push_back(SubsidyLevel(480000, 2606000000000000, 2000000000));
    levels.push_back(SubsidyLevel(500000, 2646000000000000, 1800000000));
    levels.push_back(SubsidyLevel(520000, 2682000000000000, 1600000000));
    levels.push_back(SubsidyLevel(540000, 2714000000000000, 1400000000));
    levels.push_back(SubsidyLevel(560000, 2742000000000000, 1200000000));
    levels.push_back(SubsidyLevel(580000, 2766000000000000, 1100000000));
    levels.push_back(SubsidyLevel(600000, 2788000000000000, 950000000));
    levels.push_back(SubsidyLevel(620000, 2807000000000000, 900000000));
    levels.push_back(SubsidyLevel(640000, 2825000000000000, 820000000));
    levels.push_back(SubsidyLevel(660000, 2841400000000000, 760000000));
    levels.push_back(SubsidyLevel(680000, 2856600000000000, 690000000));
    levels.push_back(SubsidyLevel(700000, 2870400000000000, 630000000));
    levels.push_back(SubsidyLevel(720000, 2883000000000000, 570000000));
    levels.push_back(SubsidyLevel(740000, 2894400000000000, 520000000));
    levels.push_back(SubsidyLevel(760000, 2904800000000000, 470000000));
    levels.push_back(SubsidyLevel(780000, 2914200000000000, 430000000));
    levels.push_back(SubsidyLevel(800000, 2922800000000000, 390000000));
    levels.push_back(SubsidyLevel(820000, 2930600000000000, 350000000));
    levels.push_back(SubsidyLevel(840000, 2937600000000000, 320000000));
    levels.push_back(SubsidyLevel(860000, 2944000000000000, 290000000));
    levels.push_back(SubsidyLevel(880000, 2949800000000000, 260000000));
    levels.push_back(SubsidyLevel(900000, 2955000000000000, 230000000));
    levels.push_back(SubsidyLevel(920000, 2959600000000000, 210000000));
    levels.push_back(SubsidyLevel(940000, 2963800000000000, 190000000));
    levels.push_back(SubsidyLevel(960000, 2967600000000000, 170000000));
    levels.push_back(SubsidyLevel(980000, 2971000000000000, 150000000));
    levels.push_back(SubsidyLevel(1000000, 2974000000000000, 130000000));
    levels.push_back(SubsidyLevel(1020000, 2976600000000000, 110000000));
    levels.push_back(SubsidyLevel(1040000, 2978800000000000, 100000000));
    levels.push_back(SubsidyLevel(1060000, 2980800000000000, 90000000));
    levels.push_back(SubsidyLevel(1080000, 2982600000000000, 82000000));
    levels.push_back(SubsidyLevel(1100000, 2984240000000000, 75000000));
    levels.push_back(SubsidyLevel(1120000, 2985740000000000, 68000000));
    levels.push_back(SubsidyLevel(1140000, 2987100000000000, 62000000));
    levels.push_back(SubsidyLevel(1160000, 2988340000000000, 56000000));
    levels.push_back(SubsidyLevel(1180000, 2989460000000000, 51000000));
    levels.push_back(SubsidyLevel(1200000, 2990480000000000, 47000000));
    levels.push_back(SubsidyLevel(1220000, 2991420000000000, 43000000));
    levels.push_back(SubsidyLevel(1240000, 2992280000000000, 39000000));
    levels.push_back(SubsidyLevel(1260000, 2993060000000000, 35000000));
    levels.push_back(SubsidyLevel(1280000, 2993760000000000, 32000000));
    levels.push_back(SubsidyLevel(1300000, 2994400000000000, 29000000));
    levels.push_back(SubsidyLevel(1320000, 2994980000000000, 26000000));
    levels.push_back(SubsidyLevel(1340000, 2995500000000000, 23000000));
    levels.push_back(SubsidyLevel(1360000, 2995960000000000, 21000000));
    levels.push_back(SubsidyLevel(1380000, 2996380000000000, 19000000));
    levels.push_back(SubsidyLevel(1400000, 2996760000000000, 17000000));
    levels.push_back(SubsidyLevel(1420000, 2997100000000000, 15000000));
    levels.push_back(SubsidyLevel(1440000, 2997400000000000, 13000000));
    levels.push_back(SubsidyLevel(1460000, 2997660000000000, 11000000));
    levels.push_back(SubsidyLevel(1480000, 2997880000000000, 10000000));
    levels.push_back(SubsidyLevel(1500000, 2998080000000000, 9100000));
    levels.push_back(SubsidyLevel(1520000, 2998262000000000, 8200000));
    levels.push_back(SubsidyLevel(1540000, 2998426000000000, 7500000));
    levels.push_back(SubsidyLevel(1560000, 2998576000000000, 6800000));
    levels.push_back(SubsidyLevel(1580000, 2998712000000000, 6200000));
    levels.push_back(SubsidyLevel(1600000, 2998836000000000, 5600000));
    levels.push_back(SubsidyLevel(1620000, 2998948000000000, 5100000));
    levels.push_back(SubsidyLevel(1640000, 2999050000000000, 4600000));
    levels.push_back(SubsidyLevel(1660000, 2999142000000000, 4300000));
    levels.push_back(SubsidyLevel(1680000, 2999228000000000, 3900000));
    levels.push_back(SubsidyLevel(1700000, 2999306000000000, 3500000));
    levels.push_back(SubsidyLevel(1720000, 2999376000000000, 3200000));
    levels.push_back(SubsidyLevel(1740000, 2999440000000000, 2900000));
    levels.push_back(SubsidyLevel(1760000, 2999498000000000, 2600000));
    levels.push_back(SubsidyLevel(1780000, 2999550000000000, 2300000));
    levels.push_back(SubsidyLevel(1800000, 2999596000000000, 2100000));
    levels.push_back(SubsidyLevel(1820000, 2999638000000000, 1900000));
    levels.push_back(SubsidyLevel(1840000, 2999676000000000, 1700000));
    levels.push_back(SubsidyLevel(1860000, 2999710000000000, 1500000));
    levels.push_back(SubsidyLevel(1880000, 2999740000000000, 1300000));
    levels.push_back(SubsidyLevel(1900000, 2999766000000000, 1100000));
    levels.push_back(SubsidyLevel(1920000, 2999788000000000, 1000000));
    levels.push_back(SubsidyLevel(1940000, 2999808000000000, 900000));
    levels.push_back(SubsidyLevel(1960000, 2999826000000000, 820000));
    levels.push_back(SubsidyLevel(1980000, 2999842400000000, 750000));
    levels.push_back(SubsidyLevel(2000000, 2999857400000000, 680000));
    levels.push_back(SubsidyLevel(2020000, 2999871000000000, 620000));
    levels.push_back(SubsidyLevel(2040000, 2999883400000000, 560000));
    levels.push_back(SubsidyLevel(2060000, 2999894600000000, 510000));
    levels.push_back(SubsidyLevel(2080000, 2999904800000000, 470000));
    levels.push_back(SubsidyLevel(2100000, 2999914200000000, 430000));
    levels.push_back(SubsidyLevel(2120000, 2999922800000000, 390000));
    levels.push_back(SubsidyLevel(2140000, 2999930600000000, 350000));
    levels.push_back(SubsidyLevel(2160000, 2999937600000000, 320000));
    levels.push_back(SubsidyLevel(2180000, 2999944000000000, 290000));
    levels.push_back(SubsidyLevel(2200000, 2999949800000000, 260000));
    levels.push_back(SubsidyLevel(2220000, 2999955000000000, 230000));
    levels.push_back(SubsidyLevel(2240000, 2999959600000000, 210000));
    levels.push_back(SubsidyLevel(2260000, 2999963800000000, 190000));
    levels.push_back(SubsidyLevel(2280000, 2999967600000000, 170000));
    levels.push_back(SubsidyLevel(2300000, 2999971000000000, 150000));
    levels.push_back(SubsidyLevel(2320000, 2999974000000000, 130000));
    levels.push_back(SubsidyLevel(2340000, 2999976600000000, 110000));
    levels.push_back(SubsidyLevel(2360000, 2999978800000000, 100000));
    levels.push_back(SubsidyLevel(2380000, 2999980800000000, 90000));
    levels.push_back(SubsidyLevel(2400000, 2999982600000000, 82000));
    levels.push_back(SubsidyLevel(2420000, 2999984240000000, 75000));
    levels.push_back(SubsidyLevel(2440000, 2999985740000000, 68000));
    levels.push_back(SubsidyLevel(2460000, 2999987100000000, 62000));
    levels.push_back(SubsidyLevel(2480000, 2999988340000000, 56000));
    levels.push_back(SubsidyLevel(2500000, 2999989460000000, 51000));
    levels.push_back(SubsidyLevel(2520000, 2999990480000000, 47000));
    levels.push_back(SubsidyLevel(2540000, 2999991420000000, 43000));
    levels.push_back(SubsidyLevel(2560000, 2999992280000000, 39000));
    levels.push_back(SubsidyLevel(2580000, 2999993060000000, 35000));
    levels.push_back(SubsidyLevel(2600000, 2999993760000000, 32000));
    levels.push_back(SubsidyLevel(2620000, 2999994400000000, 29000));
    levels.push_back(SubsidyLevel(2640000, 2999994980000000, 26000));
    levels.push_back(SubsidyLevel(2660000, 2999995500000000, 23000));
    levels.push_back(SubsidyLevel(2680000, 2999995960000000, 21000));
    levels.push_back(SubsidyLevel(2700000, 2999996380000000, 19000));
    levels.push_back(SubsidyLevel(2720000, 2999996760000000, 17000));
    levels.push_back(SubsidyLevel(2740000, 2999997100000000, 15000));
    levels.push_back(SubsidyLevel(2760000, 2999997400000000, 13000));
    levels.push_back(SubsidyLevel(2780000, 2999997660000000, 11000));
    levels.push_back(SubsidyLevel(2800000, 2999997880000000, 10000));
    levels.push_back(SubsidyLevel(2820000, 2999998080000000, 9000));
    levels.push_back(SubsidyLevel(2840000, 2999998260000000, 8200));
    levels.push_back(SubsidyLevel(2860000, 2999998424000000, 7500));
    levels.push_back(SubsidyLevel(2880000, 2999998574000000, 6800));
    levels.push_back(SubsidyLevel(2900000, 2999998710000000, 6200));
    levels.push_back(SubsidyLevel(2920000, 2999998834000000, 5600));
    levels.push_back(SubsidyLevel(2940000, 2999998946000000, 5100));
    levels.push_back(SubsidyLevel(2960000, 2999999048000000, 4700));
    levels.push_back(SubsidyLevel(2980000, 2999999142000000, 4300));
    levels.push_back(SubsidyLevel(3000000, 2999999228000000, 3900));
    levels.push_back(SubsidyLevel(3020000, 2999999306000000, 3500));
    levels.push_back(SubsidyLevel(3040000, 2999999376000000, 3200));
    levels.push_back(SubsidyLevel(3060000, 2999999440000000, 2900));
    levels.push_back(SubsidyLevel(3080000, 2999999498000000, 2600));
    levels.push_back(SubsidyLevel(3100000, 2999999550000000, 2300));
    levels.push_back(SubsidyLevel(3120000, 2999999596000000, 2100));
    levels.push_back(SubsidyLevel(3140000, 2999999638000000, 1900));
    levels.push_back(SubsidyLevel(3160000, 2999999676000000, 1700));
    levels.push_back(SubsidyLevel(3180000, 2999999710000000, 1500));
    levels.push_back(SubsidyLevel(3200000, 2999999740000000, 1300));
    levels.push_back(SubsidyLevel(3220000, 2999999766000000, 1100));
    levels.push_back(SubsidyLevel(3240000, 2999999788000000, 1000));
    levels.push_back(SubsidyLevel(3260000, 2999999808000000, 900));
    levels.push_back(SubsidyLevel(3280000, 2999999826000000, 820));
    levels.push_back(SubsidyLevel(3300000, 2999999842400000, 760));
    levels.push_back(SubsidyLevel(3320000, 2999999857600000, 690));
    levels.push_back(SubsidyLevel(3340000, 2999999871400000, 630));
    levels.push_back(SubsidyLevel(3360000, 2999999884000000, 570));
    levels.push_back(SubsidyLevel(3380000, 2999999895400000, 520));
    levels.push_back(SubsidyLevel(3400000, 2999999905800000, 470));
    levels.push_back(SubsidyLevel(3420000, 2999999915200000, 430));
    levels.push_back(SubsidyLevel(3440000, 2999999923800000, 390));
    levels.push_back(SubsidyLevel(3460000, 2999999931600000, 350));
    levels.push_back(SubsidyLevel(3480000, 2999999938600000, 320));
    levels.push_back(SubsidyLevel(3500000, 2999999945000000, 290));
    levels.push_back(SubsidyLevel(3520000, 2999999950800000, 260));
    levels.push_back(SubsidyLevel(3540000, 2999999956000000, 230));
    levels.push_back(SubsidyLevel(3560000, 2999999960600000, 210));
    levels.push_back(SubsidyLevel(3580000, 2999999964800000, 190));
    levels.push_back(SubsidyLevel(3600000, 2999999968600000, 170));
    levels.push_back(SubsidyLevel(3620000, 2999999972000000, 150));
    levels.push_back(SubsidyLevel(3640000, 2999999975000000, 130));
    levels.push_back(SubsidyLevel(3660000, 2999999977600000, 110));
    levels.push_back(SubsidyLevel(3680000, 2999999979800000, 100));
    levels.push_back(SubsidyLevel(3700000, 2999999981800000, 90));
    levels.push_back(SubsidyLevel(3720000, 2999999983600000, 82));
    levels.push_back(SubsidyLevel(3740000, 2999999985240000, 75));
    levels.push_back(SubsidyLevel(3760000, 2999999986740000, 68));
    levels.push_back(SubsidyLevel(3780000, 2999999988100000, 62));
    levels.push_back(SubsidyLevel(3800000, 2999999989340000, 57));
    levels.push_back(SubsidyLevel(3820000, 2999999990480000, 52));
    levels.push_back(SubsidyLevel(3840000, 2999999991520000, 47));
    levels.push_back(SubsidyLevel(3860000, 2999999992460000, 43));
    levels.push_back(SubsidyLevel(3880000, 2999999993320000, 39));
    levels.push_back(SubsidyLevel(3900000, 2999999994100000, 35));
    levels.push_back(SubsidyLevel(3920000, 2999999994800000, 32));
    levels.push_back(SubsidyLevel(3940000, 2999999995440000, 29));
    levels.push_back(SubsidyLevel(3960000, 2999999996020000, 26));
    levels.push_back(SubsidyLevel(3980000, 2999999996540000, 23));
    levels.push_back(SubsidyLevel(4000000, 2999999997000000, 21));
    levels.push_back(SubsidyLevel(4020000, 2999999997420000, 19));
    levels.push_back(SubsidyLevel(4040000, 2999999997800000, 17));
    levels.push_back(SubsidyLevel(4060000, 2999999998140000, 15));
    levels.push_back(SubsidyLevel(4080000, 2999999998440000, 13));
    levels.push_back(SubsidyLevel(4100000, 2999999998700000, 11));
    levels.push_back(SubsidyLevel(4120000, 2999999998920000, 10));
    levels.push_back(SubsidyLevel(4140000, 2999999999120000, 9));
    levels.push_back(SubsidyLevel(4160000, 2999999999300000, 8));
    levels.push_back(SubsidyLevel(4180000, 2999999999460000, 7));
    levels.push_back(SubsidyLevel(4200000, 2999999999600000, 6));
    levels.push_back(SubsidyLevel(4220000, 2999999999720000, 5));
    levels.push_back(SubsidyLevel(4240000, 2999999999820000, 4));
    levels.push_back(SubsidyLevel(4260000, 2999999999900000, 3));
    levels.push_back(SubsidyLevel(4280000, 2999999999960000, 2));
    levels.push_back(SubsidyLevel(4300000, 3000000000000000, 0));
    return levels;
}

/**
 * Subsidy levels for reg test params. Subsidy is
 * updated every 15 blocks.
 */
inline std::vector<SubsidyLevel> createRegTestSubsidyLevels(){
    std::vector<SubsidyLevel> levels;
    levels.push_back(SubsidyLevel(0, 0, 4000000000));
    levels.push_back(SubsidyLevel(15, 60000000000, 4600000000));
    levels.push_back(SubsidyLevel(30, 129000000000, 5200000000));
    levels.push_back(SubsidyLevel(45, 207000000000, 5900000000));
    levels.push_back(SubsidyLevel(60, 295500000000, 6700000000));
    levels.push_back(SubsidyLevel(75, 396000000000, 7700000000));
    levels.push_back(SubsidyLevel(90, 511500000000, 8800000000));
    levels.push_back(SubsidyLevel(105, 643500000000, 10120000000));
    levels.push_back(SubsidyLevel(120, 795300000000, 9200000000));
    levels.push_back(SubsidyLevel(135, 933300000000, 8400000000));
    levels.push_back(SubsidyLevel(150, 1059300000000, 7700000000));
    levels.push_back(SubsidyLevel(165, 1174800000000, 7000000000));
    levels.push_back(SubsidyLevel(180, 1279800000000, 6400000000));
    levels.push_back(SubsidyLevel(195, 1375800000000, 5800000000));
    levels.push_back(SubsidyLevel(210, 1462800000000, 5300000000));
    levels.push_back(SubsidyLevel(225, 1542300000000, 4800000000));
    levels.push_back(SubsidyLevel(240, 1614300000000, 4400000000));
    levels.push_back(SubsidyLevel(255, 1680300000000, 4000000000));
    levels.push_back(SubsidyLevel(270, 1740300000000, 3600000000));
    levels.push_back(SubsidyLevel(285, 1794300000000, 3300000000));
    levels.push_back(SubsidyLevel(300, 1843800000000, 3000000000));
    levels.push_back(SubsidyLevel(315, 1888800000000, 2700000000));
    levels.push_back(SubsidyLevel(330, 1929300000000, 2400000000));
    levels.push_back(SubsidyLevel(345, 1965300000000, 2200000000));
    levels.push_back(SubsidyLevel(360, 1998300000000, 2000000000));
    levels.push_back(SubsidyLevel(375, 2028300000000, 1800000000));
    levels.push_back(SubsidyLevel(390, 2055300000000, 1600000000));
    levels.push_back(SubsidyLevel(405, 2079300000000, 1400000000));
    levels.push_back(SubsidyLevel(420, 2100300000000, 1200000000));
    levels.push_back(SubsidyLevel(435, 2118300000000, 1100000000));
    levels.push_back(SubsidyLevel(450, 2134800000000, 1000000000));
    levels.push_back(SubsidyLevel(465, 2149800000000, 910000000));
    levels.push_back(SubsidyLevel(480, 2163450000000, 830000000));
    levels.push_back(SubsidyLevel(495, 2175900000000, 760000000));
    levels.push_back(SubsidyLevel(510, 2187300000000, 690000000));
    levels.push_back(SubsidyLevel(525, 2197650000000, 630000000));
    levels.push_back(SubsidyLevel(540, 2207100000000, 570000000));
    levels.push_back(SubsidyLevel(555, 2215650000000, 520000000));
    levels.push_back(SubsidyLevel(570, 2223450000000, 470000000));
    levels.push_back(SubsidyLevel(585, 2230500000000, 430000000));
    levels.push_back(SubsidyLevel(600, 2236950000000, 390000000));
    levels.push_back(SubsidyLevel(615, 2242800000000, 350000000));
    levels.push_back(SubsidyLevel(630, 2248050000000, 320000000));
    levels.push_back(SubsidyLevel(645, 2252850000000, 290000000));
    levels.push_back(SubsidyLevel(660, 2257200000000, 260000000));
    levels.push_back(SubsidyLevel(675, 2261100000000, 230000000));
    levels.push_back(SubsidyLevel(690, 2264550000000, 210000000));
    levels.push_back(SubsidyLevel(705, 2267700000000, 190000000));
    levels.push_back(SubsidyLevel(720, 2270550000000, 170000000));
    levels.push_back(SubsidyLevel(735, 2273100000000, 150000000));
    levels.push_back(SubsidyLevel(750, 2275350000000, 130000000));
    levels.push_back(SubsidyLevel(765, 2277300000000, 110000000));
    levels.push_back(SubsidyLevel(780, 2278950000000, 100000000));
    levels.push_back(SubsidyLevel(795, 2280450000000, 91000000));
    levels.push_back(SubsidyLevel(810, 2281815000000, 83000000));
    levels.push_back(SubsidyLevel(825, 2283060000000, 76000000));
    levels.push_back(SubsidyLevel(840, 2284200000000, 69000000));
    levels.push_back(SubsidyLevel(855, 2285235000000, 63000000));
    levels.push_back(SubsidyLevel(870, 2286180000000, 57000000));
    levels.push_back(SubsidyLevel(885, 2287035000000, 52000000));
    levels.push_back(SubsidyLevel(900, 2287815000000, 47000000));
    levels.push_back(SubsidyLevel(915, 2288520000000, 43000000));
    levels.push_back(SubsidyLevel(930, 2289165000000, 39000000));
    levels.push_back(SubsidyLevel(945, 2289750000000, 35000000));
    levels.push_back(SubsidyLevel(960, 2290275000000, 32000000));
    levels.push_back(SubsidyLevel(975, 2290755000000, 29000000));
    levels.push_back(SubsidyLevel(990, 2291190000000, 26000000));
    levels.push_back(SubsidyLevel(1005, 2291580000000, 23000000));
    levels.push_back(SubsidyLevel(1020, 2291925000000, 21000000));
    levels.push_back(SubsidyLevel(1035, 2292240000000, 19000000));
    levels.push_back(SubsidyLevel(1050, 2292525000000, 17000000));
    levels.push_back(SubsidyLevel(1065, 2292780000000, 15000000));
    levels.push_back(SubsidyLevel(1080, 2293005000000, 13000000));
    levels.push_back(SubsidyLevel(1095, 2293200000000, 11000000));
    levels.push_back(SubsidyLevel(1110, 2293365000000, 10000000));
    levels.push_back(SubsidyLevel(1125, 2293515000000, 9100000));
    levels.push_back(SubsidyLevel(1140, 2293651500000, 8300000));
    levels.push_back(SubsidyLevel(1155, 2293776000000, 7600000));
    levels.push_back(SubsidyLevel(1170, 2293890000000, 6900000));
    levels.push_back(SubsidyLevel(1185, 2293993500000, 6300000));
    levels.push_back(SubsidyLevel(1200, 2294088000000, 5700000));
    levels.push_back(SubsidyLevel(1215, 2294173500000, 5200000));
    levels.push_back(SubsidyLevel(1230, 2294251500000, 4700000));
    levels.push_back(SubsidyLevel(1245, 2294322000000, 4300000));
    levels.push_back(SubsidyLevel(1260, 2294386500000, 3900000));
    levels.push_back(SubsidyLevel(1275, 2294445000000, 3500000));
    levels.push_back(SubsidyLevel(1290, 2294497500000, 3200000));
    levels.push_back(SubsidyLevel(1305, 2294545500000, 2900000));
    levels.push_back(SubsidyLevel(1320, 2294589000000, 2600000));
    levels.push_back(SubsidyLevel(1335, 2294628000000, 2300000));
    levels.push_back(SubsidyLevel(1350, 2294662500000, 2100000));
    levels.push_back(SubsidyLevel(1365, 2294694000000, 1900000));
    levels.push_back(SubsidyLevel(1380, 2294722500000, 1700000));
    levels.push_back(SubsidyLevel(1395, 2294748000000, 1500000));
    levels.push_back(SubsidyLevel(1410, 2294770500000, 1300000));
    levels.push_back(SubsidyLevel(1425, 2294790000000, 1100000));
    levels.push_back(SubsidyLevel(1440, 2294806500000, 1000000));
    levels.push_back(SubsidyLevel(1455, 2294821500000, 910000));
    levels.push_back(SubsidyLevel(1470, 2294835150000, 830000));
    levels.push_back(SubsidyLevel(1485, 2294847600000, 760000));
    levels.push_back(SubsidyLevel(1500, 2294859000000, 690000));
    levels.push_back(SubsidyLevel(1515, 2294869350000, 630000));
    levels.push_back(SubsidyLevel(1530, 2294878800000, 570000));
    levels.push_back(SubsidyLevel(1545, 2294887350000, 520000));
    levels.push_back(SubsidyLevel(1560, 2294895150000, 470000));
    levels.push_back(SubsidyLevel(1575, 2294902200000, 430000));
    levels.push_back(SubsidyLevel(1590, 2294908650000, 390000));
    levels.push_back(SubsidyLevel(1605, 2294914500000, 350000));
    levels.push_back(SubsidyLevel(1620, 2294919750000, 320000));
    levels.push_back(SubsidyLevel(1635, 2294924550000, 290000));
    levels.push_back(SubsidyLevel(1650, 2294928900000, 260000));
    levels.push_back(SubsidyLevel(1665, 2294932800000, 230000));
    levels.push_back(SubsidyLevel(1680, 2294936250000, 210000));
    levels.push_back(SubsidyLevel(1695, 2294939400000, 190000));
    levels.push_back(SubsidyLevel(1710, 2294942250000, 170000));
    levels.push_back(SubsidyLevel(1725, 2294944800000, 150000));
    levels.push_back(SubsidyLevel(1740, 2294947050000, 130000));
    levels.push_back(SubsidyLevel(1755, 2294949000000, 110000));
    levels.push_back(SubsidyLevel(1770, 2294950650000, 100000));
    levels.push_back(SubsidyLevel(1785, 2294952150000, 91000));
    levels.push_back(SubsidyLevel(1800, 2294953515000, 83000));
    levels.push_back(SubsidyLevel(1815, 2294954760000, 76000));
    levels.push_back(SubsidyLevel(1830, 2294955900000, 69000));
    levels.push_back(SubsidyLevel(1845, 2294956935000, 63000));
    levels.push_back(SubsidyLevel(1860, 2294957880000, 57000));
    levels.push_back(SubsidyLevel(1875, 2294958735000, 52000));
    levels.push_back(SubsidyLevel(1890, 2294959515000, 47000));
    levels.push_back(SubsidyLevel(1905, 2294960220000, 43000));
    levels.push_back(SubsidyLevel(1920, 2294960865000, 39000));
    levels.push_back(SubsidyLevel(1935, 2294961450000, 35000));
    levels.push_back(SubsidyLevel(1950, 2294961975000, 32000));
    levels.push_back(SubsidyLevel(1965, 2294962455000, 29000));
    levels.push_back(SubsidyLevel(1980, 2294962890000, 26000));
    levels.push_back(SubsidyLevel(1995, 2294963280000, 23000));
    levels.push_back(SubsidyLevel(2010, 2294963625000, 21000));
    levels.push_back(SubsidyLevel(2025, 2294963940000, 19000));
    levels.push_back(SubsidyLevel(2040, 2294964225000, 17000));
    levels.push_back(SubsidyLevel(2055, 2294964480000, 15000));
    levels.push_back(SubsidyLevel(2070, 2294964705000, 13000));
    levels.push_back(SubsidyLevel(2085, 2294964900000, 11000));
    levels.push_back(SubsidyLevel(2100, 2294965065000, 10000));
    levels.push_back(SubsidyLevel(2115, 2294965215000, 9100));
    levels.push_back(SubsidyLevel(2130, 2294965351500, 8300));
    levels.push_back(SubsidyLevel(2145, 2294965476000, 7600));
    levels.push_back(SubsidyLevel(2160, 2294965590000, 6900));
    levels.push_back(SubsidyLevel(2175, 2294965693500, 6300));
    levels.push_back(SubsidyLevel(2190, 2294965788000, 5700));
    levels.push_back(SubsidyLevel(2205, 2294965873500, 5200));
    levels.push_back(SubsidyLevel(2220, 2294965951500, 4700));
    levels.push_back(SubsidyLevel(2235, 2294966022000, 4300));
    levels.push_back(SubsidyLevel(2250, 2294966086500, 3900));
    levels.push_back(SubsidyLevel(2265, 2294966145000, 3500));
    levels.push_back(SubsidyLevel(2280, 2294966197500, 3200));
    levels.push_back(SubsidyLevel(2295, 2294966245500, 2900));
    levels.push_back(SubsidyLevel(2310, 2294966289000, 2600));
    levels.push_back(SubsidyLevel(2325, 2294966328000, 2300));
    levels.push_back(SubsidyLevel(2340, 2294966362500, 2100));
    levels.push_back(SubsidyLevel(2355, 2294966394000, 1900));
    levels.push_back(SubsidyLevel(2370, 2294966422500, 1700));
    levels.push_back(SubsidyLevel(2385, 2294966448000, 1500));
    levels.push_back(SubsidyLevel(2400, 2294966470500, 1300));
    levels.push_back(SubsidyLevel(2415, 2294966490000, 1100));
    levels.push_back(SubsidyLevel(2430, 2294966506500, 1000));
    levels.push_back(SubsidyLevel(2445, 2294966521500, 910));
    levels.push_back(SubsidyLevel(2460, 2294966535150, 830));
    levels.push_back(SubsidyLevel(2475, 2294966547600, 760));
    levels.push_back(SubsidyLevel(2490, 2294966559000, 690));
    levels.push_back(SubsidyLevel(2505, 2294966569350, 630));
    levels.push_back(SubsidyLevel(2520, 2294966578800, 570));
    levels.push_back(SubsidyLevel(2535, 2294966587350, 520));
    levels.push_back(SubsidyLevel(2550, 2294966595150, 470));
    levels.push_back(SubsidyLevel(2565, 2294966602200, 430));
    levels.push_back(SubsidyLevel(2580, 2294966608650, 390));
    levels.push_back(SubsidyLevel(2595, 2294966614500, 350));
    levels.push_back(SubsidyLevel(2610, 2294966619750, 320));
    levels.push_back(SubsidyLevel(2625, 2294966624550, 290));
    levels.push_back(SubsidyLevel(2640, 2294966628900, 260));
    levels.push_back(SubsidyLevel(2655, 2294966632800, 230));
    levels.push_back(SubsidyLevel(2670, 2294966636250, 210));
    levels.push_back(SubsidyLevel(2685, 2294966639400, 190));
    levels.push_back(SubsidyLevel(2700, 2294966642250, 170));
    levels.push_back(SubsidyLevel(2715, 2294966644800, 150));
    levels.push_back(SubsidyLevel(2730, 2294966647050, 130));
    levels.push_back(SubsidyLevel(2745, 2294966649000, 110));
    levels.push_back(SubsidyLevel(2760, 2294966650650, 100));
    levels.push_back(SubsidyLevel(2775, 2294966652150, 91));
    levels.push_back(SubsidyLevel(2790, 2294966653515, 83));
    levels.push_back(SubsidyLevel(2805, 2294966654760, 76));
    levels.push_back(SubsidyLevel(2820, 2294966655900, 69));
    levels.push_back(SubsidyLevel(2835, 2294966656935, 63));
    levels.push_back(SubsidyLevel(2850, 2294966657880, 57));
    levels.push_back(SubsidyLevel(2865, 2294966658735, 52));
    levels.push_back(SubsidyLevel(2880, 2294966659515, 47));
    levels.push_back(SubsidyLevel(2895, 2294966660220, 43));
    levels.push_back(SubsidyLevel(2910, 2294966660865, 39));
    levels.push_back(SubsidyLevel(2925, 2294966661450, 35));
    levels.push_back(SubsidyLevel(2940, 2294966661975, 32));
    levels.push_back(SubsidyLevel(2955, 2294966662455, 29));
    levels.push_back(SubsidyLevel(2970, 2294966662890, 26));
    levels.push_back(SubsidyLevel(2985, 2294966663280, 23));
    levels.push_back(SubsidyLevel(3000, 2294966663625, 21));
    levels.push_back(SubsidyLevel(3015, 2294966663940, 19));
    levels.push_back(SubsidyLevel(3030, 2294966664225, 17));
    levels.push_back(SubsidyLevel(3045, 2294966664480, 15));
    levels.push_back(SubsidyLevel(3060, 2294966664705, 13));
    levels.push_back(SubsidyLevel(3075, 2294966664900, 11));
    levels.push_back(SubsidyLevel(3090, 2294966665065, 10));
    levels.push_back(SubsidyLevel(3105, 2294966665215, 9));
    levels.push_back(SubsidyLevel(3120, 2294966665350, 8));
    levels.push_back(SubsidyLevel(3135, 2294966665470, 7));
    levels.push_back(SubsidyLevel(3150, 2294966665575, 6));
    levels.push_back(SubsidyLevel(3165, 2294966665665, 5));
    levels.push_back(SubsidyLevel(3180, 2294966665740, 4));
    levels.push_back(SubsidyLevel(3195, 2294966665800, 3));
    levels.push_back(SubsidyLevel(3210, 2294966665845, 2));
    levels.push_back(SubsidyLevel(3225, 2294966665875, 0));
    return levels;
}

#endif
