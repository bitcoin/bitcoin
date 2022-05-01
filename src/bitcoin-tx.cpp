// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <clientversion.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <core_io.h>
#include <key_io.h>
#include <fs.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <univalue.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/system.h>
#include <util/translation.h>

#include <functional>
#include <memory>
#include <stdio.h>

#include "jurat.hpp"
#include <core_io.h>
#include <iostream>
#include <fstream>

#include <boost/algorithm/string.hpp>

static bool fCreateBlank;
static std::map<std::string,UniValue> registers;
static const int CONTINUE_EXECUTION=-1;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

std::string JURAT_CREATE_TX = "juratCreateTx";
std::string JURAT_SIGN_TX = "juratSignTx";
std::string JURAT_GENERIC_CMD = "juratGenricCmd";

const std::string keyJudicialAction = "judicialAction";
const std::string keyNonce = "nonce";
const std::string keySrcAddr = "srcAddr";
const std::string keyTargetAddr = "targetAddr";
const std::string keyTxId = "txid";
const std::string keyVout = "vout";
const std::string keyFreezeAmt = "amount";
const std::string keyTotalAmt = "availableAmount";
const std::string keyFeeAmt = "feeAmt";
const std::string keyLockHeight = "lockHeight";

const std::string keyJuratWitnessSig = "jwsig";
const std::string keySigs = "sigs";
const std::string keyTxHex = "txHex";
const std::string keyPrivKeys = "privkeys";

const std::string keyJuratDataHash = "juratDataHash";

using namespace jurat;
CAmount FEE = 300;

///regtes keys (addr, pubkey, privkey)
//(bcrt1qef4q448pt74s6wl9xd7d6qu5k67hf4p8ehg04c,  024a5d2b6ceb5d8291b6d97fdec2de4b50024d981c4a13f31673c0bd8bc493f70c, cPSTKWLXx7aoCsVcCNgzafAMRx2wVp2mmg5PNPrNE4Ef9sCJdbCu
//)
//
//(bcrt1qqf7n7axwxml4ye2yk80p9vyuh38sa9vvj7l3q6, 02b5efb6fa70adbe1aa9a70347699ed6948a660bf8eca9826cb4824b644a1484bb,
//cVWYLhVS2ppfQDVmvPgppw84Wo69gkBqP7Z3g8HFcRHFNZF1rAg9
//)
//
//(bcrt1qef4q448pt74s6wl9xd7d6qu5k67hf4p8ehg04c, 024a5d2b6ceb5d8291b6d97fdec2de4b50024d981c4a13f31673c0bd8bc493f70c,
//cVWYLhVS2ppfQDVmvPgppw84Wo69gkBqP7Z3g8HFcRHFNZF1rAg9
//)
//
//(mqHU8cja5UQh22biGnrrA4rnuDzLaMWhmj, 03432e525e798fb76f96b0596fc5e6bd4b78fb628b2debece5be146b9e467d7f1f,
//cS55U7eMUB7rXfPa5xPyGEacUtenbU346keSv8Js3rJLLQEVBH2M
//)



const std::string judicialVerifierPvtKeys[] =
    {  "cPSTKWLXx7aoCsVcCNgzafAMRx2wVp2mmg5PNPrNE4Ef9sCJdbCu",
        "cVWYLhVS2ppfQDVmvPgppw84Wo69gkBqP7Z3g8HFcRHFNZF1rAg9"
    };


static const std::string getJuratSigPrivateKey() {
    ///same private key in mainnet compatible format and testnet compatible format
    const std::string mainnetFormatPrivKey = "KypcjQrjKKfGN6gf6HYLa2Qn9T7mKR4N3zdqHUoPQmRTNiTCUBaz";
    const std::string testnetFormatPrivKey = "cQBcCKrakPMXXY9vUhMTwLuqmgRAysA482nJPuFtut5TdTU4TXjT";
    if(Params().IsTestChain()){
        return testnetFormatPrivKey;
    }else{
        return mainnetFormatPrivKey;
    }
    
}


CTransaction juratSign(CTransaction& tx, std::string& srcAddr,
                       std::string& targetAddr, int nIn, int nOut,
                       CAmount satoshisTxIn, CAmount satoshisFrozenLessFee,
                       int nLockBlockHeight, const std::string judicialVerifierPvtKeys[],
                       bool& isSigned)

{
    
    isSigned = false;
    if(nIn >=  (int)tx.vin.size() || nOut >= (int)tx.vout.size())
    {
        //invalid nIn or nOut
        return tx;
    }
    
    ///check addr
    std::string err;
    std::vector<unsigned char> srcAddrPKHash;
    CScript srcScriptPubKey;
    std::vector<unsigned char> targetAddrPKHash;
    CScript targetScriptPubKey;
    if(!checkAddr(srcAddr, srcAddrPKHash, srcScriptPubKey, err) ||
       !checkAddr(targetAddr, targetAddrPKHash, targetScriptPubKey, err))
    {
        std::cout << "invalid src or target addr: " + err;
        return tx;
    }
    
    ///jurat sig data
    ///<hash160(from_addr_pk) 20 byte> <hash160(to_addr_pk) 20 byte><amount 4 byte> <lock block height 4 byte>
    CHashWriter ss(SER_GETHASH, 0);
    ss << srcAddrPKHash;
    ss << targetAddrPKHash;
    ss << satoshisFrozenLessFee;
    ss << nLockBlockHeight;
    std::cout << "[signer] sig-data srcAddr=" << HexStr(targetAddrPKHash) << std::endl;
    std::cout << "[signer] sig-data targetAddr=" << HexStr(targetAddrPKHash) << std::endl;
    std::cout << "[signer] sig-data satoshisFrozenLessFee=" << satoshisFrozenLessFee << std::endl;
    std::cout << "[signer] sig-data nLockBlockHeight=" << nLockBlockHeight << std::endl;
    
    uint256 juratSigDataHash = ss.GetHash();
    std::cout << "[signer] sig-data hash=" << HexStr(juratSigDataHash) << std::endl;
    
    CMutableTransaction mutableTx(tx);
    mutableTx.vin[nIn].scriptWitness.stack.clear();
    std::vector<unsigned char> emptyChunk;
    mutableTx.vin[nIn].scriptWitness.stack.push_back(emptyChunk);
    std::vector<std::vector<unsigned char>> signingPubKeys;
    CScript redeemScript = getMultiSigRedeemScript();
    
    uint256 sigData = SignatureHash(redeemScript, mutableTx, nIn, SIGHASH_ALL,satoshisTxIn, SigVersion::WITNESS_V0);
    std::cout <<"[signer] sigDataHash=" << HexStr(sigData);
    
    for(int i=0;i<(int)judicialVerifierPubKeys.size();i++){
        std::vector<unsigned char> vchJuratSig;
        CKey privateKey = DecodeSecret(judicialVerifierPvtKeys[(int)i]);
        
        if(privateKey.Sign(juratSigDataHash,vchJuratSig))
        {
            vchJuratSig.push_back(i);//judicial verifier index
            mutableTx.vin[nIn].scriptWitness.stack.push_back(vchJuratSig);
        }else
        {
            isSigned = false;
            return tx;
        }
        
        CPubKey pubKey = privateKey.GetPubKey();
        std::vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
        //CScript scriptPubKey = GetScriptForDestination(PKHash(pubKey.GetID()));
        std::vector<unsigned char> vchSig;
        if(privateKey.Sign(sigData, vchSig))
        {
            vchSig.push_back(SIGHASH_ALL);
            mutableTx.vin[nIn].scriptWitness.stack.push_back(vchSig);
            signingPubKeys.insert(signingPubKeys.begin(),vchPubKey);
            std::cout << "[signer] sig=" << HexStr(vchSig) << std::endl;
            std::cout << "[signer] signed by pubkey=" << HexStr(vchPubKey) << std::endl;
        } else
        {
            isSigned = false;
            return tx;
        }
    }
    
    if(signingPubKeys.size()==0){
        isSigned = false;
        return tx;
    }
    
    std::vector<unsigned char> vchscriptData(redeemScript.begin(), redeemScript.end());
    mutableTx.vin[nIn].scriptWitness.stack.push_back(vchscriptData);
    isSigned = true;
    return CTransaction(mutableTx);
}


bool freezeTx(enumJurat action,
              std::string& srcAddr, std::string targetAddr,
              std::string& txId, int nOutToFreeze,
              CAmount& satoshiTxInTotalAmount,
              CAmount& satoshiFreezeAmt,
              CAmount& satoshiFeeAmt,
              uint32_t nLockBlockHeight,
              uint32_t nonce,
              CMutableTransaction& freezeTx,
              UniValue& out)
{
    
    std::cout << "create freeze tx for src addr " << srcAddr << std::endl;
    std::vector<unsigned char> vchSrcPKHash;
    CScript srcScriptPubKey;
    std::string err;
    std::vector<unsigned char> vchTargetPKHash;
    CScript targetScriptPubKey;
    
    if (!checkAddr(srcAddr, vchSrcPKHash,srcScriptPubKey, err) ||
        !checkAddr(targetAddr, vchTargetPKHash,targetScriptPubKey, err) )
    {
        tfm::format(std::cout,"invalid address: ",err);
        out.pushKV("error", err);
        return false;
    }
    
    
    if (satoshiFreezeAmt > satoshiTxInTotalAmount) {
        err = tfm::format("freeze amount %d is greater than fund available %d",satoshiFreezeAmt, satoshiTxInTotalAmount);
        out.pushKV("error", err);
        return false;
    }
    
    std::cout << "lock to pub key hash " << HexStr(vchTargetPKHash) << std::endl;
    CScript lockingScript;
    CScript sendToLockScriptPubKey;
    createLockingScript(nLockBlockHeight, vchTargetPKHash, lockingScript,
                        sendToLockScriptPubKey);
    CMutableTransaction mutableTx;
    uint256 prevTxHash;
    ParseHashStr(txId, prevTxHash);
    CTxIn txIn(prevTxHash, nOutToFreeze);
    txIn.nSequence = CTxIn::SEQUENCE_FINAL;
    mutableTx.vin.push_back(txIn);
    CAmount satoshisFrozenLessFee = satoshiFreezeAmt-satoshiFeeAmt;
    CTxOut txOutToFreeze(satoshisFrozenLessFee, sendToLockScriptPubKey);
    mutableTx.vout.push_back(txOutToFreeze);
    if ( satoshiTxInTotalAmount > satoshiFreezeAmt) {
        CTxOut txOutReturnChange(satoshiTxInTotalAmount -
                                 satoshiFreezeAmt, srcScriptPubKey);
        mutableTx.vout.push_back(txOutReturnChange);
    }
    
    //attach jurat marker
    CScript markerScript;
    createJuratMarker(action,0,0,nLockBlockHeight,nonce,vchTargetPKHash
                      ,markerScript);
    CTxOut txOutMarker(0, markerScript);
    mutableTx.vout.push_back(txOutMarker);
    
    CTransaction tx(mutableTx);
    freezeTx = mutableTx;
    std::string txHex = EncodeHexTx(tx);
    
    out.pushKV("txHex", txHex);
    
    CScript redeemScript = getMultiSigRedeemScript();
    uint256 sigData = SignatureHash(redeemScript, mutableTx, 0, SIGHASH_ALL,satoshiTxInTotalAmount, SigVersion::WITNESS_V0);
    out.pushKV("sigDataHash", HexStr(sigData));
    
    CHashWriter ss(SER_GETHASH, 0);
    ss << vchSrcPKHash;
    ss << vchTargetPKHash;
    ss << satoshisFrozenLessFee;
    ss << nLockBlockHeight;
    ss << nonce;
    uint256 juratSigDataHash = ss.GetHash();
    out.pushKV(keyJuratDataHash, HexStr(juratSigDataHash));
    
    jurat::JuratVerifier verifier(tx, srcScriptPubKey);
    if(!verifier.isJuratTx())
        return false;
    
    return true;
}

static int findSignatureIndex(std::vector<unsigned char> vchSignature,
                              std::vector<unsigned char> vchJuratDataHash){
    
    uint256 signedHash(vchJuratDataHash);
    for(int i=0; i< (int)judicialVerifierPubKeys.size();i++){
        std::vector<unsigned char> vchPubKey = ParseHex(judicialVerifierPubKeys[i]);
        CPubKey pubKey(vchPubKey.begin(), vchPubKey.end());
        
        bool isVerified = pubKey.Verify(signedHash, vchSignature);
        if(isVerified){
            return i;
        }
        
    }
    return -1;
    
}

static bool attachWitness(CMutableTransaction& mutableTx,
                          int nInToSign, CAmount& satoshisTxIn,
                          std::vector<std::string>& jsigs,
                          UniValue& out)
{
    std::string err = "";
    if (nInToSign >= (int)mutableTx.vin.size() ) {
        err = tfm::format("vin [%d] being signed is out of bound vin.size = %d", nInToSign, (int)mutableTx.vin.size());
        out.pushKV("error", err);
        return false;
    }
    
    if (jsigs.size() ==0 ){
        out.pushKV("error", "jurat witness signature array is empty");
        return false;
    }
    
    std::vector<unsigned char> vchJuratDataHash = ParseHex(out[keyJuratDataHash].get_str());
    CScriptWitness witness;
    std::vector<unsigned char> emptyChunk;
    witness.stack.push_back(emptyChunk);
    bool signatureCheck = true;
    for(int i=0;i<(int)jsigs.size();i++){
        
        if(IsHex(jsigs[i])) {
            std::vector<unsigned char> vchSignature = ParseHex(jsigs[i]);
            int verifierIndx = findSignatureIndex(vchSignature, vchJuratDataHash);
            if(verifierIndx >=0){
                vchSignature.push_back((unsigned char)verifierIndx);
                tfm::format(std::cout, "witness push hex signature => %s\n",jsigs[i]);
                witness.stack.push_back(vchSignature);
            }else{
                std::string str = tfm::format("could not verify witness signature with any of the witness pub keys ..%s",jsigs[i]);
                err = err + "; " + str;
            }
            
        } else {
            //try base64 decoding
            bool isInvalid = false;
            std::vector<unsigned char> vchSignature = DecodeBase64(jsigs[i].c_str(),&isInvalid);
            if(isInvalid){
                signatureCheck = false;
                std::string str = tfm::format("witness signature is not a valid base64 encoded string ..%s",jsigs[i]);
                err = err + "; " + str;
                continue;
            }else{
                int verifierIndx = findSignatureIndex(vchSignature, vchJuratDataHash);
                if(verifierIndx >=0){
                    vchSignature.push_back((unsigned char)verifierIndx);
                    witness.stack.push_back(vchSignature);
                    tfm::format(std::cout, "witness push base64 decoded signature  %s => %s\n",jsigs[i],HexStr(vchSignature));
                }else{
                    std::string str = tfm::format("could not verify witness signature with any of the witness pub keys ..%s",jsigs[i]);
                    err = err + "; " + str;
                }
                
            }
        }
    }
    
    if(!signatureCheck){
        out.pushKV("error", err);
        return false;
    }
    
    CScript redeemScript = getMultiSigRedeemScript();
    uint256 sigData = SignatureHash(redeemScript, mutableTx, nInToSign, SIGHASH_ALL,satoshisTxIn, SigVersion::WITNESS_V0);
    std::cout <<"[signer] sigDataHash=" << HexStr(sigData);
    std::vector<unsigned char> vchSig;
    CKey privateKey = DecodeSecret(getJuratSigPrivateKey());
    if(privateKey.Sign(sigData, vchSig))
    {
        vchSig.push_back(SIGHASH_ALL);

        witness.stack.push_back(vchSig);
        std::cout << "[signer] sig=" << HexStr(vchSig) << std::endl;
        std::cout << "[signer] signed by pubkey=" << HexStr(bitcoinSignPubKey) << std::endl;
    } else{
        out.pushKV("error", "failed to sign Tx");
        return false;
    }
    
    
    std::vector<unsigned char> vchscriptData(redeemScript.begin(), redeemScript.end());
    witness.stack.push_back(vchscriptData);
    
    mutableTx.vin[nInToSign].scriptWitness = witness;
    
    CTransaction tx(mutableTx);
    std::string txSignedHex = EncodeHexTx(tx);
    out.pushKV("signedTx", txSignedHex);
    return true;
}

static bool checkKeysForCreate(const UniValue& uniVal, UniValue& out)
{
    bool ret = true;
    std::string err = "";
    
    std::vector<std::string> strKeys;
    std::vector<std::string> numKeys;
    
    strKeys.push_back(keyJudicialAction);
    strKeys.push_back(keySrcAddr);
    strKeys.push_back(keyTargetAddr);
    strKeys.push_back(keyTxId);
    for(std::string key: strKeys){
        if(uniVal[key].isNull() || !uniVal[key].isStr() ) {
            err.append(key).append(": key is null or not a string; ");
            ret = false;
        }
    }
    
    numKeys.push_back(keyNonce);
    numKeys.push_back(keyVout);
    numKeys.push_back(keyTotalAmt);
    numKeys.push_back(keyFreezeAmt);
    numKeys.push_back(keyLockHeight);
    for(std::string key: numKeys){
        if(uniVal[key].isNull() || !uniVal[key].isNum() ) {
            err.append(key).append(": key is null or not a number; ");
            ret = false;
        }
    }
    
    if(!ret){
        out.pushKV("error",err);
    }
    
    return ret;
}

static bool getJuratActionCode(std::string& judicialAction, enumJurat& action, UniValue& out)
{
    bool ret = false;
    
    if (judicialAction == "freeze") {
        action =  enumJurat::JURAT_ACTION_FREEZE_WITH_TIMELOCK;
        ret = true;
    } else if(judicialAction == "transfer") {
        action = enumJurat::JURAT_ACTION_MOVE_FUND_WITH_TIMLOCK;
        ret = true;
    }else {
        out.pushKV("error", "invalid judicial action, can be freeze or transfer; ");
    }
    
    return ret;
}

static bool createJuratTx(const UniValue& uniVal, UniValue& out,
                          CMutableTransaction& mutableTx)
{
    if(!checkKeysForCreate(uniVal, out)) {
        return false;
    }
    
    std::string judicialAction = uniVal[keyJudicialAction].get_str();
    std::string srcAddr = uniVal[keySrcAddr].get_str();
    std::string targetAddr = uniVal[keyTargetAddr].get_str();
    std::string txid = uniVal[keyTxId].get_str();
    uint32_t nOutToFreeze = uniVal[keyVout].get_int();
    uint32_t nonce = uniVal[keyNonce].get_int();
    
    enumJurat action;
    bool isValidAction = getJuratActionCode(judicialAction, action, out);
    if(!isValidAction){
        return false;
    }
    
    CAmount satoshiTxInTotalAmount = (int64_t) (COIN* uniVal[keyTotalAmt].get_real());
    CAmount satoshiFreezeAmt = (int64_t)(COIN* uniVal[keyFreezeAmt].get_real());
    uint32_t nLockBlockHeight = uniVal[keyLockHeight].get_int();
    CAmount satoshiFee = FEE;
    if( !uniVal[keyFeeAmt].isNull()  &&  uniVal[keyLockHeight].isNum()) {
        satoshiFee = uniVal[keyFeeAmt].get_int();
    }
    
    
    
    bool isCreated = freezeTx(action,
                              srcAddr,
                              targetAddr,
                              txid,
                              nOutToFreeze,
                              satoshiTxInTotalAmount,
                              satoshiFreezeAmt,
                              satoshiFee,
                              nLockBlockHeight,
                              nonce,
                              mutableTx,
                              out);
   
    return isCreated;
}

static void printOutput(UniValue& out) {
    std::string json = out.write(4);
    std::cout << "begin_json_out:\n" << json << "\nend_json_out\n";
    std::cout.flush();
}

static void privateKeyFormats(std::string& base58CheckPrivateKey, bool isCompressed){
    
    tfm::format(std::cout,"[private key]: loading = %s\n",base58CheckPrivateKey);
    CKey privateKey = DecodeSecret(base58CheckPrivateKey);
    if(!privateKey.IsValid()){
        tfm::format(std::cout,"[private key]: failed to load = %s\n",base58CheckPrivateKey);
        return;
    }
    
    std::vector<unsigned char> vchKeyData(privateKey.begin(), privateKey.end());
    tfm::format(std::cout,"[private key]: loaded key hexStr = %s\n",HexStr(vchKeyData));
    CPubKey pubKey = privateKey.GetPubKey();
    std::vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
    tfm::format(std::cout,"[public key]: = %s\n",HexStr(vchPubKey));
    
    std::string net[] = {"mainnet", "testnet"};
    unsigned char mainnetPrefix = (unsigned char)0x80;
    unsigned char testnetPrefix = (unsigned char)0xef;
    unsigned char suffixChar = (unsigned char)0x01;
    unsigned char prefixChar[] = {mainnetPrefix,testnetPrefix};
    for(int i=0;i<2;i++){
        std::vector<unsigned char> vchBytes(privateKey.begin(), privateKey.end());
        
        ///step# 1
        vchBytes.insert(vchBytes.begin(), prefixChar[i]);
        
        ///step# 2
        if(isCompressed){
            vchBytes.push_back(suffixChar);
        }
        
        tfm::format(std::cout,"[%s]: private key with prefix+suffix = %s\n",net[i],HexStr(vchBytes));
        
        ///step# 3 get hash-256
        uint256 hash = Hash(vchBytes);
        tfm::format(std::cout,"[%s]: hash-256 = %s\n",net[i],HexStr(hash));
        
        ///step# 4 append 4 byte check sum [ taking first 4 bytes from step# 3
        vchBytes.insert(vchBytes.end(), hash.begin(), hash.begin()+4);
        tfm::format(std::cout,"[%s]: added 4 byte checksum %s\n",net[i],HexStr(vchBytes));
        
        /// step#5 base58 encode to get WIF [ wallet import format ] key
        std::string encodedKey = EncodeBase58(vchBytes);
        
        ///EncodeBase58Check can be called instead of steps #3 #4 and #5
        tfm::format(std::cout, "[%s]: private key wif compatible base58check = %s\n", net[i],encodedKey);
    }
    
}

static void genericJuratCmd(const UniValue& in, UniValue& out){
    
    {
        initWitnessKeys();
        const std::vector<unsigned char>& privkey_prefix = Params().Base58Prefix(CChainParams::SECRET_KEY);
        tfm::format(std::cout, "privkey_prefix [%s]\n", HexStr(privkey_prefix));
        
        std::string pvtKey = "";
    
        std::string strKey = "024a5d2b6ceb5d8291b6d97fdec2de4b50024d981c4a13f31673c0bd8bc493f70c";
        std::vector<unsigned char> vchPubKey =  ParseHex(strKey);
        CPubKey pk(vchPubKey.begin(), vchPubKey.end());
        uint256 hash(ParseHex("bd4cc33f0f756104824f6bb00130f91dcca549e69720358f13917ba3dddb5faf"));
        std::vector<unsigned char> vchSig = ParseHex("3044022046281f4663b816e472c785f29abc0e31954c1720fa265c1806c8e463b0cd75aa022072b0c2e55dce53e0472e1be64654494d0cab8ccd29c3e3a803e8831399880107");
        bool isverified = pk.Verify(hash, vchSig);
        tfm::format(std::cout, "is verified [%s]\n", isverified);
        
    }
    
    
    //hex string
    std::string juratDataHash = in[keyJuratDataHash].get_str();
    
    //hex str => char array
    std::vector<unsigned char> hash = ParseHex(juratDataHash);
    uint256 hash256 = uint256(hash);
    
    std::string testPrivKeyStr = "cQBcCKrakPMXXY9vUhMTwLuqmgRAysA482nJPuFtut5TdTU4TXjT";
    std::string mainPrivKeyStr = "KypcjQrjKKfGN6gf6HYLa2Qn9T7mKR4N3zdqHUoPQmRTNiTCUBaz";
    
    std::string testPubKeyStr = "03b8c53308a5ed31dea6733ee267c546872be4d2903100d5eabf4f154bed0b944d";
    std::string mainPubKeyStr = "03b8c53308a5ed31dea6733ee267c546872be4d2903100d5eabf4f154bed0b944d";
    
    std::vector<std::string> signingKeys;
    std::vector<std::string> verifierKeys;
    signingKeys.push_back(testPrivKeyStr);
    signingKeys.push_back(mainPrivKeyStr);
    verifierKeys.push_back(testPubKeyStr);
    verifierKeys.push_back(mainPubKeyStr);
    
    for(int i=0;i<(int)signingKeys.size();i++){
        std::vector<unsigned char> vchJuratSig;
        tfm::format(std::cout,"loading private key =%s\n",signingKeys[i]);
        
        CKey privateKey = DecodeSecret(signingKeys[i]);
        privateKeyFormats(signingKeys[i], privateKey.IsCompressed());
        if(!privateKey.IsValid()){
            tfm::format(std::cout,"failed to load key=%s\n",signingKeys[i]);
            continue;
        }
        bool isSigned = privateKey.Sign(hash256, vchJuratSig);
        if(isSigned) {
            tfm::format(std::cout, "sign success priv-key=%s hashHex=%s\n signatureHex=%s\n",
                        signingKeys[i], juratDataHash, HexStr(vchJuratSig));
            
        }else {
            tfm::format(std::cout,"failed to sign with key=%s\n",signingKeys[i]);
        }
        
        if (isSigned) {
            //verify
            std::string strVerifierPubKey = verifierKeys[i];
            std::vector<unsigned char> vchPubKey =  ParseHex(strVerifierPubKey);
            CPubKey verifierPubKey(vchPubKey.begin(), vchPubKey.end());
            std::vector<unsigned char> vchSignature(vchJuratSig.begin(), vchJuratSig.end());
            bool isVerified = verifierPubKey.Verify(hash256, vchSignature);
            tfm::format(std::cout, "signature verification pub-key=%s verified=%s\n",
                        verifierKeys[i], isVerified);
            
        }
    }
    std::cout.flush();
}

static bool runJuratCmd()
{

    
    if(registers.find(JURAT_CREATE_TX) != registers.end()) {
        const UniValue uniVal = registers[JURAT_CREATE_TX];
        UniValue out(UniValue::VOBJ);
        out.pushKV("request", uniVal);
        CMutableTransaction mutableTx;
        bool isCreated =  createJuratTx(uniVal, out, mutableTx);
        printOutput(out);
        return isCreated;
    }
    
    ECC_Start();
    ECCVerifyHandle verifyHandle;
    
    if(!ECC_InitSanityCheck()){
        tfm::format(std::cout,"ecc support not available at runtime");
        return false;
    }
    
    if(registers.find(JURAT_GENERIC_CMD) != registers.end()) {
        const UniValue uniVal = registers[JURAT_GENERIC_CMD];
        UniValue out(UniValue::VOBJ);
        out.pushKV("request", uniVal);
        genericJuratCmd(uniVal, out);
        return true;
    }
    
    
    if(registers.find(JURAT_SIGN_TX) != registers.end()) {
        const UniValue uniVal = registers[JURAT_SIGN_TX];
        UniValue out(UniValue::VOBJ);
        out.pushKV("request", uniVal);
        CMutableTransaction mutableTx;
        bool isCreated =  createJuratTx(uniVal, out, mutableTx);
        if(!isCreated){
            printOutput(out);
            return false;
        }
        
        if(uniVal[keyJuratWitnessSig].isNull() ||
           uniVal[keyJuratWitnessSig].getType() != UniValue::VType::VARR) {
            out.pushKV("error", "jwsig is null or not an array");
            printOutput(out);
            return false;
        }
        
        //attach witness
        std::vector<std::string> jsigs;
        for (const UniValue& univalJsig : uniVal[keyJuratWitnessSig].getValues()){
            std::string jwsig = univalJsig.get_str();
            jsigs.push_back(jwsig);
        }
        int nInToSign = 0;
        CAmount satoshiTxInTotalAmount = (int64_t) (COIN* uniVal[keyTotalAmt].get_real());
        bool isWitessAttached = attachWitness(mutableTx, nInToSign, satoshiTxInTotalAmount,jsigs, out);
        printOutput(out);
        return isWitessAttached;
    }
    
    ECC_Stop();
    return false;
}




static void SetupBitcoinTxArgs(ArgsManager &argsman)
{
    SetupHelpOptions(argsman);

    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-create", "Create new, empty TX.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-json", "Select JSON output", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-txid", "Output only the hex-encoded transaction id of the resultant transaction.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-jurat", "Create jurat tx from the json specified in the set= argument", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    
    SetupChainParamsBaseOptions(argsman);

    argsman.AddArg("delin=N", "Delete input N from TX", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("delout=N", "Delete output N from TX", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("in=TXID:VOUT(:SEQUENCE_NUMBER)", "Add input to TX", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("locktime=N", "Set TX lock time to N", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("nversion=N", "Set TX version to N", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("outaddr=VALUE:ADDRESS", "Add address-based output to TX", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("outdata=[VALUE:]DATA", "Add data-based output to TX", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("outmultisig=VALUE:REQUIRED:PUBKEYS:PUBKEY1:PUBKEY2:....[:FLAGS]", "Add Pay To n-of-m Multi-sig output to TX. n = REQUIRED, m = PUBKEYS. "
        "Optionally add the \"W\" flag to produce a pay-to-witness-script-hash output. "
        "Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash.", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("outpubkey=VALUE:PUBKEY[:FLAGS]", "Add pay-to-pubkey output to TX. "
        "Optionally add the \"W\" flag to produce a pay-to-witness-pubkey-hash output. "
        "Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash.", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("outscript=VALUE:SCRIPT[:FLAGS]", "Add raw script output to TX. "
        "Optionally add the \"W\" flag to produce a pay-to-witness-script-hash output. "
        "Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash.", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("replaceable(=N)", "Set RBF opt-in sequence number for input N (if not provided, opt-in all available inputs)", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("sign=SIGHASH-FLAGS", "Add zero or more signatures to transaction. "
        "This command requires JSON registers:"
        "prevtxs=JSON object, "
        "privatekeys=JSON object. "
        "See signrawtransactionwithkey docs for format of sighash flags, JSON objects.", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);

    argsman.AddArg("load=NAME:FILENAME", "Load JSON file FILENAME into register NAME", ArgsManager::ALLOW_ANY, OptionsCategory::REGISTER_COMMANDS);
    argsman.AddArg("set=NAME:JSON-STRING", "Set register NAME to given JSON-STRING", ArgsManager::ALLOW_ANY, OptionsCategory::REGISTER_COMMANDS);
}

//
// This function returns either one of EXIT_ codes when it's expected to stop the process or
// CONTINUE_EXECUTION when it's expected to continue further.
//
static int AppInitRawTx(int argc, char* argv[])
{
    SetupBitcoinTxArgs(gArgs);
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error);
        return EXIT_FAILURE;
    }

    // Check for chain settings (Params() calls are only valid after this clause)
    try {
        SelectParams(gArgs.GetChainName());
    } catch (const std::exception& e) {
        tfm::format(std::cerr, "Error: %s\n", e.what());
        return EXIT_FAILURE;
    }

    fCreateBlank = gArgs.GetBoolArg("-create", false);
    
    bool isJurat = gArgs.IsArgSet("-jurat");
    tfm::format(std::cout, "isJurat: %b\n", isJurat);
    
    

    if (argc < 2 || HelpRequested(gArgs) || gArgs.IsArgSet("-version")) {
        // First part of help message is specific to this utility
        std::string strUsage = PACKAGE_NAME " bitcoin-tx utility version " + FormatFullVersion() + "\n";

        if (gArgs.IsArgSet("-version")) {
            strUsage += FormatParagraph(LicenseInfo());
        } else {
            strUsage += "\n"
                "Usage:  bitcoin-tx [options] <hex-tx> [commands]  Update hex-encoded bitcoin transaction\n"
                "or:     bitcoin-tx [options] -create [commands]   Create hex-encoded bitcoin transaction\n"
                "\n";
            strUsage += gArgs.GetHelpMessage();
        }

        tfm::format(std::cout, "%s", strUsage);

        if (argc < 2) {
            tfm::format(std::cerr, "Error: too few parameters\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    return CONTINUE_EXECUTION;
}

static void RegisterSetJson(const std::string& key, const std::string& rawJson)
{
    UniValue val;
    if (!val.read(rawJson)) {
        std::string strErr = "Cannot parse JSON for key " + key;
        throw std::runtime_error(strErr);
    }

    registers[key] = val;
}

static void RegisterSet(const std::string& strInput)
{
    // separate NAME:VALUE in string
    size_t pos = strInput.find(':');
    if ((pos == std::string::npos) ||
        (pos == 0) ||
        (pos == (strInput.size() - 1)))
        throw std::runtime_error("Register input requires NAME:VALUE");

    std::string key = strInput.substr(0, pos);
    std::string valStr = strInput.substr(pos + 1, std::string::npos);

    RegisterSetJson(key, valStr);
}

static void RegisterLoad(const std::string& strInput)
{
    // separate NAME:FILENAME in string
    size_t pos = strInput.find(':');
    if ((pos == std::string::npos) ||
        (pos == 0) ||
        (pos == (strInput.size() - 1)))
        throw std::runtime_error("Register load requires NAME:FILENAME");

    std::string key = strInput.substr(0, pos);
    std::string filename = strInput.substr(pos + 1, std::string::npos);

    FILE *f = fsbridge::fopen(filename.c_str(), "r");
    if (!f) {
        std::string strErr = "Cannot open file " + filename;
        throw std::runtime_error(strErr);
    }

    // load file chunks into one big buffer
    std::string valStr;
    while ((!feof(f)) && (!ferror(f))) {
        char buf[4096];
        int bread = fread(buf, 1, sizeof(buf), f);
        if (bread <= 0)
            break;

        valStr.insert(valStr.size(), buf, bread);
    }

    int error = ferror(f);
    fclose(f);

    if (error) {
        std::string strErr = "Error reading file " + filename;
        throw std::runtime_error(strErr);
    }

    // evaluate as JSON buffer register
    RegisterSetJson(key, valStr);
}

static CAmount ExtractAndValidateValue(const std::string& strValue)
{
    if (std::optional<CAmount> parsed = ParseMoney(strValue)) {
        return parsed.value();
    } else {
        throw std::runtime_error("invalid TX output value");
    }
}

static void MutateTxVersion(CMutableTransaction& tx, const std::string& cmdVal)
{
    int64_t newVersion;
    if (!ParseInt64(cmdVal, &newVersion) || newVersion < 1 || newVersion > TX_MAX_STANDARD_VERSION) {
        throw std::runtime_error("Invalid TX version requested: '" + cmdVal + "'");
    }

    tx.nVersion = (int) newVersion;
}

static void MutateTxLocktime(CMutableTransaction& tx, const std::string& cmdVal)
{
    int64_t newLocktime;
    if (!ParseInt64(cmdVal, &newLocktime) || newLocktime < 0LL || newLocktime > 0xffffffffLL)
        throw std::runtime_error("Invalid TX locktime requested: '" + cmdVal + "'");

    tx.nLockTime = (unsigned int) newLocktime;
}

static void MutateTxRBFOptIn(CMutableTransaction& tx, const std::string& strInIdx)
{
    // parse requested index
    int64_t inIdx;
    if (!ParseInt64(strInIdx, &inIdx) || inIdx < 0 || inIdx >= static_cast<int64_t>(tx.vin.size())) {
        throw std::runtime_error("Invalid TX input index '" + strInIdx + "'");
    }

    // set the nSequence to MAX_INT - 2 (= RBF opt in flag)
    int cnt = 0;
    for (CTxIn& txin : tx.vin) {
        if (strInIdx == "" || cnt == inIdx) {
            if (txin.nSequence > MAX_BIP125_RBF_SEQUENCE) {
                txin.nSequence = MAX_BIP125_RBF_SEQUENCE;
            }
        }
        ++cnt;
    }
}

template <typename T>
static T TrimAndParse(const std::string& int_str, const std::string& err)
{
    const auto parsed{ToIntegral<T>(TrimString(int_str))};
    if (!parsed.has_value()) {
        throw std::runtime_error(err + " '" + int_str + "'");
    }
    return parsed.value();
}

static void MutateTxAddInput(CMutableTransaction& tx, const std::string& strInput)
{
    std::vector<std::string> vStrInputParts;
    boost::split(vStrInputParts, strInput, boost::is_any_of(":"));

    // separate TXID:VOUT in string
    if (vStrInputParts.size()<2)
        throw std::runtime_error("TX input missing separator");

    // extract and validate TXID
    uint256 txid;
    if (!ParseHashStr(vStrInputParts[0], txid)) {
        throw std::runtime_error("invalid TX input txid");
    }

    static const unsigned int minTxOutSz = 9;
    static const unsigned int maxVout = MAX_BLOCK_WEIGHT / (WITNESS_SCALE_FACTOR * minTxOutSz);

    // extract and validate vout
    const std::string& strVout = vStrInputParts[1];
    int64_t vout;
    if (!ParseInt64(strVout, &vout) || vout < 0 || vout > static_cast<int64_t>(maxVout))
        throw std::runtime_error("invalid TX input vout '" + strVout + "'");

    // extract the optional sequence number
    uint32_t nSequenceIn = CTxIn::SEQUENCE_FINAL;
    if (vStrInputParts.size() > 2) {
        nSequenceIn = TrimAndParse<uint32_t>(vStrInputParts.at(2), "invalid TX sequence id");
    }

    // append to transaction input list
    CTxIn txin(txid, vout, CScript(), nSequenceIn);
    tx.vin.push_back(txin);
}

static void MutateTxAddOutAddr(CMutableTransaction& tx, const std::string& strInput)
{
    // Separate into VALUE:ADDRESS
    std::vector<std::string> vStrInputParts;
    boost::split(vStrInputParts, strInput, boost::is_any_of(":"));

    if (vStrInputParts.size() != 2)
        throw std::runtime_error("TX output missing or too many separators");

    // Extract and validate VALUE
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);

    // extract and validate ADDRESS
    std::string strAddr = vStrInputParts[1];
    CTxDestination destination = DecodeDestination(strAddr);
    if (!IsValidDestination(destination)) {
        throw std::runtime_error("invalid TX output address");
    }
    CScript scriptPubKey = GetScriptForDestination(destination);

    // construct TxOut, append to transaction output list
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}

static void MutateTxAddOutPubKey(CMutableTransaction& tx, const std::string& strInput)
{
    // Separate into VALUE:PUBKEY[:FLAGS]
    std::vector<std::string> vStrInputParts;
    boost::split(vStrInputParts, strInput, boost::is_any_of(":"));

    if (vStrInputParts.size() < 2 || vStrInputParts.size() > 3)
        throw std::runtime_error("TX output missing or too many separators");

    // Extract and validate VALUE
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);

    // Extract and validate PUBKEY
    CPubKey pubkey(ParseHex(vStrInputParts[1]));
    if (!pubkey.IsFullyValid())
        throw std::runtime_error("invalid TX output pubkey");
    CScript scriptPubKey = GetScriptForRawPubKey(pubkey);

    // Extract and validate FLAGS
    bool bSegWit = false;
    bool bScriptHash = false;
    if (vStrInputParts.size() == 3) {
        std::string flags = vStrInputParts[2];
        bSegWit = (flags.find('W') != std::string::npos);
        bScriptHash = (flags.find('S') != std::string::npos);
    }

    if (bSegWit) {
        if (!pubkey.IsCompressed()) {
            throw std::runtime_error("Uncompressed pubkeys are not useable for SegWit outputs");
        }
        // Build a P2WPKH script
        scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(pubkey));
    }
    if (bScriptHash) {
        // Get the ID for the script, and then construct a P2SH destination for it.
        scriptPubKey = GetScriptForDestination(ScriptHash(scriptPubKey));
    }

    // construct TxOut, append to transaction output list
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}

static void MutateTxAddOutMultiSig(CMutableTransaction& tx, const std::string& strInput)
{
    // Separate into VALUE:REQUIRED:NUMKEYS:PUBKEY1:PUBKEY2:....[:FLAGS]
    std::vector<std::string> vStrInputParts;
    boost::split(vStrInputParts, strInput, boost::is_any_of(":"));

    // Check that there are enough parameters
    if (vStrInputParts.size()<3)
        throw std::runtime_error("Not enough multisig parameters");

    // Extract and validate VALUE
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);

    // Extract REQUIRED
    const uint32_t required{TrimAndParse<uint32_t>(vStrInputParts.at(1), "invalid multisig required number")};

    // Extract NUMKEYS
    const uint32_t numkeys{TrimAndParse<uint32_t>(vStrInputParts.at(2), "invalid multisig total number")};

    // Validate there are the correct number of pubkeys
    if (vStrInputParts.size() < numkeys + 3)
        throw std::runtime_error("incorrect number of multisig pubkeys");

    if (required < 1 || required > MAX_PUBKEYS_PER_MULTISIG || numkeys < 1 || numkeys > MAX_PUBKEYS_PER_MULTISIG || numkeys < required)
        throw std::runtime_error("multisig parameter mismatch. Required " \
                            + ToString(required) + " of " + ToString(numkeys) + "signatures.");

    // extract and validate PUBKEYs
    std::vector<CPubKey> pubkeys;
    for(int pos = 1; pos <= int(numkeys); pos++) {
        CPubKey pubkey(ParseHex(vStrInputParts[pos + 2]));
        if (!pubkey.IsFullyValid())
            throw std::runtime_error("invalid TX output pubkey");
        pubkeys.push_back(pubkey);
    }

    // Extract FLAGS
    bool bSegWit = false;
    bool bScriptHash = false;
    if (vStrInputParts.size() == numkeys + 4) {
        std::string flags = vStrInputParts.back();
        bSegWit = (flags.find('W') != std::string::npos);
        bScriptHash = (flags.find('S') != std::string::npos);
    }
    else if (vStrInputParts.size() > numkeys + 4) {
        // Validate that there were no more parameters passed
        throw std::runtime_error("Too many parameters");
    }

    CScript scriptPubKey = GetScriptForMultisig(required, pubkeys);

    if (bSegWit) {
        for (const CPubKey& pubkey : pubkeys) {
            if (!pubkey.IsCompressed()) {
                throw std::runtime_error("Uncompressed pubkeys are not useable for SegWit outputs");
            }
        }
        // Build a P2WSH with the multisig script
        scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(scriptPubKey));
    }
    if (bScriptHash) {
        if (scriptPubKey.size() > MAX_SCRIPT_ELEMENT_SIZE) {
            throw std::runtime_error(strprintf(
                        "redeemScript exceeds size limit: %d > %d", scriptPubKey.size(), MAX_SCRIPT_ELEMENT_SIZE));
        }
        // Get the ID for the script, and then construct a P2SH destination for it.
        scriptPubKey = GetScriptForDestination(ScriptHash(scriptPubKey));
    }

    // construct TxOut, append to transaction output list
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}

static void MutateTxAddOutData(CMutableTransaction& tx, const std::string& strInput)
{
    CAmount value = 0;

    // separate [VALUE:]DATA in string
    size_t pos = strInput.find(':');

    if (pos==0)
        throw std::runtime_error("TX output value not specified");

    if (pos == std::string::npos) {
        pos = 0;
    } else {
        // Extract and validate VALUE
        value = ExtractAndValidateValue(strInput.substr(0, pos));
        ++pos;
    }

    // extract and validate DATA
    const std::string strData{strInput.substr(pos, std::string::npos)};

    if (!IsHex(strData))
        throw std::runtime_error("invalid TX output data");

    std::vector<unsigned char> data = ParseHex(strData);

    CTxOut txout(value, CScript() << OP_RETURN << data);
    tx.vout.push_back(txout);
}

static void MutateTxAddOutScript(CMutableTransaction& tx, const std::string& strInput)
{
    // separate VALUE:SCRIPT[:FLAGS]
    std::vector<std::string> vStrInputParts;
    boost::split(vStrInputParts, strInput, boost::is_any_of(":"));
    if (vStrInputParts.size() < 2)
        throw std::runtime_error("TX output missing separator");

    // Extract and validate VALUE
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);

    // extract and validate script
    std::string strScript = vStrInputParts[1];
    CScript scriptPubKey = ParseScript(strScript);

    // Extract FLAGS
    bool bSegWit = false;
    bool bScriptHash = false;
    if (vStrInputParts.size() == 3) {
        std::string flags = vStrInputParts.back();
        bSegWit = (flags.find('W') != std::string::npos);
        bScriptHash = (flags.find('S') != std::string::npos);
    }

    if (scriptPubKey.size() > MAX_SCRIPT_SIZE) {
        throw std::runtime_error(strprintf(
                    "script exceeds size limit: %d > %d", scriptPubKey.size(), MAX_SCRIPT_SIZE));
    }

    if (bSegWit) {
        scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(scriptPubKey));
    }
    if (bScriptHash) {
        if (scriptPubKey.size() > MAX_SCRIPT_ELEMENT_SIZE) {
            throw std::runtime_error(strprintf(
                        "redeemScript exceeds size limit: %d > %d", scriptPubKey.size(), MAX_SCRIPT_ELEMENT_SIZE));
        }
        scriptPubKey = GetScriptForDestination(ScriptHash(scriptPubKey));
    }

    // construct TxOut, append to transaction output list
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}

static void MutateTxDelInput(CMutableTransaction& tx, const std::string& strInIdx)
{
    // parse requested deletion index
    int64_t inIdx;
    if (!ParseInt64(strInIdx, &inIdx) || inIdx < 0 || inIdx >= static_cast<int64_t>(tx.vin.size())) {
        throw std::runtime_error("Invalid TX input index '" + strInIdx + "'");
    }

    // delete input from transaction
    tx.vin.erase(tx.vin.begin() + inIdx);
}

static void MutateTxDelOutput(CMutableTransaction& tx, const std::string& strOutIdx)
{
    // parse requested deletion index
    int64_t outIdx;
    if (!ParseInt64(strOutIdx, &outIdx) || outIdx < 0 || outIdx >= static_cast<int64_t>(tx.vout.size())) {
        throw std::runtime_error("Invalid TX output index '" + strOutIdx + "'");
    }

    // delete output from transaction
    tx.vout.erase(tx.vout.begin() + outIdx);
}

static const unsigned int N_SIGHASH_OPTS = 7;
static const struct {
    const char *flagStr;
    int flags;
} sighashOptions[N_SIGHASH_OPTS] = {
    {"DEFAULT", SIGHASH_DEFAULT},
    {"ALL", SIGHASH_ALL},
    {"NONE", SIGHASH_NONE},
    {"SINGLE", SIGHASH_SINGLE},
    {"ALL|ANYONECANPAY", SIGHASH_ALL|SIGHASH_ANYONECANPAY},
    {"NONE|ANYONECANPAY", SIGHASH_NONE|SIGHASH_ANYONECANPAY},
    {"SINGLE|ANYONECANPAY", SIGHASH_SINGLE|SIGHASH_ANYONECANPAY},
};

static bool findSighashFlags(int& flags, const std::string& flagStr)
{
    flags = 0;

    for (unsigned int i = 0; i < N_SIGHASH_OPTS; i++) {
        if (flagStr == sighashOptions[i].flagStr) {
            flags = sighashOptions[i].flags;
            return true;
        }
    }

    return false;
}

static CAmount AmountFromValue(const UniValue& value)
{
    if (!value.isNum() && !value.isStr())
        throw std::runtime_error("Amount is not a number or string");
    CAmount amount;
    if (!ParseFixedPoint(value.getValStr(), 8, &amount))
        throw std::runtime_error("Invalid amount");
    if (!MoneyRange(amount))
        throw std::runtime_error("Amount out of range");
    return amount;
}

static void MutateTxSign(CMutableTransaction& tx, const std::string& flagStr)
{
    int nHashType = SIGHASH_ALL;

    if (flagStr.size() > 0)
        if (!findSighashFlags(nHashType, flagStr))
            throw std::runtime_error("unknown sighash flag/sign option");

    // mergedTx will end up with all the signatures; it
    // starts as a clone of the raw tx:
    CMutableTransaction mergedTx{tx};
    const CMutableTransaction txv{tx};
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);

    if (!registers.count("privatekeys"))
        throw std::runtime_error("privatekeys register variable must be set.");
    FillableSigningProvider tempKeystore;
    UniValue keysObj = registers["privatekeys"];

    for (unsigned int kidx = 0; kidx < keysObj.size(); kidx++) {
        if (!keysObj[kidx].isStr())
            throw std::runtime_error("privatekey not a std::string");
        CKey key = DecodeSecret(keysObj[kidx].getValStr());
        if (!key.IsValid()) {
            throw std::runtime_error("privatekey not valid");
        }
        tempKeystore.AddKey(key);
    }

    // Add previous txouts given in the RPC call:
    if (!registers.count("prevtxs"))
        throw std::runtime_error("prevtxs register variable must be set.");
    UniValue prevtxsObj = registers["prevtxs"];
    {
        for (unsigned int previdx = 0; previdx < prevtxsObj.size(); previdx++) {
            UniValue prevOut = prevtxsObj[previdx];
            if (!prevOut.isObject())
                throw std::runtime_error("expected prevtxs internal object");

            std::map<std::string, UniValue::VType> types = {
                {"txid", UniValue::VSTR},
                {"vout", UniValue::VNUM},
                {"scriptPubKey", UniValue::VSTR},
            };
            if (!prevOut.checkObject(types))
                throw std::runtime_error("prevtxs internal object typecheck fail");

            uint256 txid;
            if (!ParseHashStr(prevOut["txid"].get_str(), txid)) {
                throw std::runtime_error("txid must be hexadecimal string (not '" + prevOut["txid"].get_str() + "')");
            }

            const int nOut = prevOut["vout"].get_int();
            if (nOut < 0)
                throw std::runtime_error("vout cannot be negative");

            COutPoint out(txid, nOut);
            std::vector<unsigned char> pkData(ParseHexUV(prevOut["scriptPubKey"], "scriptPubKey"));
            CScript scriptPubKey(pkData.begin(), pkData.end());

            {
                const Coin& coin = view.AccessCoin(out);
                if (!coin.IsSpent() && coin.out.scriptPubKey != scriptPubKey) {
                    std::string err("Previous output scriptPubKey mismatch:\n");
                    err = err + ScriptToAsmStr(coin.out.scriptPubKey) + "\nvs:\n"+
                        ScriptToAsmStr(scriptPubKey);
                    throw std::runtime_error(err);
                }
                Coin newcoin;
                newcoin.out.scriptPubKey = scriptPubKey;
                newcoin.out.nValue = MAX_MONEY;
                if (prevOut.exists("amount")) {
                    newcoin.out.nValue = AmountFromValue(prevOut["amount"]);
                }
                newcoin.nHeight = 1;
                view.AddCoin(out, std::move(newcoin), true);
            }

            // if redeemScript given and private keys given,
            // add redeemScript to the tempKeystore so it can be signed:
            if ((scriptPubKey.IsPayToScriptHash() || scriptPubKey.IsPayToWitnessScriptHash()) &&
                prevOut.exists("redeemScript")) {
                UniValue v = prevOut["redeemScript"];
                std::vector<unsigned char> rsData(ParseHexUV(v, "redeemScript"));
                CScript redeemScript(rsData.begin(), rsData.end());
                tempKeystore.AddCScript(redeemScript);
            }
        }
    }

    const FillableSigningProvider& keystore = tempKeystore;

    bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);

    // Sign what we can:
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++) {
        CTxIn& txin = mergedTx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            continue;
        }
        const CScript& prevPubKey = coin.out.scriptPubKey;
        const CAmount& amount = coin.out.nValue;

        SignatureData sigdata = DataFromTransaction(mergedTx, i, coin.out);
        // Only sign SIGHASH_SINGLE if there's a corresponding output:
        if (!fHashSingle || (i < mergedTx.vout.size()))
            ProduceSignature(keystore, MutableTransactionSignatureCreator(&mergedTx, i, amount, nHashType), prevPubKey, sigdata);

        if (amount == MAX_MONEY && !sigdata.scriptWitness.IsNull()) {
            throw std::runtime_error(strprintf("Missing amount for CTxOut with scriptPubKey=%s", HexStr(prevPubKey)));
        }

        UpdateInput(txin, sigdata);
    }

    tx = mergedTx;
}

class Secp256k1Init
{
    ECCVerifyHandle globalVerifyHandle;

public:
    Secp256k1Init() {
        ECC_Start();
    }
    ~Secp256k1Init() {
        ECC_Stop();
    }
};

static void MutateTx(CMutableTransaction& tx, const std::string& command,
                     const std::string& commandVal)
{
    std::unique_ptr<Secp256k1Init> ecc;

    if (command == "nversion")
        MutateTxVersion(tx, commandVal);
    else if (command == "locktime")
        MutateTxLocktime(tx, commandVal);
    else if (command == "replaceable") {
        MutateTxRBFOptIn(tx, commandVal);
    }

    else if (command == "delin")
        MutateTxDelInput(tx, commandVal);
    else if (command == "in")
        MutateTxAddInput(tx, commandVal);

    else if (command == "delout")
        MutateTxDelOutput(tx, commandVal);
    else if (command == "outaddr")
        MutateTxAddOutAddr(tx, commandVal);
    else if (command == "outpubkey") {
        ecc.reset(new Secp256k1Init());
        MutateTxAddOutPubKey(tx, commandVal);
    } else if (command == "outmultisig") {
        ecc.reset(new Secp256k1Init());
        MutateTxAddOutMultiSig(tx, commandVal);
    } else if (command == "outscript")
        MutateTxAddOutScript(tx, commandVal);
    else if (command == "outdata")
        MutateTxAddOutData(tx, commandVal);

    else if (command == "sign") {
        ecc.reset(new Secp256k1Init());
        MutateTxSign(tx, commandVal);
    }

    else if (command == "load")
        RegisterLoad(commandVal);

    else if (command == "set")
        RegisterSet(commandVal);
    
    else
        throw std::runtime_error("unknown command");
}

static void OutputTxJSON(const CTransaction& tx)
{
    UniValue entry(UniValue::VOBJ);
    TxToUniv(tx, uint256(), entry);

    std::string jsonOutput = entry.write(4);
    tfm::format(std::cout, "%s\n", jsonOutput);
}

static void OutputTxHash(const CTransaction& tx)
{
    std::string strHexHash = tx.GetHash().GetHex(); // the hex-encoded transaction hash (aka the transaction id)

    tfm::format(std::cout, "%s\n", strHexHash);
}

static void OutputTxHex(const CTransaction& tx)
{
    std::string strHex = EncodeHexTx(tx);

    tfm::format(std::cout, "%s\n", strHex);
}

static void OutputTx(const CTransaction& tx)
{
    if (gArgs.GetBoolArg("-json", false))
        OutputTxJSON(tx);
    else if (gArgs.GetBoolArg("-txid", false))
        OutputTxHash(tx);
    else
        OutputTxHex(tx);
}

static std::string readStdin()
{
    char buf[4096];
    std::string ret;

    while (!feof(stdin)) {
        size_t bread = fread(buf, 1, sizeof(buf), stdin);
        ret.append(buf, bread);
        if (bread < sizeof(buf))
            break;
    }

    if (ferror(stdin))
        throw std::runtime_error("error reading stdin");

    return TrimString(ret);
}

static int CommandLineRawTx(int argc, char* argv[])
{
    std::string strPrint;
    int nRet = 0;
    try {
        // Skip switches; Permit common stdin convention "-"
        while (argc > 1 && IsSwitchChar(argv[1][0]) &&
               (argv[1][1] != 0)) {
            argc--;
            argv++;
        }

        CMutableTransaction tx;
        int startArg;

        if (!fCreateBlank) {
            // require at least one param
            if (argc < 2)
                throw std::runtime_error("too few parameters");

            // param: hex-encoded bitcoin transaction
            std::string strHexTx(argv[1]);
            if (strHexTx == "-")                 // "-" implies standard input
                strHexTx = readStdin();

            if (!DecodeHexTx(tx, strHexTx, true))
                throw std::runtime_error("invalid transaction encoding");

            startArg = 2;
        } else
            startArg = 1;

        for (int i = startArg; i < argc; i++) {
            std::string arg = argv[i];
            std::string key, value;
            size_t eqpos = arg.find('=');
            if (eqpos == std::string::npos)
                key = arg;
            else {
                key = arg.substr(0, eqpos);
                value = arg.substr(eqpos + 1);
            }
            MutateTx(tx, key, value);
        }
        
        bool isJurat = gArgs.IsArgSet("-jurat");
        if(isJurat) {
            runJuratCmd();
            
        } else {
            OutputTx(CTransaction(tx));
        }

        
    }
    catch (const std::exception& e) {
        strPrint = std::string("error: ") + e.what();
        nRet = EXIT_FAILURE;
    }
    catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRawTx()");
        throw;
    }

    if (strPrint != "") {
        tfm::format(nRet == 0 ? std::cout : std::cerr, "%s\n", strPrint);
    }
    return nRet;
}

int main(int argc, char* argv[])
{
    SetupEnvironment();

    try {
        int ret = AppInitRawTx(argc, argv);
        if (ret != CONTINUE_EXECUTION)
            return ret;
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInitRawTx()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInitRawTx()");
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;
    try {
        ret = CommandLineRawTx(argc, argv);
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "CommandLineRawTx()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRawTx()");
    }
    return ret;
}
