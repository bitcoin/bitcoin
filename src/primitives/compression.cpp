#include <primitives/compression.h>
#include <iostream>

CCompressedOutPoint::CCompressedOutPoint(const uint32_t block_height, const uint32_t block_index) : m_block_height(block_height), m_block_index(m_block_height > 0 ? block_index : 0), m_vout(0) {};

CCompressedOutPoint::CCompressedOutPoint(const uint256& txid, const uint32_t vout) : m_block_height(0), m_block_index(0), m_txid(txid), m_vout(vout) {};

std::string CCompressedOutPoint::ToString() const
{
    return strprintf("CCompressedOutPoint(block_height=%u, block_index=%u, txid=%s, vout=%u)",
    m_block_height,
    m_block_index,
    HexStr(m_txid),
    m_vout);
}

bool CompressLegacySignature(std::vector<unsigned char>& signature) {
    size_t length = signature.size();
    secp256k1_ecdsa_signature sig;
    if (secp256k1_ecdsa_signature_parse_der(secp256k1_context_static, &sig, &signature[0], length)) {
        signature.resize(64);
        if (secp256k1_ecdsa_signature_serialize_compact(secp256k1_context_static, &signature[0], &sig)) {
            return true;
        }
    }
    return false;
}

CCompressedTxIn::CCompressedTxIn(const CTxIn& txin, const CCompressedOutPoint& out, const CScript& script_pubkey) : m_prevout(out), m_compressedSignature(false), m_hashType(0), m_isHashStandard(false), m_nSequence(txin.nSequence) {
    if (!txin.scriptSig.empty() || !txin.scriptWitness.stack.empty()) {
        //Use Solver to get script_type but ignore solutions vector, I can split out the Solver code into a subfunction so I don't waste this solutions vector.
        std::vector<std::vector<unsigned char>> solutions;
        TxoutType script_type = Solver(script_pubkey, solutions);
        if (script_type == TxoutType::WITNESS_V1_TAPROOT) {
            if (txin.scriptWitness.stack.size() == 1 && (txin.scriptWitness.stack[0].size() == 64 || txin.scriptWitness.stack[0].size() == 65)) {
                m_signature = txin.scriptWitness.stack[0];
                if (m_signature.size() == 65) {
                    m_hashType = m_signature[m_signature.size()-1];
                    m_isHashStandard = m_hashType == 0x00;
                    m_signature.pop_back();
                } else m_isHashStandard = true;
                m_compressedSignature = true;
            }
        } else {
            if (script_type == TxoutType::PUBKEYHASH) {
                opcodetype opcodeRet;
                CScriptBase::const_iterator pc = txin.scriptSig.begin();
                txin.scriptSig.GetOp(pc, opcodeRet, m_signature);
                m_compressedSignature = true;
            } else if (script_type == TxoutType::WITNESS_V0_KEYHASH) {
                m_signature = txin.scriptWitness.stack[0];
                m_compressedSignature = true;
            } else if (script_type == TxoutType::SCRIPTHASH) {
                if (!txin.scriptWitness.stack.empty()) {
                    opcodetype opcodeRet;
                    CScriptBase::const_iterator pc = txin.scriptSig.begin();
                    std::vector<unsigned char> vec;
                    txin.scriptSig.GetOp(pc, opcodeRet, vec);
                    CScript redeemScript = CScript(vec.begin(), vec.end());
                    if (redeemScript.IsPayToWitnessKeyHash()) {
                        std::vector<std::vector<unsigned char>> solutions;
                        Solver(redeemScript, solutions);
                        m_pubkeyHash = solutions[0];
                        m_signature = txin.scriptWitness.stack[0];
                        m_compressedSignature = true;
                    }
                }
            }

            if (m_compressedSignature) {
                uint8_t hashType = m_signature[m_signature.size()-1];
                m_signature.pop_back();
                m_compressedSignature = CompressLegacySignature(m_signature);
                if (m_compressedSignature) {
                    if (hashType == 0x01) {
                        m_isHashStandard = true;
                    } else {
                        m_isHashStandard = false;
                        m_hashType = hashType;
                    }
                } else {
                    m_signature.clear();
                    m_warning = "Signature could not be compressed";
                }
            }
        }

        if (!m_compressedSignature) {
            DataStream stream;
            stream << VARINT(txin.scriptSig.size());
            if (!txin.scriptSig.empty())
                stream.write(MakeByteSpan(txin.scriptSig));
            stream << VARINT(txin.scriptWitness.stack.size());
            for (std::vector<unsigned char> stackitem : txin.scriptWitness.stack) {
                stream << VARINT(stackitem.size());
                stream.write(MakeByteSpan(stackitem));
            }
            m_signature.resize(stream.size());
            stream.read(MakeWritableByteSpan(m_signature));
        }
    }
}

std::string CCompressedTxIn::ToString() const
{
    return strprintf("CCompressedTxIn(prevout=%s, signature=%s, compressed=%b, hashType=%u, isHashStandard=%b, nSequence=%u, warning=%s)",
    m_prevout.ToString(),
    HexStr(m_signature),
    m_compressedSignature,
    m_hashType,
    m_isHashStandard,
    m_nSequence,
    m_warning);
}

CCompressedTxOut::CCompressedTxOut(const CTxOut& txout) : m_nValue(txout.nValue) {
    //Uses Solver to parse a script into a TxoutType and a byte array containing the raw script without prefix or suffix. Script gets reassembled with BuildDestination
    std::vector<std::vector<unsigned char>> solutions;
    m_scriptType = Solver(txout.scriptPubKey, solutions);
    switch (m_scriptType) {
        case TxoutType::PUBKEY:
        case TxoutType::PUBKEYHASH:
        case TxoutType::SCRIPTHASH:
        case TxoutType::WITNESS_V0_KEYHASH:
        case TxoutType::WITNESS_V0_SCRIPTHASH:
        case TxoutType::WITNESS_V1_TAPROOT:
            assert(solutions.size() == 1);
            m_scriptPubKey = solutions[0];
            break;
        default:
            m_scriptType = TxoutType::NONSTANDARD;
            m_scriptPubKey.resize(txout.scriptPubKey.size());
            copy(txout.scriptPubKey.begin(), txout.scriptPubKey.end(), m_scriptPubKey.begin());
    }
}

std::string CCompressedTxOut::ToString() const
{
    return strprintf("CCompressedTxOut(scriptPubKey=%s, scriptType=%s, nValue=%u)",
    HexStr(m_scriptPubKey),
    GetTxnOutputType(m_scriptType),
    m_nValue);
}

CCompressedTransaction::CCompressedTransaction(const CTransaction tx, const uint32_t& minimumHeight, const std::vector<CCompressedInput>& cinputs) : m_minimumHeight(minimumHeight), m_nVersion(tx.nVersion), m_nLockTime(tx.nLockTime ? tx.nLockTime-minimumHeight : 0) {
    assert(tx.vin.size() == cinputs.size());

    for (auto const& txout : tx.vout) {
        m_vout.emplace_back(txout);
    }

    for (size_t index = 0; index < tx.vin.size(); index++) {
        m_vin.emplace_back(tx.vin[index], cinputs[index].outpoint(), cinputs[index].scriptPubKey());
    }
}

std::string CCompressedTransaction::ToString() const
{
    std::string str;
    str += strprintf("CCompressedTransaction(nVersion=%u, nLockTime=%u, minimumHeight=%u)\n",
    m_nVersion,
    m_nLockTime,
    m_minimumHeight);
    for (const auto& txin : m_vin)
        str += "    " + txin.ToString() + "\n";
    for (const auto& txout : m_vout)
        str += "    " + txout.ToString() + "\n";
    return str;
}

CTxOut::CTxOut(const CCompressedTxOut& txout) {
    if (txout.IsCompressed()) {
        if (txout.scriptType() == TxoutType::PUBKEY) {
            scriptPubKey << txout.scriptPubKey();
            scriptPubKey << opcodetype::OP_CHECKSIG;
        } else {
            std::vector<std::vector<unsigned char>> vSolutions;
            vSolutions.push_back(txout.scriptPubKey());
            CTxDestination destination;
            if(!BuildDestination(vSolutions, txout.scriptType(), destination)) throw std::ios_base::failure("Script Deseralization Failed, Either Invalid Payload or Script Type.");
            scriptPubKey = GetScriptForDestination(destination);
        }
    } else {
        scriptPubKey = CScript(txout.scriptPubKey().begin(), txout.scriptPubKey().end());
    }
    nValue = txout.nValue();
}

std::string CCompressedInput::ToString() const
{
    return strprintf("CCompressedInput(prevout=%s, scriptPubKey=%s)",
    m_outpoint.ToString(),
    HexStr(m_scriptPubKey));
}


CCompressedInput::CCompressedInput(const CCompressedOutPoint& outpoint, const CScript& scriptPubKey) : m_outpoint(outpoint), m_scriptPubKey(scriptPubKey) {};

CMutableTransaction::CMutableTransaction(const CCompressedTransaction& tx, const std::vector<COutPoint>& prevouts, const std::vector<CTxOut>& outs) : nVersion(tx.nVersion()), nLockTime(tx.nLockTime()) {
    assert(outs.size() == tx.vin().size() && prevouts.size() == tx.vin().size());
    if (tx.nLockTime()) nLockTime += tx.minimumHeight();
    for (const auto& txout : tx.vout()) {
        vout.emplace_back(txout);
    }
    for (size_t index = 0; index < tx.vin().size(); index++) {
        vin.emplace_back(prevouts[index], CScript(), tx.vin()[index].nSequence());
    }

    for (size_t index = 0; index < tx.vin().size(); index++) {
        const CCompressedTxIn& input = tx.vin()[index];
        const CTxOut& output = outs[index];
        if (input.compressedSignature()) {
            if (output.scriptPubKey.IsPayToPublicKeyHash()) {
                uint8_t hash_type = input.isHashStandard() ? 0x01 : input.hashType();
                uint256 hash = SignatureHash(output.scriptPubKey, *this, index, hash_type, output.nValue, SigVersion::BASE);
                std::vector<unsigned char> vch_sig(CPubKey::COMPACT_SIGNATURE_SIZE);
                copy(input.signature().begin(), input.signature().end(), vch_sig.begin()+1);
                for (int recoveryId = 4; recoveryId < 8; recoveryId++) {
                    vch_sig[0] = recoveryId+27;
                    CPubKey pub;
                    if (pub.RecoverCompact(hash, vch_sig)) {
                        CScript script_pubkey = GetScriptForDestination(PKHash(pub));
                        if (script_pubkey != output.scriptPubKey) {
                            pub.Decompress();
                            script_pubkey = GetScriptForDestination(PKHash(pub));
                        }
                        if (script_pubkey == output.scriptPubKey) {
                            secp256k1_ecdsa_signature sig;
                            secp256k1_ecdsa_signature_parse_compact(secp256k1_context_static, &sig, &input.signature()[0]);
                            size_t sig_size = 71;
                            std::vector<unsigned char> sig_vec(sig_size);
                            secp256k1_ecdsa_signature_serialize_der(secp256k1_context_static, &sig_vec[0], &sig_size, &sig);
                            sig_vec.resize(sig_size+1);
                            sig_vec[sig_size] = hash_type;

                            size_t p_size = pub.size();
                            std::vector<unsigned char> p_vec(p_size);
                            copy(pub.begin(), pub.end(), p_vec.begin());
                            vin[index].scriptSig << sig_vec;
                            vin[index].scriptSig << p_vec;
                        }
                    }
                }
            } else if (output.scriptPubKey.IsPayToWitnessKeyHash()) {
                uint8_t hash_type = input.isHashStandard() ? 0x01 : input.hashType();
                std::vector<std::vector<unsigned char>> vSolutions;
                Solver(output.scriptPubKey, vSolutions);
                CTxDestination destination;
                BuildDestination(vSolutions, TxoutType::PUBKEYHASH, destination);
                CScript scriptCode = GetScriptForDestination(destination);
                uint256 hash = SignatureHash(scriptCode, *this, index, hash_type, output.nValue, SigVersion::WITNESS_V0);
                std::vector<unsigned char> vch_sig(CPubKey::COMPACT_SIGNATURE_SIZE);
                copy(input.signature().begin(), input.signature().end(), vch_sig.begin()+1);
                for (int recoveryId = 4; recoveryId < 8; recoveryId++) {
                    vch_sig[0] = recoveryId+27;
                    CPubKey pub;
                    if (pub.RecoverCompact(hash, vch_sig)) {
                        CScript script_pubkey = GetScriptForDestination(WitnessV0KeyHash(pub));
                        if (output.scriptPubKey == script_pubkey) {
                            secp256k1_ecdsa_signature sig;
                            secp256k1_ecdsa_signature_parse_compact(secp256k1_context_static, &sig, &input.signature()[0]);
                            size_t sig_size = 71;
                            std::vector<unsigned char> sig_vec(sig_size);
                            secp256k1_ecdsa_signature_serialize_der(secp256k1_context_static, &sig_vec[0], &sig_size, &sig);
                            sig_vec.resize(sig_size+1);
                            sig_vec[sig_size] = hash_type;

                            size_t p_size = pub.size();
                            std::vector<unsigned char> p_vec(p_size);
                            copy(pub.begin(), pub.end(), p_vec.begin());

                            std::vector<std::vector<unsigned char>> stack;
                            stack.push_back(sig_vec);
                            stack.push_back(p_vec);
                            vin[index].scriptWitness.stack = stack;
                        }
                    }
                }
            } else if (output.scriptPubKey.IsPayToScriptHash()) {
                uint8_t hash_type = input.isHashStandard() ? 0x01 : input.hashType();
                std::vector<std::vector<unsigned char>> vSolutions;
                vSolutions.push_back(input.pubkeyHash());
                CTxDestination destination;
                BuildDestination(vSolutions, TxoutType::PUBKEYHASH, destination);
                CScript scriptCode = GetScriptForDestination(destination);
                uint256 hash = SignatureHash(scriptCode, *this, index, hash_type, output.nValue, SigVersion::WITNESS_V0);
                std::vector<unsigned char> vch_sig(CPubKey::COMPACT_SIGNATURE_SIZE);
                copy(input.signature().begin(), input.signature().end(), vch_sig.begin()+1);
                for (int recoveryId = 4; recoveryId < 8; recoveryId++) {
                    vch_sig[0] = recoveryId+27;
                    CPubKey pub;
                    if (pub.RecoverCompact(hash, vch_sig)) {
                        CScript script_pubkey = GetScriptForDestination(ScriptHash(GetScriptForDestination(WitnessV0KeyHash(pub))));

                        if (output.scriptPubKey == script_pubkey) {
                            secp256k1_ecdsa_signature sig;
                            secp256k1_ecdsa_signature_parse_compact(secp256k1_context_static, &sig, &input.signature()[0]);
                            size_t sig_size = 71;
                            std::vector<unsigned char> sig_vec(sig_size);
                            secp256k1_ecdsa_signature_serialize_der(secp256k1_context_static, &sig_vec[0], &sig_size, &sig);
                            sig_vec.resize(sig_size+1);
                            sig_vec[sig_size] = hash_type;

                            size_t p_size = pub.size();
                            std::vector<unsigned char> p_vec(p_size);
                            copy(pub.begin(), pub.end(), p_vec.begin());

                            CScript redeemScript = GetScriptForDestination(WitnessV0KeyHash(pub));
                            std::vector<unsigned char> redeemVec(redeemScript.size());
                            copy(redeemScript.begin(), redeemScript.end(), redeemVec.begin());
                            vin[index].scriptSig << redeemVec;

                            std::vector<std::vector<unsigned char>> stack;
                            stack.push_back(sig_vec);
                            stack.push_back(p_vec);
                            vin[index].scriptWitness.stack = stack;
                        }
                    }
                }
            } else if (output.scriptPubKey.IsPayToTaproot()) {
                std::vector<std::vector<unsigned char>> stack;
                stack.push_back(input.signature());
                uint8_t hash_type = input.isHashStandard() ? 0 : input.hashType();
                if (hash_type) stack[0].push_back(input.hashType());
                vin[index].scriptWitness.stack = stack;
            } else {
                throw std::ios_base::failure("A compressed signature is not supported for this script type");
            }
            if (vin[index].scriptSig.empty() && vin[index].scriptWitness.stack.empty()) throw std::ios_base::failure("Malformed Signature For Input");
        } else if (!input.signature().empty()) {
            DataStream stream;
            stream.write(MakeByteSpan(input.signature()));
            uint64_t scriptSigLength = std::numeric_limits<uint64_t>::max();
            stream >> VARINT(scriptSigLength);
            if (scriptSigLength >= MAX_SCRIPT_SIZE) throw std::ios_base::failure("Script Signature Exceeds MAX_SCRIPT_SIZE");
            if (scriptSigLength) {
                std::vector<unsigned char> scriptSigBytes(scriptSigLength);
                stream.read(MakeWritableByteSpan(scriptSigBytes));
                vin[index].scriptSig = CScript(scriptSigBytes.begin(), scriptSigBytes.end());
            }
            uint64_t witnessCount = std::numeric_limits<uint64_t>::max();
            stream >> VARINT(witnessCount);
            std::vector<std::vector<unsigned char>> stack;
            for (uint64_t i = 0; i < witnessCount; i++) {
                uint64_t witnessLength = std::numeric_limits<uint64_t>::max();
                stream >> VARINT(witnessLength);
                std::vector<unsigned char> witness(witnessLength);
                stream.read(MakeWritableByteSpan(witness));
                stack.push_back(witness);
            }
            vin[index].scriptWitness.stack = stack;
        }
    }
}
