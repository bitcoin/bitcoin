// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/keyorigin.h>
#include <script/interpreter.h>
#include <script/signingprovider.h>

#include <logging.h>

const SigningProvider& DUMMY_SIGNING_PROVIDER = SigningProvider();

template<typename M, typename K, typename V>
bool LookupHelper(const M& map, const K& key, V& value)
{
    auto it = map.find(key);
    if (it != map.end()) {
        value = it->second;
        return true;
    }
    return false;
}

bool HidingSigningProvider::GetCScript(const CScriptID& scriptid, CScript& script) const
{
    return m_provider->GetCScript(scriptid, script);
}

bool HidingSigningProvider::GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const
{
    return m_provider->GetPubKey(keyid, pubkey);
}

bool HidingSigningProvider::GetKey(const CKeyID& keyid, CKey& key) const
{
    if (m_hide_secret) return false;
    return m_provider->GetKey(keyid, key);
}

bool HidingSigningProvider::GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const
{
    if (m_hide_origin) return false;
    return m_provider->GetKeyOrigin(keyid, info);
}

bool HidingSigningProvider::GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const
{
    return m_provider->GetTaprootSpendData(output_key, spenddata);
}
bool HidingSigningProvider::GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const
{
    return m_provider->GetTaprootBuilder(output_key, builder);
}

bool FlatSigningProvider::GetCScript(const CScriptID& scriptid, CScript& script) const { return LookupHelper(scripts, scriptid, script); }
bool FlatSigningProvider::GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const { return LookupHelper(pubkeys, keyid, pubkey); }
bool FlatSigningProvider::GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const
{
    std::pair<CPubKey, KeyOriginInfo> out;
    bool ret = LookupHelper(origins, keyid, out);
    if (ret) info = std::move(out.second);
    return ret;
}
bool FlatSigningProvider::GetKey(const CKeyID& keyid, CKey& key) const { return LookupHelper(keys, keyid, key); }
bool FlatSigningProvider::GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const
{
    TaprootBuilder builder;
    if (LookupHelper(tr_trees, output_key, builder)) {
        spenddata = builder.GetSpendData();
        return true;
    }
    return false;
}
bool FlatSigningProvider::GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const
{
    return LookupHelper(tr_trees, output_key, builder);
}

FlatSigningProvider& FlatSigningProvider::Merge(FlatSigningProvider&& b)
{
    scripts.merge(b.scripts);
    pubkeys.merge(b.pubkeys);
    keys.merge(b.keys);
    origins.merge(b.origins);
    tr_trees.merge(b.tr_trees);
    return *this;
}

void FillableSigningProvider::ImplicitlyLearnRelatedKeyScripts(const CPubKey& pubkey)
{
    AssertLockHeld(cs_KeyStore);
    CKeyID key_id = pubkey.GetID();
    // This adds the redeemscripts necessary to detect P2WPKH and P2SH-P2WPKH
    // outputs. Technically P2WPKH outputs don't have a redeemscript to be
    // spent. However, our current IsMine logic requires the corresponding
    // P2SH-P2WPKH redeemscript to be present in the wallet in order to accept
    // payment even to P2WPKH outputs.
    // Also note that having superfluous scripts in the keystore never hurts.
    // They're only used to guide recursion in signing and IsMine logic - if
    // a script is present but we can't do anything with it, it has no effect.
    // "Implicitly" refers to fact that scripts are derived automatically from
    // existing keys, and are present in memory, even without being explicitly
    // loaded (e.g. from a file).
    if (pubkey.IsCompressed()) {
        CScript script = GetScriptForDestination(WitnessV0KeyHash(key_id));
        // This does not use AddCScript, as it may be overridden.
        CScriptID id(script);
        mapScripts[id] = std::move(script);
    }
}

bool FillableSigningProvider::GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const
{
    CKey key;
    if (!GetKey(address, key)) {
        return false;
    }
    vchPubKeyOut = key.GetPubKey();
    return true;
}

bool FillableSigningProvider::AddKeyPubKey(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetID()] = key;
    ImplicitlyLearnRelatedKeyScripts(pubkey);
    return true;
}

bool FillableSigningProvider::HaveKey(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    return mapKeys.count(address) > 0;
}

std::set<CKeyID> FillableSigningProvider::GetKeys() const
{
    LOCK(cs_KeyStore);
    std::set<CKeyID> set_address;
    for (const auto& mi : mapKeys) {
        set_address.insert(mi.first);
    }
    return set_address;
}

bool FillableSigningProvider::GetKey(const CKeyID &address, CKey &keyOut) const
{
    LOCK(cs_KeyStore);
    KeyMap::const_iterator mi = mapKeys.find(address);
    if (mi != mapKeys.end()) {
        keyOut = mi->second;
        return true;
    }
    return false;
}

bool FillableSigningProvider::AddCScript(const CScript& redeemScript)
{
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE) {
        LogError("FillableSigningProvider::AddCScript(): redeemScripts > %i bytes are invalid\n", MAX_SCRIPT_ELEMENT_SIZE);
        return false;
    }

    LOCK(cs_KeyStore);
    mapScripts[CScriptID(redeemScript)] = redeemScript;
    return true;
}

bool FillableSigningProvider::HaveCScript(const CScriptID& hash) const
{
    LOCK(cs_KeyStore);
    return mapScripts.count(hash) > 0;
}

std::set<CScriptID> FillableSigningProvider::GetCScripts() const
{
    LOCK(cs_KeyStore);
    std::set<CScriptID> set_script;
    for (const auto& mi : mapScripts) {
        set_script.insert(mi.first);
    }
    return set_script;
}

bool FillableSigningProvider::GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const
{
    LOCK(cs_KeyStore);
    ScriptMap::const_iterator mi = mapScripts.find(hash);
    if (mi != mapScripts.end())
    {
        redeemScriptOut = (*mi).second;
        return true;
    }
    return false;
}

CKeyID GetKeyForDestination(const SigningProvider& store, const CTxDestination& dest)
{
    // Only supports destinations which map to single public keys:
    // P2PKH, P2WPKH, P2SH-P2WPKH, P2TR
    if (auto id = std::get_if<PKHash>(&dest)) {
        return ToKeyID(*id);
    }
    if (auto witness_id = std::get_if<WitnessV0KeyHash>(&dest)) {
        return ToKeyID(*witness_id);
    }
    if (auto script_hash = std::get_if<ScriptHash>(&dest)) {
        CScript script;
        CScriptID script_id = ToScriptID(*script_hash);
        CTxDestination inner_dest;
        if (store.GetCScript(script_id, script) && ExtractDestination(script, inner_dest)) {
            if (auto inner_witness_id = std::get_if<WitnessV0KeyHash>(&inner_dest)) {
                return ToKeyID(*inner_witness_id);
            }
        }
    }
    if (auto output_key = std::get_if<WitnessV1Taproot>(&dest)) {
        TaprootSpendData spenddata;
        CPubKey pub;
        if (store.GetTaprootSpendData(*output_key, spenddata)
            && !spenddata.internal_key.IsNull()
            && spenddata.merkle_root.IsNull()
            && store.GetPubKeyByXOnly(spenddata.internal_key, pub)) {
            return pub.GetID();
        }
    }
    return CKeyID();
}

void MultiSigningProvider::AddProvider(std::unique_ptr<SigningProvider> provider)
{
    m_providers.push_back(std::move(provider));
}

bool MultiSigningProvider::GetCScript(const CScriptID& scriptid, CScript& script) const
{
    for (const auto& provider: m_providers) {
        if (provider->GetCScript(scriptid, script)) return true;
    }
    return false;
}

bool MultiSigningProvider::GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const
{
    for (const auto& provider: m_providers) {
        if (provider->GetPubKey(keyid, pubkey)) return true;
    }
    return false;
}


bool MultiSigningProvider::GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const
{
    for (const auto& provider: m_providers) {
        if (provider->GetKeyOrigin(keyid, info)) return true;
    }
    return false;
}

bool MultiSigningProvider::GetKey(const CKeyID& keyid, CKey& key) const
{
    for (const auto& provider: m_providers) {
        if (provider->GetKey(keyid, key)) return true;
    }
    return false;
}

bool MultiSigningProvider::GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const
{
    for (const auto& provider: m_providers) {
        if (provider->GetTaprootSpendData(output_key, spenddata)) return true;
    }
    return false;
}

bool MultiSigningProvider::GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const
{
    for (const auto& provider: m_providers) {
        if (provider->GetTaprootBuilder(output_key, builder)) return true;
    }
    return false;
}

/*static*/ TaprootBuilder::NodeInfo TaprootBuilder::Combine(NodeInfo&& a, NodeInfo&& b)
{
    NodeInfo ret;
    /* Iterate over all tracked leaves in a, add b's hash to their Merkle branch, and move them to ret. */
    for (auto& leaf : a.leaves) {
        leaf.merkle_branch.push_back(b.hash);
        ret.leaves.emplace_back(std::move(leaf));
    }
    /* Iterate over all tracked leaves in b, add a's hash to their Merkle branch, and move them to ret. */
    for (auto& leaf : b.leaves) {
        leaf.merkle_branch.push_back(a.hash);
        ret.leaves.emplace_back(std::move(leaf));
    }
    ret.hash = ComputeTapbranchHash(a.hash, b.hash);
    return ret;
}

void TaprootSpendData::Merge(TaprootSpendData other)
{
    // TODO: figure out how to better deal with conflicting information
    // being merged.
    if (internal_key.IsNull() && !other.internal_key.IsNull()) {
        internal_key = other.internal_key;
    }
    if (merkle_root.IsNull() && !other.merkle_root.IsNull()) {
        merkle_root = other.merkle_root;
    }
    for (auto& [key, control_blocks] : other.scripts) {
        scripts[key].merge(std::move(control_blocks));
    }
}

void TaprootBuilder::Insert(TaprootBuilder::NodeInfo&& node, int depth)
{
    assert(depth >= 0 && (size_t)depth <= TAPROOT_CONTROL_MAX_NODE_COUNT);
    /* We cannot insert a leaf at a lower depth while a deeper branch is unfinished. Doing
     * so would mean the Add() invocations do not correspond to a DFS traversal of a
     * binary tree. */
    if ((size_t)depth + 1 < m_branch.size()) {
        m_valid = false;
        return;
    }
    /* As long as an entry in the branch exists at the specified depth, combine it and propagate up.
     * The 'node' variable is overwritten here with the newly combined node. */
    while (m_valid && m_branch.size() > (size_t)depth && m_branch[depth].has_value()) {
        node = Combine(std::move(node), std::move(*m_branch[depth]));
        m_branch.pop_back();
        if (depth == 0) m_valid = false; /* Can't propagate further up than the root */
        --depth;
    }
    if (m_valid) {
        /* Make sure the branch is big enough to place the new node. */
        if (m_branch.size() <= (size_t)depth) m_branch.resize((size_t)depth + 1);
        assert(!m_branch[depth].has_value());
        m_branch[depth] = std::move(node);
    }
}

/*static*/ bool TaprootBuilder::ValidDepths(const std::vector<int>& depths)
{
    std::vector<bool> branch;
    for (int depth : depths) {
        // This inner loop corresponds to effectively the same logic on branch
        // as what Insert() performs on the m_branch variable. Instead of
        // storing a NodeInfo object, just remember whether or not there is one
        // at that depth.
        if (depth < 0 || (size_t)depth > TAPROOT_CONTROL_MAX_NODE_COUNT) return false;
        if ((size_t)depth + 1 < branch.size()) return false;
        while (branch.size() > (size_t)depth && branch[depth]) {
            branch.pop_back();
            if (depth == 0) return false;
            --depth;
        }
        if (branch.size() <= (size_t)depth) branch.resize((size_t)depth + 1);
        assert(!branch[depth]);
        branch[depth] = true;
    }
    // And this check corresponds to the IsComplete() check on m_branch.
    return branch.size() == 0 || (branch.size() == 1 && branch[0]);
}

TaprootBuilder& TaprootBuilder::Add(int depth, Span<const unsigned char> script, int leaf_version, bool track)
{
    assert((leaf_version & ~TAPROOT_LEAF_MASK) == 0);
    if (!IsValid()) return *this;
    /* Construct NodeInfo object with leaf hash and (if track is true) also leaf information. */
    NodeInfo node;
    node.hash = ComputeTapleafHash(leaf_version, script);
    if (track) node.leaves.emplace_back(LeafInfo{std::vector<unsigned char>(script.begin(), script.end()), leaf_version, {}});
    /* Insert into the branch. */
    Insert(std::move(node), depth);
    return *this;
}

TaprootBuilder& TaprootBuilder::AddOmitted(int depth, const uint256& hash)
{
    if (!IsValid()) return *this;
    /* Construct NodeInfo object with the hash directly, and insert it into the branch. */
    NodeInfo node;
    node.hash = hash;
    Insert(std::move(node), depth);
    return *this;
}

TaprootBuilder& TaprootBuilder::Finalize(const XOnlyPubKey& internal_key)
{
    /* Can only call this function when IsComplete() is true. */
    assert(IsComplete());
    m_internal_key = internal_key;
    auto ret = m_internal_key.CreateTapTweak(m_branch.size() == 0 ? nullptr : &m_branch[0]->hash);
    assert(ret.has_value());
    std::tie(m_output_key, m_parity) = *ret;
    return *this;
}

WitnessV1Taproot TaprootBuilder::GetOutput() { return WitnessV1Taproot{m_output_key}; }

TaprootSpendData TaprootBuilder::GetSpendData() const
{
    assert(IsComplete());
    assert(m_output_key.IsFullyValid());
    TaprootSpendData spd;
    spd.merkle_root = m_branch.size() == 0 ? uint256() : m_branch[0]->hash;
    spd.internal_key = m_internal_key;
    if (m_branch.size()) {
        // If any script paths exist, they have been combined into the root m_branch[0]
        // by now. Compute the control block for each of its tracked leaves, and put them in
        // spd.scripts.
        for (const auto& leaf : m_branch[0]->leaves) {
            std::vector<unsigned char> control_block;
            control_block.resize(TAPROOT_CONTROL_BASE_SIZE + TAPROOT_CONTROL_NODE_SIZE * leaf.merkle_branch.size());
            control_block[0] = leaf.leaf_version | (m_parity ? 1 : 0);
            std::copy(m_internal_key.begin(), m_internal_key.end(), control_block.begin() + 1);
            if (leaf.merkle_branch.size()) {
                std::copy(leaf.merkle_branch[0].begin(),
                          leaf.merkle_branch[0].begin() + TAPROOT_CONTROL_NODE_SIZE * leaf.merkle_branch.size(),
                          control_block.begin() + TAPROOT_CONTROL_BASE_SIZE);
            }
            spd.scripts[{leaf.script, leaf.leaf_version}].insert(std::move(control_block));
        }
    }
    return spd;
}

std::optional<std::vector<std::tuple<int, std::vector<unsigned char>, int>>> InferTaprootTree(const TaprootSpendData& spenddata, const XOnlyPubKey& output)
{
    // Verify that the output matches the assumed Merkle root and internal key.
    auto tweak = spenddata.internal_key.CreateTapTweak(spenddata.merkle_root.IsNull() ? nullptr : &spenddata.merkle_root);
    if (!tweak || tweak->first != output) return std::nullopt;
    // If the Merkle root is 0, the tree is empty, and we're done.
    std::vector<std::tuple<int, std::vector<unsigned char>, int>> ret;
    if (spenddata.merkle_root.IsNull()) return ret;

    /** Data structure to represent the nodes of the tree we're going to build. */
    struct TreeNode {
        /** Hash of this node, if known; 0 otherwise. */
        uint256 hash;
        /** The left and right subtrees (note that their order is irrelevant). */
        std::unique_ptr<TreeNode> sub[2];
        /** If this is known to be a leaf node, a pointer to the (script, leaf_ver) pair.
         *  nullptr otherwise. */
        const std::pair<std::vector<unsigned char>, int>* leaf = nullptr;
        /** Whether or not this node has been explored (is known to be a leaf, or known to have children). */
        bool explored = false;
        /** Whether or not this node is an inner node (unknown until explored = true). */
        bool inner;
        /** Whether or not we have produced output for this subtree. */
        bool done = false;
    };

    // Build tree from the provided branches.
    TreeNode root;
    root.hash = spenddata.merkle_root;
    for (const auto& [key, control_blocks] : spenddata.scripts) {
        const auto& [script, leaf_ver] = key;
        for (const auto& control : control_blocks) {
            // Skip script records with nonsensical leaf version.
            if (leaf_ver < 0 || leaf_ver >= 0x100 || leaf_ver & 1) continue;
            // Skip script records with invalid control block sizes.
            if (control.size() < TAPROOT_CONTROL_BASE_SIZE || control.size() > TAPROOT_CONTROL_MAX_SIZE ||
                ((control.size() - TAPROOT_CONTROL_BASE_SIZE) % TAPROOT_CONTROL_NODE_SIZE) != 0) continue;
            // Skip script records that don't match the control block.
            if ((control[0] & TAPROOT_LEAF_MASK) != leaf_ver) continue;
            // Skip script records that don't match the provided Merkle root.
            const uint256 leaf_hash = ComputeTapleafHash(leaf_ver, script);
            const uint256 merkle_root = ComputeTaprootMerkleRoot(control, leaf_hash);
            if (merkle_root != spenddata.merkle_root) continue;

            TreeNode* node = &root;
            size_t levels = (control.size() - TAPROOT_CONTROL_BASE_SIZE) / TAPROOT_CONTROL_NODE_SIZE;
            for (size_t depth = 0; depth < levels; ++depth) {
                // Can't descend into a node which we already know is a leaf.
                if (node->explored && !node->inner) return std::nullopt;

                // Extract partner hash from Merkle branch in control block.
                uint256 hash;
                std::copy(control.begin() + TAPROOT_CONTROL_BASE_SIZE + (levels - 1 - depth) * TAPROOT_CONTROL_NODE_SIZE,
                          control.begin() + TAPROOT_CONTROL_BASE_SIZE + (levels - depth) * TAPROOT_CONTROL_NODE_SIZE,
                          hash.begin());

                if (node->sub[0]) {
                    // Descend into the existing left or right branch.
                    bool desc = false;
                    for (int i = 0; i < 2; ++i) {
                        if (node->sub[i]->hash == hash || (node->sub[i]->hash.IsNull() && node->sub[1-i]->hash != hash)) {
                            node->sub[i]->hash = hash;
                            node = &*node->sub[1-i];
                            desc = true;
                            break;
                        }
                    }
                    if (!desc) return std::nullopt; // This probably requires a hash collision to hit.
                } else {
                    // We're in an unexplored node. Create subtrees and descend.
                    node->explored = true;
                    node->inner = true;
                    node->sub[0] = std::make_unique<TreeNode>();
                    node->sub[1] = std::make_unique<TreeNode>();
                    node->sub[1]->hash = hash;
                    node = &*node->sub[0];
                }
            }
            // Cannot turn a known inner node into a leaf.
            if (node->sub[0]) return std::nullopt;
            node->explored = true;
            node->inner = false;
            node->leaf = &key;
            node->hash = leaf_hash;
        }
    }

    // Recursive processing to turn the tree into flattened output. Use an explicit stack here to avoid
    // overflowing the call stack (the tree may be 128 levels deep).
    std::vector<TreeNode*> stack{&root};
    while (!stack.empty()) {
        TreeNode& node = *stack.back();
        if (!node.explored) {
            // Unexplored node, which means the tree is incomplete.
            return std::nullopt;
        } else if (!node.inner) {
            // Leaf node; produce output.
            ret.emplace_back(stack.size() - 1, node.leaf->first, node.leaf->second);
            node.done = true;
            stack.pop_back();
        } else if (node.sub[0]->done && !node.sub[1]->done && !node.sub[1]->explored && !node.sub[1]->hash.IsNull() &&
                   ComputeTapbranchHash(node.sub[1]->hash, node.sub[1]->hash) == node.hash) {
            // Whenever there are nodes with two identical subtrees under it, we run into a problem:
            // the control blocks for the leaves underneath those will be identical as well, and thus
            // they will all be matched to the same path in the tree. The result is that at the location
            // where the duplicate occurred, the left child will contain a normal tree that can be explored
            // and processed, but the right one will remain unexplored.
            //
            // This situation can be detected, by encountering an inner node with unexplored right subtree
            // with known hash, and H_TapBranch(hash, hash) is equal to the parent node (this node)'s hash.
            //
            // To deal with this, simply process the left tree a second time (set its done flag to false;
            // noting that the done flag of its children have already been set to false after processing
            // those). To avoid ending up in an infinite loop, set the done flag of the right (unexplored)
            // subtree to true.
            node.sub[0]->done = false;
            node.sub[1]->done = true;
        } else if (node.sub[0]->done && node.sub[1]->done) {
            // An internal node which we're finished with.
            node.sub[0]->done = false;
            node.sub[1]->done = false;
            node.done = true;
            stack.pop_back();
        } else if (!node.sub[0]->done) {
            // An internal node whose left branch hasn't been processed yet. Do so first.
            stack.push_back(&*node.sub[0]);
        } else if (!node.sub[1]->done) {
            // An internal node whose right branch hasn't been processed yet. Do so first.
            stack.push_back(&*node.sub[1]);
        }
    }

    return ret;
}

std::vector<std::tuple<uint8_t, uint8_t, std::vector<unsigned char>>> TaprootBuilder::GetTreeTuples() const
{
    assert(IsComplete());
    std::vector<std::tuple<uint8_t, uint8_t, std::vector<unsigned char>>> tuples;
    if (m_branch.size()) {
        const auto& leaves = m_branch[0]->leaves;
        for (const auto& leaf : leaves) {
            assert(leaf.merkle_branch.size() <= TAPROOT_CONTROL_MAX_NODE_COUNT);
            uint8_t depth = (uint8_t)leaf.merkle_branch.size();
            uint8_t leaf_ver = (uint8_t)leaf.leaf_version;
            tuples.emplace_back(depth, leaf_ver, leaf.script);
        }
    }
    return tuples;
}
