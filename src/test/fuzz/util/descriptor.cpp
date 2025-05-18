// Copyright (c) 2023-present The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/util/descriptor.h>

#include <ranges>
#include <stack>

void MockedDescriptorConverter::Init() {
    // The data to use as a private key or a seed for an xprv.
    std::array<std::byte, 32> key_data{std::byte{1}};
    // Generate keys of all kinds and store them in the keys array.
    for (size_t i{0}; i < TOTAL_KEYS_GENERATED; i++) {
        key_data[31] = std::byte(i);

        // If this is a "raw" key, generate a normal privkey. Otherwise generate
        // an extended one.
        if (IdIsCompPubKey(i) || IdIsUnCompPubKey(i) || IdIsXOnlyPubKey(i) || IdIsConstPrivKey(i)) {
            CKey privkey;
            privkey.Set(key_data.begin(), key_data.end(), !IdIsUnCompPubKey(i));
            if (IdIsCompPubKey(i) || IdIsUnCompPubKey(i)) {
                CPubKey pubkey{privkey.GetPubKey()};
                keys_str[i] = HexStr(pubkey);
            } else if (IdIsXOnlyPubKey(i)) {
                const XOnlyPubKey pubkey{privkey.GetPubKey()};
                keys_str[i] = HexStr(pubkey);
            } else {
                keys_str[i] = EncodeSecret(privkey);
            }
        } else {
            CExtKey ext_privkey;
            ext_privkey.SetSeed(key_data);
            if (IdIsXprv(i)) {
                keys_str[i] = EncodeExtKey(ext_privkey);
            } else {
                const CExtPubKey ext_pubkey{ext_privkey.Neuter()};
                keys_str[i] = EncodeExtPubKey(ext_pubkey);
            }
        }
    }
}

std::optional<uint8_t> MockedDescriptorConverter::IdxFromHex(std::string_view hex_characters) const {
    if (hex_characters.size() != 2) return {};
    auto idx = ParseHex(hex_characters);
    if (idx.size() != 1) return {};
    return idx[0];
}

std::optional<std::string> MockedDescriptorConverter::GetDescriptor(std::string_view mocked_desc) const {
    // The smallest fragment would be "pk(%00)"
    if (mocked_desc.size() < 7) return {};

    // The actual descriptor string to be returned.
    std::string desc;
    desc.reserve(mocked_desc.size());

    // Replace all occurrences of '%' followed by two hex characters with the corresponding key.
    for (size_t i = 0; i < mocked_desc.size();) {
        if (mocked_desc[i] == '%') {
            if (i + 3 >= mocked_desc.size()) return {};
            if (const auto idx = IdxFromHex(mocked_desc.substr(i + 1, 2))) {
                desc += keys_str[*idx];
                i += 3;
            } else {
                return {};
            }
        } else {
            desc += mocked_desc[i++];
        }
    }

    return desc;
}

bool HasDeepDerivPath(const FuzzBufferType& buff, const int max_depth)
{
    auto depth{0};
    for (const auto& ch: buff) {
        if (ch == ',') {
            // A comma is always present between two key expressions, so we use that as a delimiter.
            depth = 0;
        } else if (ch == '/') {
            if (++depth > max_depth) return true;
        }
    }
    return false;
}

bool HasTooManySubFrag(const FuzzBufferType& buff, const int max_subs, const size_t max_nested_subs)
{
    // We use a stack because there may be many nested sub-frags.
    std::stack<int> counts;
    for (const auto& ch: buff) {
        // The fuzzer may generate an input with a ton of parentheses. Rule out pathological cases.
        if (counts.size() > max_nested_subs) return true;

        if (ch == '(') {
            // A new fragment was opened, create a new sub-count for it and start as one since any fragment with
            // parentheses has at least one sub.
            counts.push(1);
        } else if (ch == ',' && !counts.empty()) {
            // When encountering a comma, account for an additional sub in the last opened fragment. If it exceeds the
            // limit, bail.
            if (++counts.top() > max_subs) return true;
        } else if (ch == ')' && !counts.empty()) {
            // Fragment closed! Drop its sub count and resume to counting the number of subs for its parent.
            counts.pop();
        }
    }
    return false;
}

bool HasTooManyWrappers(const FuzzBufferType& buff, const int max_wrappers)
{
    // The number of nested wrappers. Nested wrappers are always characters which follow each other so we don't have to
    // use a stack as we do above when counting the number of sub-fragments.
    std::optional<int> count;

    // We want to detect nested wrappers. A wrapper is a character prepended to a fragment, separated by a colon. There
    // may be more than one wrapper, in which case the colon is not repeated. For instance `jjjjj:pk()`.  To count
    // wrappers we iterate in reverse and use the colon to detect the end of a wrapper expression and count how many
    // characters there are since the beginning of the expression. We stop counting when we encounter a character
    // indicating the beginning of a new expression.
    for (const auto ch: buff | std::views::reverse) {
        // A colon, start counting.
        if (ch == ':') {
            // The colon itself is not a wrapper so we start at 0.
            count = 0;
        } else if (count) {
            // If we are counting wrappers, stop when we crossed the beginning of the wrapper expression. Otherwise keep
            // counting and bail if we reached the limit.
            // A wrapper may only ever occur as the first sub of a descriptor/miniscript expression ('('), as the
            // first Taproot leaf in a pair ('{') or as the nth sub in each case (',').
            if (ch == ',' || ch == '(' || ch == '{') {
                count.reset();
            } else if (++*count > max_wrappers) {
                return true;
            }
        }
    }

    return false;
}
