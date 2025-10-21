// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

/**
 * Example demonstrating Bitcoin script verification using the kernel C API.
 * 
 * This example shows how to:
 * - Parse a raw transaction from hex
 * - Create a script public key from hex
 * - Verify that a transaction input correctly spends a previous output
 * - Handle different script types (including segwit/taproot)
 * 
 * The example uses a real mainnet transaction for demonstration.
 */

#include <cstdio>
#include <cctype>
#include <string>
#include <string_view>
#include <stdexcept>
#include <vector>
#include "bitcoinkernel.h"

// Helper function to convert hex string to bytes
// Accepts spaces and newlines for readability
static std::vector<unsigned char> from_hex(std::string_view hex)
{
    auto nib = [](char c) -> int {
        if ('0' <= c && c <= '9') return c - '0';
        c = std::tolower(static_cast<unsigned char>(c));
        if ('a' <= c && c <= 'f') return 10 + (c - 'a');
        return -1;
    };

    std::vector<unsigned char> out;
    int hi = -1;
    for (char c : hex) {
        if (std::isspace(static_cast<unsigned char>(c))) continue;
        int v = nib(c);
        if (v < 0) throw std::runtime_error("non-hex character in input");
        if (hi < 0) {
            hi = v;
        } else {
            out.push_back(static_cast<unsigned char>((hi << 4) | v));
            hi = -1;
        }
    }
    if (hi >= 0) throw std::runtime_error("odd-length hex string");
    return out;
}

int main()
{
    // Example transaction spending a taproot output
    // Transaction: https://mempool.space/tx/ed90adb91f56ff0589750d4da1566739fb5edfe59aa333d478498f46919773bd
    const char* TX_HEX = 
        "01000000000101bd739791468f4978d433a39ae5df5efb396756a14d0d758905ff561fb9ad90ed"
        "00000000000000000001b9f00000000000001600141bd20f539be349c105360ec5727c3598a0ba"
        "700b0140fdaa04a6f821a612760a2915311c1156b52a54cafaa59fc4f0505188658c115b67d635"
        "ab8c6d8ba66045c2c6c023448652a49f828f6d5e6fa2901cf9f81adbfb00000000";

    // The previous output being spent (taproot output)
    // Script public key (P2TR): 512082012364b27893b092dfbd89bff41b17cff5ffb6f7b8d0d381cd93a0a3689f33
    const char* SPK_HEX = "512082012364b27893b092dfbd89bff41b17cff5ffb6f7b8d0d381cd93a0a3689f33";
    
    // Amount of the previous output in satoshis
    int64_t AMOUNT = 61922;

    // Index of the input we're verifying (0 for first input)
    unsigned input_index = 0;

    // Verification flags - use all standard flags
    unsigned flags = btck_ScriptVerificationFlags_ALL;

    std::puts("Bitcoin Script Verification Example");
    std::puts("=====================================");
    std::printf("Verifying input %u of transaction\n", input_index);
    std::printf("Previous output amount: %lld satoshis\n\n", (long long)AMOUNT);

    try {
        // Convert hex strings to byte vectors
        std::vector<unsigned char> tx_bytes = from_hex(TX_HEX);
        std::vector<unsigned char> spk_bytes = from_hex(SPK_HEX);

        std::printf("Transaction size: %zu bytes\n", tx_bytes.size());
        std::printf("Script pubkey size: %zu bytes\n", spk_bytes.size());
        
        // Detect script type
        if (spk_bytes.size() > 0) {
            if (spk_bytes[0] == 0x51 && spk_bytes.size() == 34) {
                std::puts("Script type: P2TR (Pay to Taproot)");
            } else if (spk_bytes[0] == 0x00 && spk_bytes.size() == 22) {
                std::puts("Script type: P2WPKH (Pay to Witness Public Key Hash)");
            } else if (spk_bytes[0] == 0x00 && spk_bytes.size() == 34) {
                std::puts("Script type: P2WSH (Pay to Witness Script Hash)");
            } else if (spk_bytes[0] == 0x76 && spk_bytes[1] == 0xa9) {
                std::puts("Script type: P2PKH (Pay to Public Key Hash)");
            } else if (spk_bytes[0] == 0xa9 && spk_bytes.size() == 23) {
                std::puts("Script type: P2SH (Pay to Script Hash)");
            } else {
                std::puts("Script type: Custom/Unknown");
            }
        }
        std::puts("");

        // Create transaction object from raw bytes
        btck_Transaction* tx = btck_transaction_create(tx_bytes.data(), tx_bytes.size());
        if (!tx) {
            std::fprintf(stderr, "Error: Failed to parse transaction\n");
            return 1;
        }

        // Create script public key object
        btck_ScriptPubkey* spk = btck_script_pubkey_create(spk_bytes.data(), spk_bytes.size());
        if (!spk) {
            std::fprintf(stderr, "Error: Failed to parse script public key\n");
            btck_transaction_destroy(tx);
            return 1;
        }

        // For segwit/taproot verification, we need to provide the previous output
        // being spent (script  amount)
        btck_TransactionOutput* out = btck_transaction_output_create(spk, AMOUNT);
        const btck_TransactionOutput* out_ptr = out;

        // Perform script verification
        btck_ScriptVerifyStatus status = btck_ScriptVerifyStatus_SCRIPT_VERIFY_OK;
        std::puts("Performing script verification...");
        
        int ok = btck_script_pubkey_verify(
            spk,                    // Script public key being verified
            AMOUNT,                 // Amount of the output being spent
            tx,                     // Transaction containing the input
            (flags & btck_ScriptVerificationFlags_TAPROOT) ? &out_ptr : nullptr,
            (flags & btck_ScriptVerificationFlags_TAPROOT) ? 1u : 0u,
            input_index,            // Index of the input to verify
            flags,                  // Verification flags
            &status                 // Output status
        );

        // Clean up resources
        btck_transaction_output_destroy(out);
        btck_script_pubkey_destroy(spk);
        btck_transaction_destroy(tx);

        // Check verification result
        std::printf("\nVerification result: %s\n", ok ? "SUCCESS" : "FAILURE");
        std::printf("Status code: %u\n", static_cast<unsigned>(status));

        if (ok && status == btck_ScriptVerifyStatus_SCRIPT_VERIFY_OK) {
            std::puts("\n✅ Script verification passed! The transaction input is valid.");
            return 0;
        } else {
            std::puts("\n❌ Script verification failed! The transaction input is invalid.");
            return 2;
        }

    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }

    return 0;
}