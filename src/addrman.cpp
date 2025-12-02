// Copyright (c) 2012 Pieter Wuille
// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <addrman.h>
#include <addrman_impl.h>

#include <hash.h>
#include <logging.h>
#include <logging/timer.h>
#include <netaddress.h>
#include <protocol.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/check.h>
#include <util/time.h>

#include <cmath>
#include <optional>

/** Over how many buckets entries with tried addresses from a single group (/16 for IPv4) are spread */
static constexpr uint32_t ADDRMAN_TRIED_BUCKETS_PER_GROUP{8};
/** Over how many buckets entries with new addresses originating from a single group are spread */
static constexpr uint32_t ADDRMAN_NEW_BUCKETS_PER_SOURCE_GROUP{64};
/** Maximum number of times an address can occur in the new table */
static constexpr int32_t ADDRMAN_NEW_BUCKETS_PER_ADDRESS{8};
/** How old addresses can maximally be */
static constexpr auto ADDRMAN_HORIZON{30 * 24h};
/** After how many failed attempts we give up on a new node */
static constexpr int32_t ADDRMAN_RETRIES{3};
/** How many successive failures are allowed ... */
static constexpr int32_t ADDRMAN_MAX_FAILURES{10};
/** ... in at least this duration */
static constexpr auto ADDRMAN_MIN_FAIL{7 * 24h};
/** How recent a successful connection should be before we allow an address to be evicted from tried */
static constexpr auto ADDRMAN_REPLACEMENT{4h};
/** The maximum number of tried addr collisions to store */
static constexpr size_t ADDRMAN_SET_TRIED_COLLISION_SIZE{10};
/** The maximum time we'll spend trying to resolve a tried table collision */
static constexpr auto ADDRMAN_TEST_WINDOW{40min};

int AddrInfo::GetTriedBucket(const uint256& nKey, const NetGroupManager& netgroupman) const
{
    uint64_t hash1 = (HashWriter{} << nKey << GetKey()).GetCheapHash();
    uint64_t hash2 = (HashWriter{} << nKey << netgroupman.GetGroup(*this) << (hash1 % ADDRMAN_TRIED_BUCKETS_PER_GROUP)).GetCheapHash();
    return hash2 % ADDRMAN_TRIED_BUCKET_COUNT;
}

int AddrInfo::GetNewBucket(const uint256& nKey, const CNetAddr& src, const NetGroupManager& netgroupman) const
{
    std::vector<unsigned char> vchSourceGroupKey = netgroupman.GetGroup(src);
    uint64_t hash1 = (HashWriter{} << nKey << netgroupman.GetGroup(*this) << vchSourceGroupKey).GetCheapHash();
    uint64_t hash2 = (HashWriter{} << nKey << vchSourceGroupKey << (hash1 % ADDRMAN_NEW_BUCKETS_PER_SOURCE_GROUP)).GetCheapHash();
    return hash2 % ADDRMAN_NEW_BUCKET_COUNT;
}

int AddrInfo::GetBucketPosition(const uint256& nKey, bool fNew, int bucket) const
{
    uint64_t hash1 = (HashWriter{} << nKey << (fNew ? uint8_t{'N'} : uint8_t{'K'}) << bucket << GetKey()).GetCheapHash();
    return hash1 % ADDRMAN_BUCKET_SIZE;
}

bool AddrInfo::IsTerrible(NodeSeconds now) const
{
    if (now - m_last_try <= 1min) { // never remove things tried in the last minute
        return false;
    }

    if (nTime > now + 10min) { // came in a flying DeLorean
        return true;
    }

    if (now - nTime > ADDRMAN_HORIZON) { // not seen in recent history
        return true;
    }

    if (TicksSinceEpoch<std::chrono::seconds>(m_last_success) == 0 && nAttempts >= ADDRMAN_RETRIES) { // tried N times and never a success
        return true;
    }

    if (now - m_last_success > ADDRMAN_MIN_FAIL && nAttempts >= ADDRMAN_MAX_FAILURES) { // N successive failures in the last week
        return true;
    }

    return false;
}

double AddrInfo::GetChance(NodeSeconds now) const
{
    double fChance = 1.0;

    // deprioritize very recent attempts away
    if (now - m_last_try < 10min) {
        fChance *= 0.01;
    }

    // deprioritize 66% after each failed attempt, but at most 1/28th to avoid the search taking forever or overly penalizing outages.
    fChance *= pow(0.66, std::min(nAttempts, 8));

    return fChance;
}

AddrManImpl::AddrManImpl(const NetGroupManager& netgroupman, bool deterministic, int32_t consistency_check_ratio)
    : insecure_rand{deterministic}
    , nKey{deterministic ? uint256{1} : insecure_rand.rand256()}
    , m_consistency_check_ratio{consistency_check_ratio}
    , m_netgroupman{netgroupman}
{
    for (auto& bucket : vvNew) {
        for (auto& entry : bucket) {
            entry = -1;
        }
    }
    for (auto& bucket : vvTried) {
        for (auto& entry : bucket) {
            entry = -1;
        }
    }
}

AddrManImpl::~AddrManImpl()
{
    nKey.SetNull();
}

template <typename Stream>
void AddrManImpl::Serialize(Stream& s_) const
{
    LOCK(cs);

    /**
     * Serialized format.
     * * format version byte (@see `Format`)
     * * lowest compatible format version byte. This is used to help old software decide
     *   whether to parse the file. For example:
     *   * Bitcoin Core version N knows how to parse up to format=3. If a new format=4 is
     *     introduced in version N+1 that is compatible with format=3 and it is known that
     *     version N will be able to parse it, then version N+1 will write
     *     (format=4, lowest_compatible=3) in the first two bytes of the file, and so
     *     version N will still try to parse it.
     *   * Bitcoin Core version N+2 introduces a new incompatible format=5. It will write
     *     (format=5, lowest_compatible=5) and so any versions that do not know how to parse
     *     format=5 will not try to read the file.
     * * nKey
     * * nNew
     * * nTried
     * * number of "new" buckets XOR 2**30
     * * all new addresses (total count: nNew)
     * * all tried addresses (total count: nTried)
     * * for each new bucket:
     *   * number of elements
     *   * for each element: index in the serialized "all new addresses"
     * * asmap checksum
     *
     * 2**30 is xorred with the number of buckets to make addrman deserializer v0 detect it
     * as incompatible. This is necessary because it did not check the version number on
     * deserialization.
     *
     * vvNew, vvTried, mapInfo, mapAddr and vRandom are never encoded explicitly;
     * they are instead reconstructed from the other information.
     *
     * This format is more complex, but significantly smaller (at most 1.5 MiB), and supports
     * changes to the ADDRMAN_ parameters without breaking the on-disk structure.
     *
     * We don't use SERIALIZE_METHODS since the serialization and deserialization code has
     * very little in common.
     */

    // Always serialize in the latest version (FILE_FORMAT).
    ParamsStream s{s_, CAddress::V2_DISK};

    s << static_cast<uint8_t>(FILE_FORMAT);

    // Increment `lowest_compatible` iff a newly introduced format is incompatible with
    // the previous one.
    static constexpr uint8_t lowest_compatible = Format::V4_MULTIPORT;
    s << static_cast<uint8_t>(INCOMPATIBILITY_BASE + lowest_compatible);

    s << nKey;
    s << nNew;
    s << nTried;

    int nUBuckets = ADDRMAN_NEW_BUCKET_COUNT ^ (1 << 30);
    s << nUBuckets;
    std::unordered_map<nid_type, int> mapUnkIds;
    int nIds = 0;
    for (const auto& entry : mapInfo) {
        mapUnkIds[entry.first] = nIds;
        const AddrInfo& info = entry.second;
        if (info.nRefCount) {
            assert(nIds != nNew); // this means nNew was wrong, oh ow
            s << info;
            nIds++;
        }
    }
    nIds = 0;
    for (const auto& entry : mapInfo) {
        const AddrInfo& info = entry.second;
        if (info.fInTried) {
            assert(nIds != nTried); // this means nTried was wrong, oh ow
            s << info;
            nIds++;
        }
    }
    for (int bucket = 0; bucket < ADDRMAN_NEW_BUCKET_COUNT; bucket++) {
        int nSize = 0;
        for (int i = 0; i < ADDRMAN_BUCKET_SIZE; i++) {
            if (vvNew[bucket][i] != -1)
                nSize++;
        }
        s << nSize;
        for (int i = 0; i < ADDRMAN_BUCKET_SIZE; i++) {
            if (vvNew[bucket][i] != -1) {
                int nIndex = mapUnkIds[vvNew[bucket][i]];
                s << nIndex;
            }
        }
    }
    // Store asmap checksum after bucket entries so that it
    // can be ignored by older clients for backward compatibility.
    s << m_netgroupman.GetAsmapChecksum();
}

template <typename Stream>
void AddrManImpl::Unserialize(Stream& s_)
{
    LOCK(cs);

    assert(vRandom.empty());

    Format format;
    s_ >> Using<CustomUintFormatter<1>>(format);

    const auto ser_params = (format >= Format::V3_BIP155 ? CAddress::V2_DISK : CAddress::V1_DISK);
    ParamsStream s{s_, ser_params};

    uint8_t compat;
    s >> compat;
    if (compat < INCOMPATIBILITY_BASE) {
        throw std::ios_base::failure(strprintf(
            "Corrupted addrman database: The compat value (%u) "
            "is lower than the expected minimum value %u.",
            compat, INCOMPATIBILITY_BASE));
    }
    const uint8_t lowest_compatible = compat - INCOMPATIBILITY_BASE;
    if (lowest_compatible > FILE_FORMAT) {
        throw InvalidAddrManVersionError(strprintf(
            "Unsupported format of addrman database: %u. It is compatible with formats >=%u, "
            "but the maximum supported by this version of %s is %u.",
            uint8_t{format}, lowest_compatible, CLIENT_NAME, uint8_t{FILE_FORMAT}));
    }

    s >> nKey;
    s >> nNew;
    s >> nTried;
    int nUBuckets = 0;
    s >> nUBuckets;
    if (format >= Format::V1_DETERMINISTIC) {
        nUBuckets ^= (1 << 30);
    }

    if (nNew > ADDRMAN_NEW_BUCKET_COUNT * ADDRMAN_BUCKET_SIZE || nNew < 0) {
        throw std::ios_base::failure(
                strprintf("Corrupt AddrMan serialization: nNew=%d, should be in [0, %d]",
                    nNew,
                    ADDRMAN_NEW_BUCKET_COUNT * ADDRMAN_BUCKET_SIZE));
    }

    if (nTried > ADDRMAN_TRIED_BUCKET_COUNT * ADDRMAN_BUCKET_SIZE || nTried < 0) {
        throw std::ios_base::failure(
                strprintf("Corrupt AddrMan serialization: nTried=%d, should be in [0, %d]",
                    nTried,
                    ADDRMAN_TRIED_BUCKET_COUNT * ADDRMAN_BUCKET_SIZE));
    }

    // Deserialize entries from the new table.
    for (int n = 0; n < nNew; n++) {
        AddrInfo& info = mapInfo[n];
        s >> info;
        mapAddr[info] = n;
        info.nRandomPos = vRandom.size();
        vRandom.push_back(n);
        m_network_counts[info.GetNetwork()].n_new++;
    }
    nIdCount = nNew;

    // Deserialize entries from the tried table.
    int nLost = 0;
    for (int n = 0; n < nTried; n++) {
        AddrInfo info;
        s >> info;
        int nKBucket = info.GetTriedBucket(nKey, m_netgroupman);
        int nKBucketPos = info.GetBucketPosition(nKey, false, nKBucket);
        if (info.IsValid()
                && vvTried[nKBucket][nKBucketPos] == -1) {
            info.nRandomPos = vRandom.size();
            info.fInTried = true;
            vRandom.push_back(nIdCount);
            mapInfo[nIdCount] = info;
            mapAddr[info] = nIdCount;
            vvTried[nKBucket][nKBucketPos] = nIdCount;
            nIdCount++;
            m_network_counts[info.GetNetwork()].n_tried++;
        } else {
            nLost++;
        }
    }
    nTried -= nLost;

    // Store positions in the new table buckets to apply later (if possible).
    // An entry may appear in up to ADDRMAN_NEW_BUCKETS_PER_ADDRESS buckets,
    // so we store all bucket-entry_index pairs to iterate through later.
    std::vector<std::pair<int, int>> bucket_entries;

    for (int bucket = 0; bucket < nUBuckets; ++bucket) {
        int num_entries{0};
        s >> num_entries;
        for (int n = 0; n < num_entries; ++n) {
            int entry_index{0};
            s >> entry_index;
            if (entry_index >= 0 && entry_index < nNew) {
                bucket_entries.emplace_back(bucket, entry_index);
            }
        }
    }

    // If the bucket count and asmap checksum haven't changed, then attempt
    // to restore the entries to the buckets/positions they were in before
    // serialization.
    uint256 supplied_asmap_checksum{m_netgroupman.GetAsmapChecksum()};
    uint256 serialized_asmap_checksum;
    if (format >= Format::V2_ASMAP) {
        s >> serialized_asmap_checksum;
    }
    const bool restore_bucketing{nUBuckets == ADDRMAN_NEW_BUCKET_COUNT &&
        serialized_asmap_checksum == supplied_asmap_checksum};

    if (!restore_bucketing) {
        LogDebug(BCLog::ADDRMAN, "Bucketing method was updated, re-bucketing addrman entries from disk\n");
    }

    for (auto bucket_entry : bucket_entries) {
        int bucket{bucket_entry.first};
        const int entry_index{bucket_entry.second};
        AddrInfo& info = mapInfo[entry_index];

        // Don't store the entry in the new bucket if it's not a valid address for our addrman
        if (!info.IsValid()) continue;

        // The entry shouldn't appear in more than
        // ADDRMAN_NEW_BUCKETS_PER_ADDRESS. If it has already, just skip
        // this bucket_entry.
        if (info.nRefCount >= ADDRMAN_NEW_BUCKETS_PER_ADDRESS) continue;

        int bucket_position = info.GetBucketPosition(nKey, true, bucket);
        if (restore_bucketing && vvNew[bucket][bucket_position] == -1) {
            // Bucketing has not changed, using existing bucket positions for the new table
            vvNew[bucket][bucket_position] = entry_index;
            ++info.nRefCount;
        } else {
            // In case the new table data cannot be used (bucket count wrong or new asmap),
            // try to give them a reference based on their primary source address.
            bucket = info.GetNewBucket(nKey, m_netgroupman);
            bucket_position = info.GetBucketPosition(nKey, true, bucket);
            if (vvNew[bucket][bucket_position] == -1) {
                vvNew[bucket][bucket_position] = entry_index;
                ++info.nRefCount;
            }
        }
    }

    // Prune new entries with refcount 0 (as a result of collisions or invalid address).
    int nLostUnk = 0;
    for (auto it = mapInfo.cbegin(); it != mapInfo.cend(); ) {
        if (it->second.fInTried == false && it->second.nRefCount == 0) {
            const auto itCopy = it++;
            Delete(itCopy->first);
            ++nLostUnk;
        } else {
            ++it;
        }
    }
    if (nLost + nLostUnk > 0) {
        LogDebug(BCLog::ADDRMAN, "addrman lost %i new and %i tried addresses due to collisions or invalid addresses\n", nLostUnk, nLost);
    }

    const int check_code{CheckAddrman()};
    if (check_code != 0) {
        throw std::ios_base::failure(strprintf(
            "Corrupt data. Consistency check failed with code %s",
            check_code));
    }
}

AddrInfo* AddrManImpl::Find(const CService& addr, nid_type* pnId)
{
    AssertLockHeld(cs);

    const auto it = mapAddr.find(addr);
    if (it == mapAddr.end())
        return nullptr;
    if (pnId)
        *pnId = (*it).second;
    const auto it2 = mapInfo.find((*it).second);
    if (it2 != mapInfo.end())
        return &(*it2).second;
    return nullptr;
}

AddrInfo* AddrManImpl::Create(const CAddress& addr, const CNetAddr& addrSource, nid_type* pnId)
{
    AssertLockHeld(cs);

    nid_type nId = nIdCount++;
    mapInfo[nId] = AddrInfo(addr, addrSource);
    mapAddr[addr] = nId;
    mapInfo[nId].nRandomPos = vRandom.size();
    vRandom.push_back(nId);
    nNew++;
    m_network_counts[addr.GetNetwork()].n_new++;
    if (pnId)
        *pnId = nId;
    return &mapInfo[nId];
}

void AddrManImpl::SwapRandom(unsigned int nRndPos1, unsigned int nRndPos2) const
{
    AssertLockHeld(cs);

    if (nRndPos1 == nRndPos2)
        return;

    assert(nRndPos1 < vRandom.size() && nRndPos2 < vRandom.size());

    nid_type nId1 = vRandom[nRndPos1];
    nid_type nId2 = vRandom[nRndPos2];

    const auto it_1{mapInfo.find(nId1)};
    const auto it_2{mapInfo.find(nId2)};
    assert(it_1 != mapInfo.end());
    assert(it_2 != mapInfo.end());

    it_1->second.nRandomPos = nRndPos2;
    it_2->second.nRandomPos = nRndPos1;

    vRandom[nRndPos1] = nId2;
    vRandom[nRndPos2] = nId1;
}

void AddrManImpl::Delete(nid_type nId)
{
    AssertLockHeld(cs);

    assert(mapInfo.count(nId) != 0);
    AddrInfo& info = mapInfo[nId];
    assert(!info.fInTried);
    assert(info.nRefCount == 0);

    SwapRandom(info.nRandomPos, vRandom.size() - 1);
    m_network_counts[info.GetNetwork()].n_new--;
    vRandom.pop_back();
    mapAddr.erase(info);
    mapInfo.erase(nId);
    nNew--;
}

void AddrManImpl::ClearNew(int nUBucket, int nUBucketPos)
{
    AssertLockHeld(cs);

    // if there is an entry in the specified bucket, delete it.
    if (vvNew[nUBucket][nUBucketPos] != -1) {
        nid_type nIdDelete = vvNew[nUBucket][nUBucketPos];
        AddrInfo& infoDelete = mapInfo[nIdDelete];
        assert(infoDelete.nRefCount > 0);
        infoDelete.nRefCount--;
        vvNew[nUBucket][nUBucketPos] = -1;
        LogDebug(BCLog::ADDRMAN, "Removed %s from new[%i][%i]\n", infoDelete.ToStringAddrPort(), nUBucket, nUBucketPos);
        if (infoDelete.nRefCount == 0) {
            Delete(nIdDelete);
        }
    }
}

void AddrManImpl::MakeTried(AddrInfo& info, nid_type nId)
{
    AssertLockHeld(cs);

    // remove the entry from all new buckets
    const int start_bucket{info.GetNewBucket(nKey, m_netgroupman)};
    for (int n = 0; n < ADDRMAN_NEW_BUCKET_COUNT; ++n) {
        const int bucket{(start_bucket + n) % ADDRMAN_NEW_BUCKET_COUNT};
        const int pos{info.GetBucketPosition(nKey, true, bucket)};
        if (vvNew[bucket][pos] == nId) {
            vvNew[bucket][pos] = -1;
            info.nRefCount--;
            if (info.nRefCount == 0) break;
        }
    }
    nNew--;
    m_network_counts[info.GetNetwork()].n_new--;

    assert(info.nRefCount == 0);

    // which tried bucket to move the entry to
    int nKBucket = info.GetTriedBucket(nKey, m_netgroupman);
    int nKBucketPos = info.GetBucketPosition(nKey, false, nKBucket);

    // first make space to add it (the existing tried entry there is moved to new, deleting whatever is there).
    if (vvTried[nKBucket][nKBucketPos] != -1) {
        // find an item to evict
        nid_type nIdEvict = vvTried[nKBucket][nKBucketPos];
        assert(mapInfo.count(nIdEvict) == 1);
        AddrInfo& infoOld = mapInfo[nIdEvict];

        // Remove the to-be-evicted item from the tried set.
        infoOld.fInTried = false;
        vvTried[nKBucket][nKBucketPos] = -1;
        nTried--;
        m_network_counts[infoOld.GetNetwork()].n_tried--;

        // find which new bucket it belongs to
        int nUBucket = infoOld.GetNewBucket(nKey, m_netgroupman);
        int nUBucketPos = infoOld.GetBucketPosition(nKey, true, nUBucket);
        ClearNew(nUBucket, nUBucketPos);
        assert(vvNew[nUBucket][nUBucketPos] == -1);

        // Enter it into the new set again.
        infoOld.nRefCount = 1;
        vvNew[nUBucket][nUBucketPos] = nIdEvict;
        nNew++;
        m_network_counts[infoOld.GetNetwork()].n_new++;
        LogDebug(BCLog::ADDRMAN, "Moved %s from tried[%i][%i] to new[%i][%i] to make space\n",
                 infoOld.ToStringAddrPort(), nKBucket, nKBucketPos, nUBucket, nUBucketPos);
    }
    assert(vvTried[nKBucket][nKBucketPos] == -1);

    vvTried[nKBucket][nKBucketPos] = nId;
    nTried++;
    info.fInTried = true;
    m_network_counts[info.GetNetwork()].n_tried++;
}

bool AddrManImpl::AddSingle(const CAddress& addr, const CNetAddr& source, std::chrono::seconds time_penalty)
{
    AssertLockHeld(cs);

    if (!addr.IsRoutable())
        return false;

    nid_type nId;
    AddrInfo* pinfo = Find(addr, &nId);

    // Do not set a penalty for a source's self-announcement
    if (addr == source) {
        time_penalty = 0s;
    }

    if (pinfo) {
        // periodically update nTime
        const bool currently_online{NodeClock::now() - addr.nTime < 24h};
        const auto update_interval{currently_online ? 1h : 24h};
        if (pinfo->nTime < addr.nTime - update_interval - time_penalty) {
            pinfo->nTime = std::max(NodeSeconds{0s}, addr.nTime - time_penalty);
        }

        // add services
        pinfo->nServices = ServiceFlags(pinfo->nServices | addr.nServices);

        // do not update if no new information is present
        if (addr.nTime <= pinfo->nTime) {
            return false;
        }

        // do not update if the entry was already in the "tried" table
        if (pinfo->fInTried)
            return false;

        // do not update if the max reference count is reached
        if (pinfo->nRefCount == ADDRMAN_NEW_BUCKETS_PER_ADDRESS)
            return false;

        // stochastic test: previous nRefCount == N: 2^N times harder to increase it
        if (pinfo->nRefCount > 0) {
            const int nFactor{1 << pinfo->nRefCount};
            if (insecure_rand.randrange(nFactor) != 0) return false;
        }
    } else {
        pinfo = Create(addr, source, &nId);
        pinfo->nTime = std::max(NodeSeconds{0s}, pinfo->nTime - time_penalty);
    }

    int nUBucket = pinfo->GetNewBucket(nKey, source, m_netgroupman);
    int nUBucketPos = pinfo->GetBucketPosition(nKey, true, nUBucket);
    bool fInsert = vvNew[nUBucket][nUBucketPos] == -1;
    if (vvNew[nUBucket][nUBucketPos] != nId) {
        if (!fInsert) {
            AddrInfo& infoExisting = mapInfo[vvNew[nUBucket][nUBucketPos]];
            if (infoExisting.IsTerrible() || (infoExisting.nRefCount > 1 && pinfo->nRefCount == 0)) {
                // Overwrite the existing new table entry.
                fInsert = true;
            }
        }
        if (fInsert) {
            ClearNew(nUBucket, nUBucketPos);
            pinfo->nRefCount++;
            vvNew[nUBucket][nUBucketPos] = nId;
            const auto mapped_as{m_netgroupman.GetMappedAS(addr)};
            LogDebug(BCLog::ADDRMAN, "Added %s%s to new[%i][%i]\n",
                     addr.ToStringAddrPort(), (mapped_as ? strprintf(" mapped to AS%i", mapped_as) : ""), nUBucket, nUBucketPos);
        } else {
            if (pinfo->nRefCount == 0) {
                Delete(nId);
            }
        }
    }
    return fInsert;
}

bool AddrManImpl::Good_(const CService& addr, bool test_before_evict, NodeSeconds time)
{
    AssertLockHeld(cs);

    nid_type nId;

    m_last_good = time;

    AddrInfo* pinfo = Find(addr, &nId);

    // if not found, bail out
    if (!pinfo) return false;

    AddrInfo& info = *pinfo;

    // update info
    info.m_last_success = time;
    info.m_last_try = time;
    info.nAttempts = 0;
    // nTime is not updated here, to avoid leaking information about
    // currently-connected peers.

    // if it is already in the tried set, don't do anything else
    if (info.fInTried) return false;

    // if it is not in new, something bad happened
    if (!Assume(info.nRefCount > 0)) return false;


    // which tried bucket to move the entry to
    int tried_bucket = info.GetTriedBucket(nKey, m_netgroupman);
    int tried_bucket_pos = info.GetBucketPosition(nKey, false, tried_bucket);

    // Will moving this address into tried evict another entry?
    if (test_before_evict && (vvTried[tried_bucket][tried_bucket_pos] != -1)) {
        if (m_tried_collisions.size() < ADDRMAN_SET_TRIED_COLLISION_SIZE) {
            m_tried_collisions.insert(nId);
        }
        // Output the entry we'd be colliding with, for debugging purposes
        auto colliding_entry = mapInfo.find(vvTried[tried_bucket][tried_bucket_pos]);
        LogDebug(BCLog::ADDRMAN, "Collision with %s while attempting to move %s to tried table. Collisions=%d\n",
                 colliding_entry != mapInfo.end() ? colliding_entry->second.ToStringAddrPort() : "",
                 addr.ToStringAddrPort(),
                 m_tried_collisions.size());
        return false;
    } else {
        // move nId to the tried tables
        MakeTried(info, nId);
        const auto mapped_as{m_netgroupman.GetMappedAS(addr)};
        LogDebug(BCLog::ADDRMAN, "Moved %s%s to tried[%i][%i]\n",
                 addr.ToStringAddrPort(), (mapped_as ? strprintf(" mapped to AS%i", mapped_as) : ""), tried_bucket, tried_bucket_pos);
        return true;
    }
}

bool AddrManImpl::Add_(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty)
{
    int added{0};
    for (std::vector<CAddress>::const_iterator it = vAddr.begin(); it != vAddr.end(); it++) {
        added += AddSingle(*it, source, time_penalty) ? 1 : 0;
    }
    if (added > 0) {
        LogDebug(BCLog::ADDRMAN, "Added %i addresses (of %i) from %s: %i tried, %i new\n", added, vAddr.size(), source.ToStringAddr(), nTried, nNew);
    }
    return added > 0;
}

void AddrManImpl::Attempt_(const CService& addr, bool fCountFailure, NodeSeconds time)
{
    AssertLockHeld(cs);

    AddrInfo* pinfo = Find(addr);

    // if not found, bail out
    if (!pinfo)
        return;

    AddrInfo& info = *pinfo;

    // update info
    info.m_last_try = time;
    if (fCountFailure && info.m_last_count_attempt < m_last_good) {
        info.m_last_count_attempt = time;
        info.nAttempts++;
    }
}

std::pair<CAddress, NodeSeconds> AddrManImpl::Select_(bool new_only, const std::unordered_set<Network>& networks) const
{
    AssertLockHeld(cs);

    if (vRandom.empty()) return {};

    size_t new_count = nNew;
    size_t tried_count = nTried;

    if (!networks.empty()) {
        new_count = 0;
        tried_count = 0;
        for (auto& network : networks) {
            auto it = m_network_counts.find(network);
            if (it == m_network_counts.end()) {
                continue;
            }
            auto counts = it->second;
            new_count += counts.n_new;
            tried_count += counts.n_tried;
        }
    }

    if (new_only && new_count == 0) return {};
    if (new_count + tried_count == 0) return {};

    // Decide if we are going to search the new or tried table
    // If either option is viable, use a 50% chance to choose
    bool search_tried;
    if (new_only || tried_count == 0) {
        search_tried = false;
    } else if (new_count == 0) {
        search_tried = true;
    } else {
        search_tried = insecure_rand.randbool();
    }

    const int bucket_count{search_tried ? ADDRMAN_TRIED_BUCKET_COUNT : ADDRMAN_NEW_BUCKET_COUNT};

    // Loop through the addrman table until we find an appropriate entry
    double chance_factor = 1.0;
    while (1) {
        // Pick a bucket, and an initial position in that bucket.
        int bucket = insecure_rand.randrange(bucket_count);
        int initial_position = insecure_rand.randrange(ADDRMAN_BUCKET_SIZE);

        // Iterate over the positions of that bucket, starting at the initial one,
        // and looping around.
        int i, position;
        nid_type node_id;
        for (i = 0; i < ADDRMAN_BUCKET_SIZE; ++i) {
            position = (initial_position + i) % ADDRMAN_BUCKET_SIZE;
            node_id = GetEntry(search_tried, bucket, position);
            if (node_id != -1) {
                if (!networks.empty()) {
                    const auto it{mapInfo.find(node_id)};
                    if (Assume(it != mapInfo.end()) && networks.contains(it->second.GetNetwork())) break;
                } else {
                    break;
                }
            }
        }

        // If the bucket is entirely empty, start over with a (likely) different one.
        if (i == ADDRMAN_BUCKET_SIZE) continue;

        // Find the entry to return.
        const auto it_found{mapInfo.find(node_id)};
        assert(it_found != mapInfo.end());
        const AddrInfo& info{it_found->second};

        // With probability GetChance() * chance_factor, return the entry.
        if (insecure_rand.randbits<30>() < chance_factor * info.GetChance() * (1 << 30)) {
            LogDebug(BCLog::ADDRMAN, "Selected %s from %s\n", info.ToStringAddrPort(), search_tried ? "tried" : "new");
            return {info, info.m_last_try};
        }

        // Otherwise start over with a (likely) different bucket, and increased chance factor.
        chance_factor *= 1.2;
    }
}

nid_type AddrManImpl::GetEntry(bool use_tried, size_t bucket, size_t position) const
{
    AssertLockHeld(cs);

    if (use_tried) {
        if (Assume(position < ADDRMAN_BUCKET_SIZE) && Assume(bucket < ADDRMAN_TRIED_BUCKET_COUNT)) {
            return vvTried[bucket][position];
        }
    } else {
        if (Assume(position < ADDRMAN_BUCKET_SIZE) && Assume(bucket < ADDRMAN_NEW_BUCKET_COUNT)) {
            return vvNew[bucket][position];
        }
    }

    return -1;
}

std::vector<CAddress> AddrManImpl::GetAddr_(size_t max_addresses, size_t max_pct, std::optional<Network> network, const bool filtered) const
{
    AssertLockHeld(cs);
    Assume(max_pct <= 100);

    size_t nNodes = vRandom.size();
    if (max_pct != 0) {
        max_pct = std::min(max_pct, size_t{100});
        nNodes = max_pct * nNodes / 100;
    }
    if (max_addresses != 0) {
        nNodes = std::min(nNodes, max_addresses);
    }

    // gather a list of random nodes, skipping those of low quality
    const auto now{Now<NodeSeconds>()};
    std::vector<CAddress> addresses;
    addresses.reserve(nNodes);
    for (unsigned int n = 0; n < vRandom.size(); n++) {
        if (addresses.size() >= nNodes)
            break;

        int nRndPos = insecure_rand.randrange(vRandom.size() - n) + n;
        SwapRandom(n, nRndPos);
        const auto it{mapInfo.find(vRandom[n])};
        assert(it != mapInfo.end());

        const AddrInfo& ai{it->second};

        // Filter by network (optional)
        if (network != std::nullopt && ai.GetNetClass() != network) continue;

        // Filter for quality
        if (ai.IsTerrible(now) && filtered) continue;

        addresses.push_back(ai);
    }
    LogDebug(BCLog::ADDRMAN, "GetAddr returned %d random addresses\n", addresses.size());
    return addresses;
}

std::vector<std::pair<AddrInfo, AddressPosition>> AddrManImpl::GetEntries_(bool from_tried) const
{
    AssertLockHeld(cs);

    const int bucket_count = from_tried ? ADDRMAN_TRIED_BUCKET_COUNT : ADDRMAN_NEW_BUCKET_COUNT;
    std::vector<std::pair<AddrInfo, AddressPosition>> infos;
    for (int bucket = 0; bucket < bucket_count; ++bucket) {
        for (int position = 0; position < ADDRMAN_BUCKET_SIZE; ++position) {
            nid_type id = GetEntry(from_tried, bucket, position);
            if (id >= 0) {
                AddrInfo info = mapInfo.at(id);
                AddressPosition location = AddressPosition(
                    from_tried,
                    /*multiplicity_in=*/from_tried ? 1 : info.nRefCount,
                    bucket,
                    position);
                infos.emplace_back(info, location);
            }
        }
    }

    return infos;
}

void AddrManImpl::Connected_(const CService& addr, NodeSeconds time)
{
    AssertLockHeld(cs);

    AddrInfo* pinfo = Find(addr);

    // if not found, bail out
    if (!pinfo)
        return;

    AddrInfo& info = *pinfo;

    // update info
    const auto update_interval{20min};
    if (time - info.nTime > update_interval) {
        info.nTime = time;
    }
}

void AddrManImpl::SetServices_(const CService& addr, ServiceFlags nServices)
{
    AssertLockHeld(cs);

    AddrInfo* pinfo = Find(addr);

    // if not found, bail out
    if (!pinfo)
        return;

    AddrInfo& info = *pinfo;

    // update info
    info.nServices = nServices;
}

void AddrManImpl::ResolveCollisions_()
{
    AssertLockHeld(cs);

    for (std::set<nid_type>::iterator it = m_tried_collisions.begin(); it != m_tried_collisions.end();) {
        nid_type id_new = *it;

        bool erase_collision = false;

        // If id_new not found in mapInfo remove it from m_tried_collisions
        if (mapInfo.count(id_new) != 1) {
            erase_collision = true;
        } else {
            AddrInfo& info_new = mapInfo[id_new];

            // Which tried bucket to move the entry to.
            int tried_bucket = info_new.GetTriedBucket(nKey, m_netgroupman);
            int tried_bucket_pos = info_new.GetBucketPosition(nKey, false, tried_bucket);
            if (!info_new.IsValid()) { // id_new may no longer map to a valid address
                erase_collision = true;
            } else if (vvTried[tried_bucket][tried_bucket_pos] != -1) { // The position in the tried bucket is not empty

                // Get the to-be-evicted address that is being tested
                nid_type id_old = vvTried[tried_bucket][tried_bucket_pos];
                AddrInfo& info_old = mapInfo[id_old];

                const auto current_time{Now<NodeSeconds>()};

                // Has successfully connected in last X hours
                if (current_time - info_old.m_last_success < ADDRMAN_REPLACEMENT) {
                    erase_collision = true;
                } else if (current_time - info_old.m_last_try < ADDRMAN_REPLACEMENT) { // attempted to connect and failed in last X hours

                    // Give address at least 60 seconds to successfully connect
                    if (current_time - info_old.m_last_try > 60s) {
                        LogDebug(BCLog::ADDRMAN, "Replacing %s with %s in tried table\n", info_old.ToStringAddrPort(), info_new.ToStringAddrPort());

                        // Replaces an existing address already in the tried table with the new address
                        Good_(info_new, false, current_time);
                        erase_collision = true;
                    }
                } else if (current_time - info_new.m_last_success > ADDRMAN_TEST_WINDOW) {
                    // If the collision hasn't resolved in some reasonable amount of time,
                    // just evict the old entry -- we must not be able to
                    // connect to it for some reason.
                    LogDebug(BCLog::ADDRMAN, "Unable to test; replacing %s with %s in tried table anyway\n", info_old.ToStringAddrPort(), info_new.ToStringAddrPort());
                    Good_(info_new, false, current_time);
                    erase_collision = true;
                }
            } else { // Collision is not actually a collision anymore
                Good_(info_new, false, Now<NodeSeconds>());
                erase_collision = true;
            }
        }

        if (erase_collision) {
            m_tried_collisions.erase(it++);
        } else {
            it++;
        }
    }
}

std::pair<CAddress, NodeSeconds> AddrManImpl::SelectTriedCollision_()
{
    AssertLockHeld(cs);

    if (m_tried_collisions.size() == 0) return {};

    std::set<nid_type>::iterator it = m_tried_collisions.begin();

    // Selects a random element from m_tried_collisions
    std::advance(it, insecure_rand.randrange(m_tried_collisions.size()));
    nid_type id_new = *it;

    // If id_new not found in mapInfo remove it from m_tried_collisions
    if (mapInfo.count(id_new) != 1) {
        m_tried_collisions.erase(it);
        return {};
    }

    const AddrInfo& newInfo = mapInfo[id_new];

    // which tried bucket to move the entry to
    int tried_bucket = newInfo.GetTriedBucket(nKey, m_netgroupman);
    int tried_bucket_pos = newInfo.GetBucketPosition(nKey, false, tried_bucket);

    const AddrInfo& info_old = mapInfo[vvTried[tried_bucket][tried_bucket_pos]];
    return {info_old, info_old.m_last_try};
}

std::optional<AddressPosition> AddrManImpl::FindAddressEntry_(const CAddress& addr)
{
    AssertLockHeld(cs);

    AddrInfo* addr_info = Find(addr);

    if (!addr_info) return std::nullopt;

    if(addr_info->fInTried) {
        int bucket{addr_info->GetTriedBucket(nKey, m_netgroupman)};
        return AddressPosition(/*tried_in=*/true,
                               /*multiplicity_in=*/1,
                               /*bucket_in=*/bucket,
                               /*position_in=*/addr_info->GetBucketPosition(nKey, false, bucket));
    } else {
        int bucket{addr_info->GetNewBucket(nKey, m_netgroupman)};
        return AddressPosition(/*tried_in=*/false,
                               /*multiplicity_in=*/addr_info->nRefCount,
                               /*bucket_in=*/bucket,
                               /*position_in=*/addr_info->GetBucketPosition(nKey, true, bucket));
    }
}

size_t AddrManImpl::Size_(std::optional<Network> net, std::optional<bool> in_new) const
{
    AssertLockHeld(cs);

    if (!net.has_value()) {
        if (in_new.has_value()) {
            return *in_new ? nNew : nTried;
        } else {
            return vRandom.size();
        }
    }
    if (auto it = m_network_counts.find(*net); it != m_network_counts.end()) {
        auto net_count = it->second;
        if (in_new.has_value()) {
            return *in_new ? net_count.n_new : net_count.n_tried;
        } else {
            return net_count.n_new + net_count.n_tried;
        }
    }
    return 0;
}

void AddrManImpl::Check() const
{
    AssertLockHeld(cs);

    // Run consistency checks 1 in m_consistency_check_ratio times if enabled
    if (m_consistency_check_ratio == 0) return;
    if (insecure_rand.randrange(m_consistency_check_ratio) >= 1) return;

    const int err{CheckAddrman()};
    if (err) {
        LogError("ADDRMAN CONSISTENCY CHECK FAILED!!! err=%i", err);
        assert(false);
    }
}

int AddrManImpl::CheckAddrman() const
{
    AssertLockHeld(cs);

    LOG_TIME_MILLIS_WITH_CATEGORY_MSG_ONCE(
        strprintf("new %i, tried %i, total %u", nNew, nTried, vRandom.size()), BCLog::ADDRMAN);

    std::unordered_set<nid_type> setTried;
    std::unordered_map<nid_type, int> mapNew;
    std::unordered_map<Network, NewTriedCount> local_counts;

    if (vRandom.size() != (size_t)(nTried + nNew))
        return -7;

    for (const auto& entry : mapInfo) {
        nid_type n = entry.first;
        const AddrInfo& info = entry.second;
        if (info.fInTried) {
            if (!TicksSinceEpoch<std::chrono::seconds>(info.m_last_success)) {
                return -1;
            }
            if (info.nRefCount)
                return -2;
            setTried.insert(n);
            local_counts[info.GetNetwork()].n_tried++;
        } else {
            if (info.nRefCount < 0 || info.nRefCount > ADDRMAN_NEW_BUCKETS_PER_ADDRESS)
                return -3;
            if (!info.nRefCount)
                return -4;
            mapNew[n] = info.nRefCount;
            local_counts[info.GetNetwork()].n_new++;
        }
        const auto it{mapAddr.find(info)};
        if (it == mapAddr.end() || it->second != n) {
            return -5;
        }
        if (info.nRandomPos < 0 || (size_t)info.nRandomPos >= vRandom.size() || vRandom[info.nRandomPos] != n)
            return -14;
        if (info.m_last_try < NodeSeconds{0s}) {
            return -6;
        }
        if (info.m_last_success < NodeSeconds{0s}) {
            return -8;
        }
    }

    if (setTried.size() != (size_t)nTried)
        return -9;
    if (mapNew.size() != (size_t)nNew)
        return -10;

    for (int n = 0; n < ADDRMAN_TRIED_BUCKET_COUNT; n++) {
        for (int i = 0; i < ADDRMAN_BUCKET_SIZE; i++) {
            if (vvTried[n][i] != -1) {
                if (!setTried.count(vvTried[n][i]))
                    return -11;
                const auto it{mapInfo.find(vvTried[n][i])};
                if (it == mapInfo.end() || it->second.GetTriedBucket(nKey, m_netgroupman) != n) {
                    return -17;
                }
                if (it->second.GetBucketPosition(nKey, false, n) != i) {
                    return -18;
                }
                setTried.erase(vvTried[n][i]);
            }
        }
    }

    for (int n = 0; n < ADDRMAN_NEW_BUCKET_COUNT; n++) {
        for (int i = 0; i < ADDRMAN_BUCKET_SIZE; i++) {
            if (vvNew[n][i] != -1) {
                if (!mapNew.count(vvNew[n][i]))
                    return -12;
                const auto it{mapInfo.find(vvNew[n][i])};
                if (it == mapInfo.end() || it->second.GetBucketPosition(nKey, true, n) != i) {
                    return -19;
                }
                if (--mapNew[vvNew[n][i]] == 0)
                    mapNew.erase(vvNew[n][i]);
            }
        }
    }

    if (setTried.size())
        return -13;
    if (mapNew.size())
        return -15;
    if (nKey.IsNull())
        return -16;

    // It's possible that m_network_counts may have all-zero entries that local_counts
    // doesn't have if addrs from a network were being added and then removed again in the past.
    if (m_network_counts.size() < local_counts.size()) {
        return -20;
    }
    for (const auto& [net, count] : m_network_counts) {
        if (local_counts[net].n_new != count.n_new || local_counts[net].n_tried != count.n_tried) {
            return -21;
        }
    }

    return 0;
}

size_t AddrManImpl::Size(std::optional<Network> net, std::optional<bool> in_new) const
{
    LOCK(cs);
    Check();
    auto ret = Size_(net, in_new);
    Check();
    return ret;
}

bool AddrManImpl::Add(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty)
{
    LOCK(cs);
    Check();
    auto ret = Add_(vAddr, source, time_penalty);
    Check();
    return ret;
}

bool AddrManImpl::Good(const CService& addr, NodeSeconds time)
{
    LOCK(cs);
    Check();
    auto ret = Good_(addr, /*test_before_evict=*/true, time);
    Check();
    return ret;
}

void AddrManImpl::Attempt(const CService& addr, bool fCountFailure, NodeSeconds time)
{
    LOCK(cs);
    Check();
    Attempt_(addr, fCountFailure, time);
    Check();
}

void AddrManImpl::ResolveCollisions()
{
    LOCK(cs);
    Check();
    ResolveCollisions_();
    Check();
}

std::pair<CAddress, NodeSeconds> AddrManImpl::SelectTriedCollision()
{
    LOCK(cs);
    Check();
    auto ret = SelectTriedCollision_();
    Check();
    return ret;
}

std::pair<CAddress, NodeSeconds> AddrManImpl::Select(bool new_only, const std::unordered_set<Network>& networks) const
{
    LOCK(cs);
    Check();
    auto addrRet = Select_(new_only, networks);
    Check();
    return addrRet;
}

std::vector<CAddress> AddrManImpl::GetAddr(size_t max_addresses, size_t max_pct, std::optional<Network> network, const bool filtered) const
{
    LOCK(cs);
    Check();
    auto addresses = GetAddr_(max_addresses, max_pct, network, filtered);
    Check();
    return addresses;
}

std::vector<std::pair<AddrInfo, AddressPosition>> AddrManImpl::GetEntries(bool from_tried) const
{
    LOCK(cs);
    Check();
    auto addrInfos = GetEntries_(from_tried);
    Check();
    return addrInfos;
}

void AddrManImpl::Connected(const CService& addr, NodeSeconds time)
{
    LOCK(cs);
    Check();
    Connected_(addr, time);
    Check();
}

void AddrManImpl::SetServices(const CService& addr, ServiceFlags nServices)
{
    LOCK(cs);
    Check();
    SetServices_(addr, nServices);
    Check();
}

std::optional<AddressPosition> AddrManImpl::FindAddressEntry(const CAddress& addr)
{
    LOCK(cs);
    Check();
    auto entry = FindAddressEntry_(addr);
    Check();
    return entry;
}

AddrMan::AddrMan(const NetGroupManager& netgroupman, bool deterministic, int32_t consistency_check_ratio)
    : m_impl(std::make_unique<AddrManImpl>(netgroupman, deterministic, consistency_check_ratio)) {}

AddrMan::~AddrMan() = default;

template <typename Stream>
void AddrMan::Serialize(Stream& s_) const
{
    m_impl->Serialize<Stream>(s_);
}

template <typename Stream>
void AddrMan::Unserialize(Stream& s_)
{
    m_impl->Unserialize<Stream>(s_);
}

// explicit instantiation
template void AddrMan::Serialize(HashedSourceWriter<AutoFile>&) const;
template void AddrMan::Serialize(DataStream&) const;
template void AddrMan::Unserialize(AutoFile&);
template void AddrMan::Unserialize(HashVerifier<AutoFile>&);
template void AddrMan::Unserialize(DataStream&);
template void AddrMan::Unserialize(HashVerifier<DataStream>&);

size_t AddrMan::Size(std::optional<Network> net, std::optional<bool> in_new) const
{
    return m_impl->Size(net, in_new);
}

bool AddrMan::Add(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty)
{
    return m_impl->Add(vAddr, source, time_penalty);
}

bool AddrMan::Good(const CService& addr, NodeSeconds time)
{
    return m_impl->Good(addr, time);
}

void AddrMan::Attempt(const CService& addr, bool fCountFailure, NodeSeconds time)
{
    m_impl->Attempt(addr, fCountFailure, time);
}

void AddrMan::ResolveCollisions()
{
    m_impl->ResolveCollisions();
}

std::pair<CAddress, NodeSeconds> AddrMan::SelectTriedCollision()
{
    return m_impl->SelectTriedCollision();
}

std::pair<CAddress, NodeSeconds> AddrMan::Select(bool new_only, const std::unordered_set<Network>& networks) const
{
    return m_impl->Select(new_only, networks);
}

std::vector<CAddress> AddrMan::GetAddr(size_t max_addresses, size_t max_pct, std::optional<Network> network, const bool filtered) const
{
    return m_impl->GetAddr(max_addresses, max_pct, network, filtered);
}

std::vector<std::pair<AddrInfo, AddressPosition>> AddrMan::GetEntries(bool use_tried) const
{
    return m_impl->GetEntries(use_tried);
}

void AddrMan::Connected(const CService& addr, NodeSeconds time)
{
    m_impl->Connected(addr, time);
}

void AddrMan::SetServices(const CService& addr, ServiceFlags nServices)
{
    m_impl->SetServices(addr, nServices);
}

std::optional<AddressPosition> AddrMan::FindAddressEntry(const CAddress& addr)
{
    return m_impl->FindAddressEntry(addr);
}
