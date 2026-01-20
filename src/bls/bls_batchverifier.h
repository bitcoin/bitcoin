// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_CRYPTO_BLS_BATCHVERIFIER_H
#define DASH_CRYPTO_BLS_BATCHVERIFIER_H

#include <bls/bls.h>

#include <map>
#include <vector>

template<typename SourceId, typename MessageId>
class CBLSBatchVerifier
{
private:
    struct Message {
        MessageId msgId;
        uint256 msgHash;
        CBLSSignature sig;
        CBLSPublicKey pubKey;
    };

    using MessageMap = std::map<MessageId, Message>;
    using MessageMapIterator = typename MessageMap::iterator;
    using MessagesBySourceMap = std::map<SourceId, std::vector<MessageMapIterator>>;

    bool secureVerification;
    bool perMessageFallback;
    size_t subBatchSize;

    MessageMap messages;
    MessagesBySourceMap messagesBySource;

public:
    std::set<SourceId> badSources;
    std::set<MessageId> badMessages;

public:
    CBLSBatchVerifier(bool _secureVerification, bool _perMessageFallback, size_t _subBatchSize = 0) :
            secureVerification(_secureVerification),
            perMessageFallback(_perMessageFallback),
            subBatchSize(_subBatchSize)
    {
    }

    void PushMessage(const SourceId& sourceId, const MessageId& msgId, const uint256& msgHash, const CBLSSignature& sig, const CBLSPublicKey& pubKey)
    {
        assert(sig.IsValid() && pubKey.IsValid());

        auto it = messages.emplace(msgId, Message{msgId, msgHash, sig, pubKey}).first;
        messagesBySource[sourceId].emplace_back(it);

        if (subBatchSize != 0 && messages.size() >= subBatchSize) {
            Verify();
            ClearMessages();
        }
    }

    void ClearMessages()
    {
        messages.clear();
        messagesBySource.clear();
    }

    size_t GetUniqueSourceCount() const
    {
        return messagesBySource.size();
    }

    void Verify()
    {
        std::map<uint256, std::vector<MessageMapIterator>> byMessageHash;

        for (auto it = messages.begin(); it != messages.end(); ++it) {
            byMessageHash[it->second.msgHash].emplace_back(it);
        }

        if (VerifyBatch(byMessageHash)) {
            // full batch is valid
            return;
        }

        // revert to per-source verification
        for (const auto& [from, message_map] : messagesBySource) {
            bool batchValid = false;

            // no need to verify it again if there was just one source
            if (messagesBySource.size() != 1) {
                byMessageHash.clear();
                for (auto it = message_map.begin(); it != message_map.end(); ++it) {
                    byMessageHash[(*it)->second.msgHash].emplace_back(*it);
                }
                batchValid = VerifyBatch(byMessageHash);
            }
            if (!batchValid) {
                badSources.emplace(from);

                if (perMessageFallback) {
                    // revert to per-message verification
                    if (message_map.size() == 1) {
                        // no need to re-verify a single message
                        badMessages.emplace(message_map[0]->second.msgId);
                    } else {
                        for (const auto& msgIt : message_map) {
                            if (badMessages.count(msgIt->first)) {
                                // same message might be invalid from different source, so no need to re-verify it
                                continue;
                            }

                            const auto& msg = msgIt->second;
                            if (!msg.sig.VerifyInsecure(msg.pubKey, msg.msgHash)) {
                                badMessages.emplace(msg.msgId);
                            }
                        }
                    }
                }
            }
        }
    }

private:
    // All Verify methods take ownership of the passed byMessageHash map and thus might modify the map. This is to avoid
    // unnecessary copies

    bool VerifyBatch(std::map<uint256, std::vector<MessageMapIterator>>& byMessageHash)
    {
        if (secureVerification) {
            return VerifyBatchSecure(byMessageHash);
        } else {
            return VerifyBatchInsecure(byMessageHash);
        }
    }

    bool VerifyBatchInsecure(const std::map<uint256, std::vector<MessageMapIterator>>& byMessageHash)
    {
        std::vector<CBLSSignature> sigsToAggregate;
        std::vector<uint256> msgHashes;
        std::vector<CBLSPublicKey> pubKeys;
        std::set<MessageId> dups;

        msgHashes.reserve(messages.size());
        pubKeys.reserve(messages.size());
        sigsToAggregate.reserve(messages.size());

        std::vector<CBLSPublicKey> pubKeysToAggregate;
        for (const auto& [msgHash, vec_message_it] : byMessageHash) {
            pubKeysToAggregate.clear();
            pubKeysToAggregate.reserve(vec_message_it.size());

            for (const auto& msgIt : vec_message_it) {
                const auto& msg = msgIt->second;

                if (!dups.emplace(msg.msgId).second) {
                    continue;
                }

                sigsToAggregate.push_back(msg.sig);
                pubKeysToAggregate.push_back(msg.pubKey);
            }

            CBLSPublicKey aggPubKey = CBLSPublicKey::AggregateInsecure(pubKeysToAggregate);

            if (!aggPubKey.IsValid()) {
                // only duplicates for this msgHash
                continue;
            }

            msgHashes.emplace_back(msgHash);
            pubKeys.emplace_back(aggPubKey);
        }

        if (msgHashes.empty()) {
            return true;
        }

        CBLSSignature aggSig = CBLSSignature::AggregateInsecure(sigsToAggregate);
        return aggSig.VerifyInsecureAggregated(pubKeys, msgHashes);
    }

    bool VerifyBatchSecure(std::map<uint256, std::vector<MessageMapIterator>>& byMessageHash)
    {
        // Loop until the byMessageHash map is empty, which means that all messages were verified
        // The secure form of verification will only aggregate one message for the same message hash, even if multiple
        // exist (signed with different keys). This avoids the rogue public key attack.
        // This is slower than the insecure form as it requires more pairings
        while (!byMessageHash.empty()) {
            if (!VerifyBatchSecureStep(byMessageHash)) {
                return false;
            }
        }
        return true;
    }

    bool VerifyBatchSecureStep(std::map<uint256, std::vector<MessageMapIterator>>& byMessageHash)
    {
        std::vector<CBLSSignature> sigsToAggregate;
        std::vector<uint256> msgHashes;
        std::vector<CBLSPublicKey> pubKeys;
        std::set<MessageId> dups;

        msgHashes.reserve(messages.size());
        pubKeys.reserve(messages.size());
        sigsToAggregate.reserve(messages.size());

        for (auto it = byMessageHash.begin(); it != byMessageHash.end(); ) {
            const auto& msgHash = it->first;
            auto& messageIts = it->second;
            const auto& msg = messageIts.back()->second;

            if (dups.emplace(msg.msgId).second) {
                msgHashes.emplace_back(msgHash);
                pubKeys.emplace_back(msg.pubKey);
                sigsToAggregate.push_back(msg.sig);
            }

            messageIts.pop_back();
            if (messageIts.empty()) {
                it = byMessageHash.erase(it);
            } else {
                ++it;
            }
        }

        assert(!msgHashes.empty());

        CBLSSignature aggSig = CBLSSignature::AggregateInsecure(sigsToAggregate);
        return aggSig.VerifyInsecureAggregated(pubKeys, msgHashes);
    }
};

#endif //DASH_CRYPTO_BLS_BATCHVERIFIER_H
