// Copyright (c) 2017-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETUTIL_H
#define BITCOIN_WALLET_WALLETUTIL_H

#include <script/descriptor.h>
#include <util/fs.h>

#include <vector>

namespace wallet {

enum WalletFlags : uint64_t {
    // wallet flags in the upper section (> 1 << 31) will lead to not opening the wallet if flag is unknown
    // unknown wallet flags in the lower section <= (1 << 31) will be tolerated

    // will categorize coins as clean (not reused) and dirty (reused), and handle
    // them with privacy considerations in mind
    WALLET_FLAG_AVOID_REUSE = (1ULL << 0),

    // Indicates that the metadata has already been upgraded to contain key origins
    WALLET_FLAG_KEY_ORIGIN_METADATA = (1ULL << 1),

    // Indicates that the descriptor cache has been upgraded to cache last hardened xpubs
    WALLET_FLAG_LAST_HARDENED_XPUB_CACHED = (1ULL << 2),

    // will enforce the rule that the wallet can't contain any private keys (only watch-only/pubkeys)
    WALLET_FLAG_DISABLE_PRIVATE_KEYS = (1ULL << 32),

    //! Flag set when a wallet contains no HD seed and no private keys, scripts,
    //! addresses, and other watch only things, and is therefore "blank."
    //!
    //! The main function this flag serves is to distinguish a blank wallet from
    //! a newly created wallet when the wallet database is loaded, to avoid
    //! initialization that should only happen on first run.
    //!
    //! A secondary function of this flag, which applies to descriptor wallets
    //! only, is to serve as an ongoing indication that descriptors in the
    //! wallet should be created manually, and that the wallet should not
    //! generate automatically generate new descriptors if it is later
    //! encrypted. To support this behavior, descriptor wallets unlike legacy
    //! wallets do not automatically unset the BLANK flag when things are
    //! imported.
    //!
    //! This flag is also a mandatory flag to prevent previous versions of
    //! bitcoin from opening the wallet, thinking it was newly created, and
    //! then improperly reinitializing it.
    WALLET_FLAG_BLANK_WALLET = (1ULL << 33),

    //! Indicate that this wallet supports DescriptorScriptPubKeyMan
    WALLET_FLAG_DESCRIPTORS = (1ULL << 34),

    //! Indicates that the wallet needs an external signer
    WALLET_FLAG_EXTERNAL_SIGNER = (1ULL << 35),
};

// Version numbers for the wallet client that opens a wallet
// These numbers will be written as the last client version in the "version" record and can be used to detect
// when a upgrade-downgrade-upgrade was performed. However, we should prefer to use LastClientFeatures rather
// than new version numbers.
// New version numbers must be greater than 299900 which is guaranteed by setting bit 30
enum WalletClientVersion : int32_t {
    // OR with all versions to set bit 30
    VERSION_MASK = (1ULL << 30),

    VERSION_LATEST = VERSION_MASK
};

//! Get the path of the wallet directory.
fs::path GetWalletDir();

/** Descriptor with some wallet metadata */
class WalletDescriptor
{
public:
    std::shared_ptr<Descriptor> descriptor;
    uint256 id; // Descriptor ID (calculated once at descriptor initialization/deserialization)
    uint64_t creation_time = 0;
    int32_t range_start = 0; // First item in range; start of range, inclusive, i.e. [range_start, range_end). This never changes.
    int32_t range_end = 0; // Item after the last; end of range, exclusive, i.e. [range_start, range_end). This will increment with each TopUp()
    int32_t next_index = 0; // Position of the next item to generate
    DescriptorCache cache;

    void DeserializeDescriptor(const std::string& str)
    {
        std::string error;
        FlatSigningProvider keys;
        auto descs = Parse(str, keys, error, true);
        if (descs.empty()) {
            throw std::ios_base::failure("Invalid descriptor: " + error);
        }
        if (descs.size() > 1) {
            throw std::ios_base::failure("Can't load a multipath descriptor from databases");
        }
        descriptor = std::move(descs.at(0));
        id = DescriptorID(*descriptor);
    }

    SERIALIZE_METHODS(WalletDescriptor, obj)
    {
        std::string descriptor_str;
        SER_WRITE(obj, descriptor_str = obj.descriptor->ToString());
        READWRITE(descriptor_str, obj.creation_time, obj.next_index, obj.range_start, obj.range_end);
        SER_READ(obj, obj.DeserializeDescriptor(descriptor_str));
    }

    WalletDescriptor() = default;
    WalletDescriptor(std::shared_ptr<Descriptor> descriptor, uint64_t creation_time, int32_t range_start, int32_t range_end, int32_t next_index) : descriptor(descriptor), id(DescriptorID(*descriptor)), creation_time(creation_time), range_start(range_start), range_end(range_end), next_index(next_index) { }
};

WalletDescriptor GenerateWalletDescriptor(const CExtPubKey& master_key, const OutputType& output_type, bool internal);
} // namespace wallet

#endif // BITCOIN_WALLET_WALLETUTIL_H
