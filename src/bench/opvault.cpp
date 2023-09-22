// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <script/script.h>
#include <script/interpreter.h>
#include <util/strencodings.h>
#include <vector>


using valtype = std::vector<unsigned char>;


/**
 * This bench exists to help determine how OP_VAULT invocations should be
 * costed in the script interpreter.
 *
 * The benchmark compares the operation essential to OP_VAULT, which is computing
 * the expected Taproot for a trigger output, to the cost of a valid Schnorr
 * signature verification.
 */
static void OPVaultTweakCostingTest(benchmark::Bench& bench)
{
    // Expected "average" controlblock size for OP_VAULT usage.
    const valtype small_control(65);

    // Pathological max-length controlblock.
    const valtype control(TAPROOT_CONTROL_MAX_SIZE);

    const valtype key(ParseHex(
        "F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9"));
    const XOnlyPubKey internal_key{key};
    const valtype leaf_script(128);  // expected OP_VAULT leaf-update script, doubled

    bench.warmup(100).relative(true);

    bench.unit("verify").run(strprintf("opvault-tweak-ctrl-%d", TAPROOT_CONTROL_MAX_SIZE), [&] {
        const auto tapleaf_hash = ComputeTapleafHash(
            control[0] & TAPROOT_LEAF_MASK, leaf_script);
        const uint256 merkle_root = ComputeTaprootMerkleRoot(control, tapleaf_hash);
        auto ret = internal_key.CreateTapTweak(&merkle_root);
        assert(ret);
    });

    bench.unit("verify").run(strprintf("opvault-tweak-ctrl-%d", small_control.size()), [&] {
        const auto tapleaf_hash = ComputeTapleafHash(
            control[0] & TAPROOT_LEAF_MASK, leaf_script);
        const uint256 merkle_root = ComputeTaprootMerkleRoot(small_control, tapleaf_hash);
        auto ret = internal_key.CreateTapTweak(&merkle_root);
        assert(ret);
    });

    const XOnlyPubKey schnorr_key1{ParseHex(
        "EEFDEA4CDB677750A420FEE807EACF21EB9898AE79B9768766E4FAA04A2D4A34")};
    const valtype msg1{ParseHex(
        "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89")};
    const valtype bad_sig{ParseHex(
        "6CFF5C3BA86C69EA4B7376F31A9BCB4F74C1976089B2D9963DA2E5543E17776969E89B4C5564D00349106B8497785DD7D1D713A8AE82B32FA79D5F7FC407D39B")};

    bench.unit("verify").run("schnorr-bad-verify", [&] {
        auto ret = schnorr_key1.VerifySchnorr(uint256(msg1), bad_sig);
        assert(!ret);
    });

    const XOnlyPubKey schnorr_key2{ParseHex(
        "F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9")};
    const valtype msg2{ParseHex(
        "0000000000000000000000000000000000000000000000000000000000000000")};
    const valtype good_sig{ParseHex(
        "E907831F80848D1069A5371B402410364BDF1C5F8307B0084C55F1CE2DCA821525F66A4A85EA8B71E482A74F382D2CE5EBEEE8FDB2172F477DF4900D310536C0")};

    bench.unit("verify").run("schnorr-good-verify", [&] {
        auto ret = schnorr_key2.VerifySchnorr(uint256(msg2), good_sig);
        assert(ret);
    });
}


BENCHMARK(OPVaultTweakCostingTest, benchmark::PriorityLevel::HIGH);
