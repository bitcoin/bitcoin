// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <cluster_linearize.h>
#include <test/util/cluster_linearize.h>
#include <util/bitset.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

using namespace cluster_linearize;
using namespace util::hex_literals;

namespace {

/** Construct a linear graph. These are pessimal for AncestorCandidateFinder, as they maximize
 *  the number of ancestor set feerate updates. The best ancestor set is always the topmost
 *  remaining transaction, whose removal requires updating all remaining transactions' ancestor
 *  set feerates. */
template<typename SetType>
DepGraph<SetType> MakeLinearGraph(ClusterIndex ntx)
{
    DepGraph<SetType> depgraph;
    for (ClusterIndex i = 0; i < ntx; ++i) {
        depgraph.AddTransaction({-int32_t(i), 1});
        if (i > 0) depgraph.AddDependency(i - 1, i);
    }
    return depgraph;
}

/** Construct a wide graph (one root, with N-1 children that are otherwise unrelated, with
 *  increasing feerates). These graphs are pessimal for the LIMO step in Linearize, because
 *  rechunking is needed after every candidate (the last transaction gets picked every time).
 */
template<typename SetType>
DepGraph<SetType> MakeWideGraph(ClusterIndex ntx)
{
    DepGraph<SetType> depgraph;
    for (ClusterIndex i = 0; i < ntx; ++i) {
        depgraph.AddTransaction({int32_t(i) + 1, 1});
        if (i > 0) depgraph.AddDependency(0, i);
    }
    return depgraph;
}

// Construct a difficult graph. These need at least sqrt(2^(n-1)) iterations in the best
// known algorithms (purely empirically determined).
template<typename SetType>
DepGraph<SetType> MakeHardGraph(ClusterIndex ntx)
{
    DepGraph<SetType> depgraph;
    for (ClusterIndex i = 0; i < ntx; ++i) {
        if (ntx & 1) {
            // Odd cluster size.
            //
            // Mermaid diagram code for the resulting cluster for 11 transactions:
            // ```mermaid
            // graph BT
            // T0["T0: 1/2"];T1["T1: 14/2"];T2["T2: 6/1"];T3["T3: 5/1"];T4["T4: 7/1"];
            // T5["T5: 5/1"];T6["T6: 7/1"];T7["T7: 5/1"];T8["T8: 7/1"];T9["T9: 5/1"];
            // T10["T10: 7/1"];
            // T1-->T0;T1-->T2;T3-->T2;T4-->T3;T4-->T5;T6-->T5;T4-->T7;T8-->T7;T4-->T9;T10-->T9;
            // ```
            if (i == 0) {
                depgraph.AddTransaction({1, 2});
            } else if (i == 1) {
                depgraph.AddTransaction({14, 2});
                depgraph.AddDependency(0, 1);
            } else if (i == 2) {
                depgraph.AddTransaction({6, 1});
                depgraph.AddDependency(2, 1);
            } else if (i == 3) {
                depgraph.AddTransaction({5, 1});
                depgraph.AddDependency(2, 3);
            } else if ((i & 1) == 0) {
                depgraph.AddTransaction({7, 1});
                depgraph.AddDependency(i - 1, i);
            } else {
                depgraph.AddTransaction({5, 1});
                depgraph.AddDependency(i, 4);
            }
        } else {
            // Even cluster size.
            //
            // Mermaid diagram code for the resulting cluster for 10 transactions:
            // ```mermaid
            // graph BT
            // T0["T0: 1"];T1["T1: 3"];T2["T2: 1"];T3["T3: 4"];T4["T4: 0"];T5["T5: 4"];T6["T6: 0"];
            // T7["T7: 4"];T8["T8: 0"];T9["T9: 4"];
            // T1-->T0;T2-->T0;T3-->T2;T3-->T4;T5-->T4;T3-->T6;T7-->T6;T3-->T8;T9-->T8;
            // ```
            if (i == 0) {
                depgraph.AddTransaction({1, 1});
            } else if (i == 1) {
                depgraph.AddTransaction({3, 1});
                depgraph.AddDependency(0, 1);
            } else if (i == 2) {
                depgraph.AddTransaction({1, 1});
                depgraph.AddDependency(0, 2);
            } else if (i & 1) {
                depgraph.AddTransaction({4, 1});
                depgraph.AddDependency(i - 1, i);
            } else {
                depgraph.AddTransaction({0, 1});
                depgraph.AddDependency(i, 3);
            }
        }
    }
    return depgraph;
}

/** Benchmark that does search-based candidate finding with a specified number of iterations.
 *
 * Its goal is measuring how much time every additional search iteration in linearization costs,
 * by running with a low and a high count, subtracting the results, and divided by the number
 * iterations difference.
 */
template<typename SetType>
void BenchLinearizeWorstCase(ClusterIndex ntx, benchmark::Bench& bench, uint64_t iter_limit)
{
    const auto depgraph = MakeHardGraph<SetType>(ntx);
    uint64_t rng_seed = 0;
    bench.run([&] {
        SearchCandidateFinder finder(depgraph, rng_seed++);
        auto [candidate, iters_performed] = finder.FindCandidateSet(iter_limit, {});
        assert(iters_performed == iter_limit);
    });
}

/** Benchmark for linearization improvement of a trivial linear graph using just ancestor sort.
 *
 * Its goal is measuring how much time linearization may take without any search iterations.
 *
 * If P is the benchmarked per-iteration count (obtained by running BenchLinearizeWorstCase for a
 * high and a low iteration count, subtracting them, and dividing by the difference in count), and
 * N is the resulting time of BenchLinearizeNoItersWorstCase*, then an invocation of Linearize with
 * max_iterations=m should take no more than roughly N+m*P time. This may however be an
 * overestimate, as the worst cases do not coincide (the ones that are worst for linearization
 * without any search happen to be ones that do not need many search iterations).
 *
 * This benchmark exercises a worst case for AncestorCandidateFinder, but for which improvement is
 * cheap.
 */
template<typename SetType>
void BenchLinearizeNoItersWorstCaseAnc(ClusterIndex ntx, benchmark::Bench& bench)
{
    const auto depgraph = MakeLinearGraph<SetType>(ntx);
    uint64_t rng_seed = 0;
    std::vector<ClusterIndex> old_lin(ntx);
    for (ClusterIndex i = 0; i < ntx; ++i) old_lin[i] = i;
    bench.run([&] {
        Linearize(depgraph, /*max_iterations=*/0, rng_seed++, old_lin);
    });
}

/** Benchmark for linearization improvement of a trivial wide graph using just ancestor sort.
 *
 * Its goal is measuring how much time improving a linearization may take without any search
 * iterations, similar to the previous function.
 *
 * This benchmark exercises a worst case for improving an existing linearization, but for which
 * AncestorCandidateFinder is cheap.
 */
template<typename SetType>
void BenchLinearizeNoItersWorstCaseLIMO(ClusterIndex ntx, benchmark::Bench& bench)
{
    const auto depgraph = MakeWideGraph<SetType>(ntx);
    uint64_t rng_seed = 0;
    std::vector<ClusterIndex> old_lin(ntx);
    for (ClusterIndex i = 0; i < ntx; ++i) old_lin[i] = i;
    bench.run([&] {
        Linearize(depgraph, /*max_iterations=*/0, rng_seed++, old_lin);
    });
}

template<typename SetType>
void BenchPostLinearizeWorstCase(ClusterIndex ntx, benchmark::Bench& bench)
{
    DepGraph<SetType> depgraph = MakeWideGraph<SetType>(ntx);
    std::vector<ClusterIndex> lin(ntx);
    bench.run([&] {
        for (ClusterIndex i = 0; i < ntx; ++i) lin[i] = i;
        PostLinearize(depgraph, lin);
    });
}

template<typename SetType>
void BenchMergeLinearizationsWorstCase(ClusterIndex ntx, benchmark::Bench& bench)
{
    DepGraph<SetType> depgraph;
    for (ClusterIndex i = 0; i < ntx; ++i) {
        depgraph.AddTransaction({i, 1});
        if (i) depgraph.AddDependency(0, i);
    }
    std::vector<ClusterIndex> lin1;
    std::vector<ClusterIndex> lin2;
    lin1.push_back(0);
    lin2.push_back(0);
    for (ClusterIndex i = 1; i < ntx; ++i) {
        lin1.push_back(i);
        lin2.push_back(ntx - i);
    }
    bench.run([&] {
        MergeLinearizations(depgraph, lin1, lin2);
    });
}

template<size_t N>
void BenchLinearizeOptimally(benchmark::Bench& bench, const std::array<uint8_t, N>& serialized)
{
    // Determine how many transactions the serialized cluster has.
    ClusterIndex num_tx{0};
    {
        SpanReader reader{serialized};
        DepGraph<BitSet<128>> depgraph;
        reader >> Using<DepGraphFormatter>(depgraph);
        num_tx = depgraph.TxCount();
        assert(num_tx < 128);
    }

    SpanReader reader{serialized};
    auto runner_fn = [&]<typename SetType>() noexcept {
        DepGraph<SetType> depgraph;
        reader >> Using<DepGraphFormatter>(depgraph);
        uint64_t rng_seed = 0;
        bench.run([&] {
            auto res = Linearize(depgraph, /*max_iterations=*/10000000, rng_seed++);
            assert(res.second);
        });
    };

    if (num_tx <= 32) {
        runner_fn.template operator()<BitSet<32>>();
    } else if (num_tx <= 64) {
        runner_fn.template operator()<BitSet<64>>();
    } else if (num_tx <= 96) {
        runner_fn.template operator()<BitSet<96>>();
    } else if (num_tx <= 128) {
        runner_fn.template operator()<BitSet<128>>();
    } else {
        assert(false);
    }
}

} // namespace

static void Linearize16TxWorstCase20Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<16>>(16, bench, 20); }
static void Linearize16TxWorstCase120Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<16>>(16, bench, 120); }
static void Linearize32TxWorstCase5000Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<32>>(32, bench, 5000); }
static void Linearize32TxWorstCase15000Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<32>>(32, bench, 15000); }
static void Linearize48TxWorstCase5000Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<48>>(48, bench, 5000); }
static void Linearize48TxWorstCase15000Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<48>>(48, bench, 15000); }
static void Linearize64TxWorstCase5000Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<64>>(64, bench, 5000); }
static void Linearize64TxWorstCase15000Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<64>>(64, bench, 15000); }
static void Linearize75TxWorstCase5000Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<75>>(75, bench, 5000); }
static void Linearize75TxWorstCase15000Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<75>>(75, bench, 15000); }
static void Linearize99TxWorstCase5000Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<99>>(99, bench, 5000); }
static void Linearize99TxWorstCase15000Iters(benchmark::Bench& bench) { BenchLinearizeWorstCase<BitSet<99>>(99, bench, 15000); }

static void LinearizeNoIters16TxWorstCaseAnc(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseAnc<BitSet<16>>(16, bench); }
static void LinearizeNoIters32TxWorstCaseAnc(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseAnc<BitSet<32>>(32, bench); }
static void LinearizeNoIters48TxWorstCaseAnc(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseAnc<BitSet<48>>(48, bench); }
static void LinearizeNoIters64TxWorstCaseAnc(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseAnc<BitSet<64>>(64, bench); }
static void LinearizeNoIters75TxWorstCaseAnc(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseAnc<BitSet<75>>(75, bench); }
static void LinearizeNoIters99TxWorstCaseAnc(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseAnc<BitSet<99>>(99, bench); }

static void LinearizeNoIters16TxWorstCaseLIMO(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseLIMO<BitSet<16>>(16, bench); }
static void LinearizeNoIters32TxWorstCaseLIMO(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseLIMO<BitSet<32>>(32, bench); }
static void LinearizeNoIters48TxWorstCaseLIMO(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseLIMO<BitSet<48>>(48, bench); }
static void LinearizeNoIters64TxWorstCaseLIMO(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseLIMO<BitSet<64>>(64, bench); }
static void LinearizeNoIters75TxWorstCaseLIMO(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseLIMO<BitSet<75>>(75, bench); }
static void LinearizeNoIters99TxWorstCaseLIMO(benchmark::Bench& bench) { BenchLinearizeNoItersWorstCaseLIMO<BitSet<99>>(99, bench); }

static void PostLinearize16TxWorstCase(benchmark::Bench& bench) { BenchPostLinearizeWorstCase<BitSet<16>>(16, bench); }
static void PostLinearize32TxWorstCase(benchmark::Bench& bench) { BenchPostLinearizeWorstCase<BitSet<32>>(32, bench); }
static void PostLinearize48TxWorstCase(benchmark::Bench& bench) { BenchPostLinearizeWorstCase<BitSet<48>>(48, bench); }
static void PostLinearize64TxWorstCase(benchmark::Bench& bench) { BenchPostLinearizeWorstCase<BitSet<64>>(64, bench); }
static void PostLinearize75TxWorstCase(benchmark::Bench& bench) { BenchPostLinearizeWorstCase<BitSet<75>>(75, bench); }
static void PostLinearize99TxWorstCase(benchmark::Bench& bench) { BenchPostLinearizeWorstCase<BitSet<99>>(99, bench); }

static void MergeLinearizations16TxWorstCase(benchmark::Bench& bench) { BenchMergeLinearizationsWorstCase<BitSet<16>>(16, bench); }
static void MergeLinearizations32TxWorstCase(benchmark::Bench& bench) { BenchMergeLinearizationsWorstCase<BitSet<32>>(32, bench); }
static void MergeLinearizations48TxWorstCase(benchmark::Bench& bench) { BenchMergeLinearizationsWorstCase<BitSet<48>>(48, bench); }
static void MergeLinearizations64TxWorstCase(benchmark::Bench& bench) { BenchMergeLinearizationsWorstCase<BitSet<64>>(64, bench); }
static void MergeLinearizations75TxWorstCase(benchmark::Bench& bench) { BenchMergeLinearizationsWorstCase<BitSet<75>>(75, bench); }
static void MergeLinearizations99TxWorstCase(benchmark::Bench& bench) { BenchMergeLinearizationsWorstCase<BitSet<99>>(99, bench); }

// The following example clusters were constructed by replaying historical mempool activity, and
// selecting for ones that take many iterations (after the introduction of some but not all
// linearization algorithm optimizations).

/* 2023-05-05T23:12:21Z 71, 521780, 543141,*/
static constexpr auto BENCH_EXAMPLE_00 = "801081a5360092239efc6201810982ab58029b6b98c86803800eed7804800ecb7e058f2f878778068030d43407853e81902a08962a81d176098010b6620a8010b2280b8010da3a0c9f069da9580d800db11e0e9d719ad37a0f967897ed5210990e99fc0e11812c81982012804685823e0f0a893982b6040a10804682c146110a6e80db5c120a8010819806130a8079858f0c140a8054829a120c12803483a1760c116f81843c0d11718189000e11800d81ac2c0f11800d81e50e10117181c77c1111822e87f2601012815983d17211127180f2121212811584a21e1312800e80d1781412813c83e81815126f80ef5016126f80ff6c16126f80f66017126e80fd541812800d81942a1912800e80dd781a12800d81f96c1b12805282e7581b127180fd721c1271a918230b805fc11a220d8118a15a2d036f80e5002011817684d8241e346f80e1181c37805082fc04260024800d81f8621734803382b354270b12805182ca2e162f800e80d52e0d32803dc360201b850e818c400b318c49808a5a290210805181d65823142a800d81a34e0850800e81fb3c0851886994fc0a280b00082c805482d208032e28805e83ba380059801081cd4a0159811884f770002e0015e17280e49024300a0000000000000031803dcb48014200"_hex_u8;
/* 2023-12-06T09:30:01Z 81, 141675, 647053,*/
static constexpr auto BENCH_EXAMPLE_01 = "b348f1fc4000f365818a9e2c01b44cf7ca0002b004f0b02003b33ef8ae3004b334f9e87005800d81c85e06b368fae26007b05ef2e14208be1a8093a50409b15cf5ee500a802c80a1420b802dea440c802ce50a0d802cdc320e802cd7220f802dd72210805380f74a118174f370126e96b32812127182c4701312817389d26414128035848c221512800e82bf3816126f81e4341712801082b228181280518af57418128040859a0019127182d0401a12803e858b641b127182c4421c126f82b3481d12811486b6301e12821d89e7281f126e8a8b421f127182d6642012806284c12021126e81d34822126e86a76222126e86d8102212805187b6542312800d82fc002412803d848e0e2512801082d27a26126e8589642612800e83a9602712800e83bd0028126e81ef1a29116e858d7228126f82db5e2912801083843c2a127181c93c2b126e85d0162b127181c5622c126e84f8262c12800f8392202d12800e82b66c2e126e81d0082f12803282d50430126e84f9003012805f84be6c3112846e88df0e2b12804080d44c340a8b31898808350a800ed760350b801083a1182b517182817e2a51800e82b6582951803583cb52420030806284cb6c204f7181d300204f82688ce0303e001d800e82bb200f488010808a182822a3289cd63041000a6fcd100a408a7caaa7024800002f803584e0741e27288f3386dd783b001000802683f27e004b8c44bcd0763f0000000000000000000100000e00"_hex_u8;
/* 2023-04-04T00:26:50Z 90, 99930, 529375,*/
static constexpr auto BENCH_EXAMPLE_02 = "815b80b61e00800da63001cd378da70e028010991a03800e9d3e0480109708058010991a068010973a07da738fa72408de7491831009b35b88f0080a9d4485de180b71974e0c71974e0d80108e500eb27988a75a0f719632108061a56c11801087761280108a1413807893441480538c1415a606828806168010893e1780548c40188e4b80bb2c196eab3e1718805ed60e18188051c97a19188010cf781a1871b11e1b1871c5281c1880508080581d186e80b13c1e188035cf421f18805fe0482018804caa661f198035a9001f156e80cb701d1871a2281e1871ad281f18817380a16020186f98642118805ee04821198010b6702219800ea12623196eb67024198035808b0025196fa65c26198054ba1c2719807680bf7c28198053cd782919803d80b80429198051db5a2a198040d3742b19976584bb1c28196efc1c281971b21a29198052bc762a1971a2502b196eb73c2c19976381ab0c2a18806290543409862081c3423b00336fbc70224d80109e7c1c52805ebd5c1942800eb57016468034ba423405158118da28350416927480f4743000159f6a81c9462e00188051ec5e380e00800e9e420775800d9e26007c906c82f754251d0025870480f12c14280023800d9e26027e9e1385ed08102900001a804fac7a018001719856028001800da87e0180039b1a868b60064102246e9f42018005800da87e028005850d81d600026d862381a2200e0008230015831480a5480342000524803eeb32006e873582a4700a0100351300"_hex_u8;
/* 2023-05-08T15:51:59Z 87, 76869, 505222,*/
static constexpr auto BENCH_EXAMPLE_03 = "c040b9e15a00b10eac842601805f85931802c104bae17403ae50aaa336049d76a9bf7005c55bbeab6606ae2aa9c72c07805e81992e08af7dab817a096e80a7e4520909803e92bd780a097185c76c0b096e98e7380b09850bb9953c0c09803389f6260d096f859d620e09803f88d3000f0971829c6e1009837690f6481109806285931811097181f56814076ea09b74120980408eb73213096f87853214096f86e2701509803f8c860016098a6fe6c3721709814f92a204180980628a8a441909803285df681a0980348498661b096e8290781c096e978e081c097187da1a1d097186c05c1e097185893c1f09805f8ad9002009800d84e74e21097183a67a22097182e23423097184b53a23096ea393062309840faddd46240980618eb732250980548bee6a2609807986883c2709718298402809815388b6582909805384ec742a097181b9142b096e97b5262b096e85e14e2c0980518abb5c2d09805489e75a2e09803187e3382f097180eb1c34046f87c34a2f098309a5c54430097186911831098054899c083209801083bc1033097081e02a3409805f848f0c35096e80d4343a057180c37040006f80a22438097180a0503f03816f8381444003803f80ef003f05800580a4283f066ef72845016efb91663e09923d808d8216470041803584837c46012f9247dc86684501268267a09610450222862184db68440712803585ea40440113835d97887805800b8723c7a40a4b00022f81529ae2143c0c1f80548b8f381b311980408e955c055e802589dc10037e801083b54602658010848130006700"_hex_u8;
/* 2023-05-01T19:32:10Z 35, 55747, 504128,*/
static constexpr auto BENCH_EXAMPLE_04 = "801af95c00801af72801801af95c02873e85f2180202873e85f2180202873e85f21802028018fb2802068018fb2803068018fb2804068018fb2805068018fb2806068018fb2807068018fb2808068018fb2809068018fb280a068018fb280a058018fb280b058018fb280c058018fb280d058018fb280e058018fb280f058018fb2810058018fb2811058018fb2812058018fb2813058018fb2814058018fb2815058018fb2815048018fb2816048018fb2817048018fb2818048018fb2819048018fb281a048018fb281b04810d80d9481f00000100"_hex_u8;
/* 2023-02-27T17:06:38Z 60, 55680, 502749,*/
static constexpr auto BENCH_EXAMPLE_05 = "b5108ab56600b26d89f85601b07383b01602b22683c96003b34a83d82e04b12f83b53a05b20e83c75a066e80840a06068040be0007066fb10608066fb2120906800eba320a06842b80b05a0a066eff420b067199300b068124c3140c0680618085180d066faa1c0e068010b4440f068051af541006800da1781106857881946812066eee1613068052b31014068324808d361506806180885c150671b03216066ef11017068052b63218066ef3521806803f80865419066e93441a068035a13e1b0680628085181c06806ec4481d068117e72c1e06719c721f068077c42420068159808d1821066eef0c21058010b90022056f9908230571993024058010b00a25058010b00a260580608087402705803fc10027068032b42828068051b6322906800db11e212a8324808d361933803ff400192f826381a7141a2f8032ac08152a800db54c044e8323808d3630010002018158d84000042d821cea12002807853580d462002d01891181d022002e00"_hex_u8;
/* 2023-04-20T22:25:49Z 99, 49100, 578622,*/
static constexpr auto BENCH_EXAMPLE_06 = "bf3c87c14c008010955a01b21d85e07002800d946c036e8e3404b77f86c26605b33c85f55e06bd06879852078010970a08bd4b87cf00098123a7720ab2158687680b8054d4440b0a8062fa4c0c0a71ac400d0a80628081540e0a8010a2580f0a8054b676100a8032b85c110a6e9a40120a6e809012130a817f80c31e140a8175808674150a719d46160a8172d86415098033c1481609800da4181709800ada2e1809803dc85219098034b4041a096ef5501b098052d67c1c098051d3281d09800ebc4a1e098175808c641f098061c55020098078c85021096e8081141f0b6faf1e200b8061da68210b8062f000220b800ebc20230b8035d058240b8053de32250b8050b610250b6fad32260b803dc276270b803d80a610280b6ef812290b8052b6322a0b800eb57e2b0b8052bd062c0b719e522d0b71a3762e0b8010bb1e2f0b80109a78310a80109962320a8051a60c330a6f9f3e320b6e808b24330b719e40340b8117cc50350b803d80971a360b8051b930370b6f9e0a380b719b10390b8052a6003a0b6e808c76390a7195603a0a6f935c3b0a8054a31a3c0a803ce30c3b0b803fa3003c0b800dbe2a3d0b8f3480a84244058005851a44069d1bf824400b83098f284507719c723d4f6f9c1c3449719c722f4f6eb23c304f8061c5502e528061da682b4e8118bb724e022a8054b35028476e941c1d51815be02c4f01148557808e3a4f070e8104af464e001180329d364e010d805f9f6a421b9c3387aa744c0d4d71ac400b800881748098444710338173809b780b80008054d444292c12821dc040550403078b4682b4664517003f00"_hex_u8;
/* 2023-06-05T19:56:12Z 52, 44896, 540514,*/
static constexpr auto BENCH_EXAMPLE_07 = "b317998a4000b40098d53e01b45b99814802b7289b940003b3699a9d1204b6619a807a05814682cb78050571d854060571d8540705800e808d7a0805803480c06a09056e8189280a056ffd060b05800d80ea7a0c05803c80b80c0c03803e80d86e0d036ed2280e03811581804a0f036fd34e1003805380eb6811036e81f60e12038010ec101204805f80e83a13048033809534140471e00a15048010f95816046e81fa301704805180a74c1705800d808f1018056fd55c1905800e8091481a056e80a76e1b05805f80e2741c0571809b021c05826382c8401d0571df201e05800e809d2c1f05850083e87c1f05811580af68200571f20a21056ff9042205803e80df1e23056e81956c24056e9f542604805180e83829000e800e8080621325803380b0402a020d6ef8100e2c8c4889a96a2c000f803580ce4c2c000b6e9f54062a803480c96406260500"_hex_u8;
/* 2023-12-05T23:48:44Z 69, 44283, 586734,*/
static constexpr auto BENCH_EXAMPLE_08 = "83728ce80000b90befca1001806083b24002b40de6da3203b545e9c35c04b34beede3005b068e8883006d41c80b1e14c07b337e7841208b26beadb2e096e83892e090980518487380a096e82815c0a096e81ce3c0b097181db200c097181d4020d09810084ed600e096e96b0100f0971819a0210086e93da2e0f09803583ee5e1009803583c66c1109800d82bb6e1209800d81d56a1309803c82e622140971819f521509803d84a55c15057181d6161605806283ac5217056e949c5a18056e89e8641806815889e23419067181de321a066e8af2641a076e82a70a1b07803583f2081c076f81e76e1d076e81d33e1e07800d83b8761e086e82a5541f087181de302008805f84ad0021086e81c74022086e81bd3e23086e9288182408806184b3102409803283816025096e91ed662609830a88e70827096e81d14a27097181ce6028096e8cf03829097181883832016f81835c3103806181e0103203804180b8103204863584fe183304800de66434046e9e4c34056e81d6742f429213c0eb602e3d6483b06c283a6e81d73c263d6e82f9581831805485ab360e37805080c62609398b3189880838010603916db1f3583a03000110873199f8623c000000011100"_hex_u8;
/* 2023-04-14T19:36:52Z 77, 20418, 501117,*/
static constexpr auto BENCH_EXAMPLE_09 = "bf2989d00400815bca5c01af1e86f97602800d9d6c03800d8a3404b47988866e05b36287f92e0680109f68078010991a08805ecf1208076e80933e09078062d01c0a078054b6760b078053b6760c076f9c1c0d078054b6760e0771af260f0771b17e10078032f57011078035d56812078054e1581307886b83dc301407817480d13013068005a6001406803d80821a15066ef3201606800ea2181706800da628180671ab1219068054db0c1a06719b001b06815b80a11c1c068050b9301d066fac2a1e068033ab481f06719b1020068035ab721e07803dc2761f0771ae3c20078040f60e210771ce282207800ea4322307882a81a66024078035ad4625076efe7e26078162808e1827078118bb7228076eac7428088010bf58290871a04c2a0871bc722b086fa8382c08803d80a0142d088035d6282e088051c30c2f086efc623008800d9f6231086f986432088117bb7237028010a63034068010c84e2740800ea64c2237832c80933e1f3b830880c454390208813c80955c3905068032c73611348010a03c093c837a808a101b278050ac34093a8051ac34291b8f3b8187401d28881a82cb3a3a0a37977b86d20843000028996686a7083f030f8078d3761b27106e995a08499070839b5a1131000b00"_hex_u8;
/* 2023-11-07T17:59:35Z 48, 4792, 498995,*/
static constexpr auto BENCH_EXAMPLE_10 = "875f89aa1000b51ec09d7201c55cc7a72e02a11aa1fb3203b233a7f95204800ef56205b33ea9d13006803e80b26e07d90ec9dd4008b45eabbe6c09806080ca000a815984e8680a0a6f80925e0a0a803f80e1660c09937c94b7420d086e82f5640a086e80997e0b086f808d320c08800580a5640d086f8089100e08804080c9060f088115819a1c10086e82961a0f0a805f81bc0a100a6ff826110a6ef53e120a807584c60c110a6e818f32120a803c81c246130a805481d508140a8159838410150a7180a55c160a6f80821c170a6fe6101c066fe6101d06805080f854190a6e81b27c1a0a8155819c701e06805180ae0c21046e8b9a222501805180f53422001680f26880f8a62a220116803580da582007058153838e6e21000c800d80a712033a807681ae1c23000308834a82d36023020205815981e03a051a08001700"_hex_u8;

static void LinearizeOptimallyExample00(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_00); }
static void LinearizeOptimallyExample01(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_01); }
static void LinearizeOptimallyExample02(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_02); }
static void LinearizeOptimallyExample03(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_03); }
static void LinearizeOptimallyExample04(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_04); }
static void LinearizeOptimallyExample05(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_05); }
static void LinearizeOptimallyExample06(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_06); }
static void LinearizeOptimallyExample07(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_07); }
static void LinearizeOptimallyExample08(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_08); }
static void LinearizeOptimallyExample09(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_09); }
static void LinearizeOptimallyExample10(benchmark::Bench& bench) { BenchLinearizeOptimally(bench, BENCH_EXAMPLE_10); }

BENCHMARK(Linearize16TxWorstCase20Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize16TxWorstCase120Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize32TxWorstCase5000Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize32TxWorstCase15000Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize48TxWorstCase5000Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize48TxWorstCase15000Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize64TxWorstCase5000Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize64TxWorstCase15000Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize75TxWorstCase5000Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize75TxWorstCase15000Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize99TxWorstCase5000Iters, benchmark::PriorityLevel::HIGH);
BENCHMARK(Linearize99TxWorstCase15000Iters, benchmark::PriorityLevel::HIGH);

BENCHMARK(LinearizeNoIters16TxWorstCaseAnc, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeNoIters32TxWorstCaseAnc, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeNoIters48TxWorstCaseAnc, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeNoIters64TxWorstCaseAnc, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeNoIters75TxWorstCaseAnc, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeNoIters99TxWorstCaseAnc, benchmark::PriorityLevel::HIGH);

BENCHMARK(LinearizeNoIters16TxWorstCaseLIMO, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeNoIters32TxWorstCaseLIMO, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeNoIters48TxWorstCaseLIMO, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeNoIters64TxWorstCaseLIMO, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeNoIters75TxWorstCaseLIMO, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeNoIters99TxWorstCaseLIMO, benchmark::PriorityLevel::HIGH);

BENCHMARK(PostLinearize16TxWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(PostLinearize32TxWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(PostLinearize48TxWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(PostLinearize64TxWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(PostLinearize75TxWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(PostLinearize99TxWorstCase, benchmark::PriorityLevel::HIGH);

BENCHMARK(MergeLinearizations16TxWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(MergeLinearizations32TxWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(MergeLinearizations48TxWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(MergeLinearizations64TxWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(MergeLinearizations75TxWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(MergeLinearizations99TxWorstCase, benchmark::PriorityLevel::HIGH);

BENCHMARK(LinearizeOptimallyExample00, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeOptimallyExample01, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeOptimallyExample02, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeOptimallyExample03, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeOptimallyExample04, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeOptimallyExample05, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeOptimallyExample06, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeOptimallyExample07, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeOptimallyExample08, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeOptimallyExample09, benchmark::PriorityLevel::HIGH);
BENCHMARK(LinearizeOptimallyExample10, benchmark::PriorityLevel::HIGH);
