// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_CRYPTO_BLS_BATCHVERIFIER_H
#define DASH_CRYPTO_BLS_BATCHVERIFIER_H

#include "bls.h"

#include <map>
#include <vector>

template<typename SourceId, typename MessageId>
class CBLSInsecureBatchVerifier
{
private:
    struct Message {
        MessageId msgId;
        uint256 msgHash;
        CBLSSignature sig;
        CBLSPublicKey pubKey;
    };

    typedef std::map<MessageId, Message> MessageMap;
    typedef typename MessageMap::iterator MessageMapIterator;
    typedef std::map<SourceId, std::vector<MessageMapIterator>> MessagesBySourceMap;

    MessageMap messages;
    MessagesBySourceMap messagesBySource;

public:
    std::set<SourceId> badSources;
    std::set<MessageId> badMessages;

public:
    void PushMessage(const SourceId& sourceId, const MessageId& msgId, const uint256& msgHash, const CBLSSignature& sig, const CBLSPublicKey& pubKey)
    {
        assert(sig.IsValid() && pubKey.IsValid());

        auto it = messages.emplace(msgId, Message{msgId, msgHash, sig, pubKey}).first;
        messagesBySource[sourceId].emplace_back(it);
    }

    void Verify(bool perMessageFallback)
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
        for (const auto& p : messagesBySource) {
            bool batchValid = false;

            // no need to verify it again if there was just one source
            if (messagesBySource.size() != 1) {
                byMessageHash.clear();
                for (auto it = p.second.begin(); it != p.second.end(); ++it) {
                    byMessageHash[(*it)->second.msgHash].emplace_back(*it);
                }
                batchValid = VerifyBatch(byMessageHash);
            }
            if (!batchValid) {
                badSources.emplace(p.first);

                if (perMessageFallback) {
                    // revert to per-message verification
                    if (p.second.size() == 1) {
                        // no need to re-verify a single message
                        badMessages.emplace(p.second[0]->second.msgId);
                    } else {
                        for (const auto& msgIt : p.second) {
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
    bool VerifyBatch(const std::map<uint256, std::vector<MessageMapIterator>>& byMessageHash)
    {
        CBLSSignature aggSig;
        std::vector<uint256> msgHashes;
        std::vector<CBLSPublicKey> pubKeys;
        std::set<MessageId> dups;

        msgHashes.reserve(messages.size());
        pubKeys.reserve(messages.size());

        for (const auto& p : byMessageHash) {
            const auto& msgHash = p.first;

            CBLSPublicKey aggPubKey;

            for (const auto& msgIt : p.second) {
                const auto& msg = msgIt->second;

                if (!dups.emplace(msg.msgId).second) {
                    continue;
                }

                if (!aggSig.IsValid()) {
                    aggSig = msg.sig;
                } else {
                    aggSig.AggregateInsecure(msg.sig);
                }

                if (!aggPubKey.IsValid()) {
                    aggPubKey = msg.pubKey;
                } else {
                    aggPubKey.AggregateInsecure(msg.pubKey);
                }
            }

            if (!aggSig.IsValid()) {
                // only duplicates for this msgHash
                continue;
            }

            msgHashes.emplace_back(msgHash);
            pubKeys.emplace_back(aggPubKey);
        }

        if (msgHashes.empty()) {
            return true;
        }

        return aggSig.VerifyInsecureAggregated(pubKeys, msgHashes);
    }
};

#endif //DASH_CRYPTO_BLS_BATCHVERIFIER_H
