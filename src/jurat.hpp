//
//  jurat.hpp
//
//  Created by Subodh Sharma on 12/29/21.
//

#ifndef jurat_hpp
#define jurat_hpp

#include <stdio.h>

#include <hash.h>
#include <core_io.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <key.h>
#include <algorithm>
#include <set>
#include <base58.h>
#include <key_io.h>
#include <script/standard.h>




namespace jurat {

///jurat keys
const int N_JURAT_MIN_SIGS = 2;
const int N_JURAT_KEYS = 2;
const std::string judicialVerifierPubKeys[] = {"03b45b93cc13534ff960d236f8a5b57c4d1e6c559397420612840ae2167e6eded1",
    "02c25b7cf4cb06fa97a718524954d3febd46edc45ced2d129f8e0e3bdc43b391e4"};

///declarations

///jurat verifier
class JuratVerifier {
    
private:
    const CTransaction& m_tx;
    
    const CScript& prevScriptPubKey;
    
    int32_t vin;
    int32_t vout;
    int nLockBlockHeight;
    
    std::vector<int> verifiedKeys;
    bool isVerified;
    
public:
    JuratVerifier() = delete;
    
    //JuratVerifier(const CTransaction& tx):m_tx(tx){}
    
    JuratVerifier(const CTransaction& tx, const CScript& scriptPubKey):m_tx(tx),prevScriptPubKey(scriptPubKey){}
    
    bool isJuratTx();
    
    ///needs ECDSA library to be initialized before calling this
    bool verify();
    
    ///for jurat verified tx this will return the scriptPubKey compatible with the witness
    CScript getScriptPubKey();
    
    ///strip out jutar sigs and retain onlybitcoin format sigs
    CScriptWitness getModifiedWitness();
    
    
};


///jurat enum for various jurat constants
enum class enumJurat:uint8_t {
    
    JURAT_MARKER1 = 0xAB,
    JURAT_MARKER2 = 0xCD,
    JURAT_ACTION_FREEZE_WITH_TIMELOCK = 0x01,
    JURAT_ACTION_MOVE_FUND_WITH_TIMLOCK = 0x02,
    JURAT_ACTION_MOVE_FUND_IMMEDIATE = 0x03
};

///implementations

///checks if bitcoin address is supported for jurat freeze etc
bool checkAddr(std::string addr, std::vector<unsigned char>& vchHash,
               CScript& addrScriptPubKey,std::string& err)
{
    vchHash.clear();
    std::string errMsg;
    CTxDestination dest = DecodeDestination(addr, errMsg);
    const bool isValid = IsValidDestination(dest);
    if(!isValid)
    {
        //invalid address
        err =  addr + " ...invalid address";
        return false;
    }
    
    CScript scriptPubKey = GetScriptForDestination(dest);
    std::vector<std::vector<unsigned char>> vSolutionsRet;
    TxoutType txOutType = Solver(scriptPubKey, vSolutionsRet);
    std::string strSrcAddrType = GetTxnOutputType(txOutType);
    std::vector<unsigned char> vchPKHash;
    
    if (txOutType == TxoutType::PUBKEY){
        uint160 pubKeyHash = Hash160(vSolutionsRet[0]);
        vchPKHash = std::vector<unsigned char>(pubKeyHash.begin(), pubKeyHash.end());
    } else if(txOutType == TxoutType::PUBKEYHASH){
        vchPKHash =vSolutionsRet[0];
    } else if(txOutType == TxoutType::WITNESS_V0_KEYHASH){
        vchPKHash =vSolutionsRet[0];
    } else {
        //address is unsupported
        err =  "unsupported address type: " + GetTxnOutputType(txOutType);
        return false;
    }
    
    copy(vchPKHash.begin(), vchPKHash.end(), back_inserter(vchHash));
    addrScriptPubKey = scriptPubKey;
    return true;
    
}

///helper function to create the vout with OP_RETURN and jurat info
bool createJuratMarker(const enumJurat juratAction, uint8_t vin, uint8_t vout, int nBlockLockHeigt,
                       std::vector<unsigned char> vchData,CScript& outScript)
{
    outScript << OP_RETURN;
    
    std::vector<unsigned char> markerPayload;
    markerPayload.push_back((unsigned char)enumJurat::JURAT_MARKER1);
    markerPayload.push_back((unsigned char)enumJurat::JURAT_MARKER2);
    markerPayload.push_back((unsigned char)juratAction);
    markerPayload.push_back(vin);
    markerPayload.push_back(vout);
    //4 bytes for blockheight
    std::cout << "createJuratMarker: vchData=" << HexStr(vchData) << " nBlockLockHeigt="<<
    nBlockLockHeigt << std::endl;
    
    unsigned char* pBlockHeight = static_cast<unsigned char*>(static_cast<void*>(&nBlockLockHeigt));
    copy(pBlockHeight, pBlockHeight+4,back_inserter(markerPayload));
    
    //copy upto 20 bytes of data
    unsigned char copy_len = (unsigned char)vchData.size();
    copy_len = copy_len > 20 ? 20:copy_len;
    markerPayload.push_back(copy_len);
    if (copy_len>0){
        copy(vchData.begin(), vchData.begin()+copy_len,back_inserter(markerPayload));
    }
    
    outScript << markerPayload;
    
    std::cout << "createJuratMarker: final=" << HexStr(outScript) << std::endl;
    
    return true;
}

///create the time lock script. the freeze tx will send the coin to this scripthash
bool createLockingScript(int nLockBlockHeight, std::vector<unsigned char>& vchPKHash,
                         CScript& script, CScript& pshScriptPubKey)
{
    //naive check for block height and 20 byte pubkey hash
    if (nLockBlockHeight < 0 || vchPKHash.size()!=20){
        return false;
    }
    
    script << nLockBlockHeight << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_DUP;
    script << OP_HASH160 << vchPKHash << OP_EQUALVERIFY << OP_CHECKSIG;
    
    std::cout << "createLockingScript: pkhash="
    << HexStr(vchPKHash) << " blockheight=" << nLockBlockHeight << std::endl;
    
    ScriptHash scriptHash(script);
    pshScriptPubKey = GetScriptForDestination(scriptHash);
    
    return true;
}


///<OP_RETURN 2 byte> <Jurat Marker1 1 byte> <Jurat Marker2 1 byte> <Jurat Action 1 byte>
///<OP_RETURN 2 byte> <Jurat Marker1 1 byte> <Jurat Marker2 1 byte> <Jurat Action 1 byte>
///<vin 1 byte> <vout 1 byte> <block-height 4 bytes>
///<trailing data length 1 byte> <trailing data bytes upto 20 bytes>
///minimum length = 2 + 1 + 1 + 1 + 1 + 1 + 4 + 1 = 12 bytes
///maximum length = 12 + 20 = 32 bytes
///allowed maximum length = OP_RETURN + 40 bytes of data = 42 bytes
bool identifyJuratTx(const CTransaction& tx, int32_t& vin, int32_t& vout,
                     int32_t& nLockBlockHeight,
                     std::vector<unsigned char>& targetAddrPKHash)
{
    bool found = false;
    std::cout << "looking for jurat tx...." << std::endl;
    const int minScriptPubKeyLength = 12;
    for (const CTxOut& out: tx.vout)
    {
        const CScript& scriptPubKey = out.scriptPubKey;
        int len = scriptPubKey.end() - scriptPubKey.begin();
        if (len < minScriptPubKeyLength){
            continue;
        }
        
        //check OP_RETURN
        CScript::const_iterator pc = scriptPubKey.begin();
        if (*pc != OP_RETURN){
            continue;
        }
        
        std::cout << "identifyJuratTx: scriptPubKey len = " << len << std::endl;
        pc = pc+2;
        uint8_t marker1 = *(pc);
        uint8_t marker2 = *(pc+1);
        uint8_t action = *(pc+2);
        vin = (int32_t)*(pc+3);
        vout = (int32_t)*(pc+4);
        unsigned char ch[4];
        for(int i=0;i<4;i++) ch[i] = *(pc+5+i);
        int32_t blockHeight=*((int*)ch);
        int32_t datalen = (int32_t)*(pc+9);
        
        //check jurat markers
        if((uint8_t)enumJurat::JURAT_MARKER1 != marker1 ||
           (uint8_t)enumJurat::JURAT_MARKER2 != marker2 ||
           datalen < 0 ){
            break;
        }
        
        //check jurat action
        if((uint8_t)enumJurat::JURAT_ACTION_FREEZE_WITH_TIMELOCK != action){
            std::cout << "jurat action not supported = " << (int)action << std::endl;
            break;
        }
        std::cout << "identifyJuratTx: [verified] jurat marker1 = " << marker1 << std::endl;
        std::cout << "identifyJuratTx: [verified] jurat marker2 = " << marker2 << std::endl;
        std::cout << "identifyJuratTx: [verified] jurat action  = " << (int)action << std::endl;
        
        if (blockHeight < 0 || vin < 0 || vin >= (int)tx.vin.size()
            || vout < 0 || vout >= (int)tx.vout.size()){
            std::cout << "invalid jurat marker payload..." << blockHeight << std::endl;
            break;
        }
        
        std::cout << "identifyJuratTx: [verified] vin  = " << vin << std::endl;
        std::cout << "identifyJuratTx: [verified] vout  = " << vout << std::endl;
        std::cout << "identifyJuratTx: [verified] blockHeight  = " << blockHeight << std::endl;
        std::cout << "identifyJuratTx: [verified] trailing data length  = " << datalen << std::endl;
        
        pc = pc+10;
        if(datalen > 0){
            std::vector<unsigned char> vchData(pc,pc+datalen);
            CScript script;
            CScript pshScriptPubKey;
            createLockingScript(blockHeight, vchData, script, pshScriptPubKey);
            if (pshScriptPubKey != tx.vout[vout].scriptPubKey){
                std::cout << "identifyJuratTx: tx.vout[vout].scriptPubKey did not match... " <<
                HexStr(pshScriptPubKey) << std::endl;
                break;
            }
            
            ///this confirms that this vout is sent to script hash tied to target-address pub keyhash
            targetAddrPKHash = vchData;
            std::cout << "identifyJuratTx: [verified] tx.vout[vout].scriptPubKey " <<
            HexStr(pshScriptPubKey) << std::endl;
        }
        
        nLockBlockHeight = blockHeight;
        found = true;
        break;
    }
    
    return found;
    
}

///this verifies the witness that has the following expected format
///witness.stack[0] =""
///witness.stack[1] = jurat_specific_sig_judicial_verifier_pvt_key0
///witness.stack[2] = witnesssi_V0_sig_judicial_verifier_pvt_key0
///witness.stack[3] = jurat_specific_sig_judicial_verifier_pvt_key1
///witness.stack[4] = witnesssi_V0_sig_judicial_verifier_pvt_key1
///witness.stack[2n] = jurat_specific_sig_judicial_verifier_pvt_keyn
///witness.stack[2n+1] = witnesssi_V0_sig_judicial_verifier_pvt_keyn
///witness.stack[2(n+1)] = redeemScript
///redeemScript = <pub_key_n> OP_CHECKSIGVERIFY OP_DROP .... <pub_key_0> OP_CHECKSIGVERIFY OP_DROP OP_0 OP_ADD OP_VERIFY
bool verifyJuratTxSig(const CTransaction& tx, int32_t& vin, int32_t& vout,
                      int nLockBlockHeight,
                      const CScript& srcScriptPubKey,
                      std::vector<unsigned char>& targetAddrPKHash,
                      std::vector<int>& verifiedKeys)
{
    if (vin < 0 || vin >= (int32_t)tx.vin.size()
        || vout < 0 || vout >= (int32_t)tx.vout.size())
    {
        return false;
    }
    
    CScriptWitness witness =  tx.vin[vin].scriptWitness;
    int len = (int) witness.stack.size();
    std::cout << "[verifier] witness stack size = " << len << std::endl;
    if (len < N_JURAT_MIN_SIGS+2 ||  len%2 != 0)
    {
        //starting 0 element + each pubkey has two sigs + redeemScript last element
        return false;
    }
    
    std::vector<std::vector<unsigned char>> vSolutionsRet;
    TxoutType txOutType = Solver(srcScriptPubKey, vSolutionsRet);
    std::string strSrcAddrType = GetTxnOutputType(txOutType);
    
    std::vector<unsigned char> srcAddrPKHash;
    if (txOutType == TxoutType::PUBKEY){
        uint160 pubKeyHash = Hash160(vSolutionsRet[0]);
        srcAddrPKHash = std::vector<unsigned char>(pubKeyHash.begin(), pubKeyHash.end());
    } else if(txOutType == TxoutType::PUBKEYHASH){
        srcAddrPKHash =vSolutionsRet[0];
    } else if(txOutType == TxoutType::WITNESS_V0_KEYHASH){
        srcAddrPKHash =vSolutionsRet[0];
    } else {
        //address is unsupported
        std::cout <<  "unsupported address type: " << GetTxnOutputType(txOutType) << std::endl;
        return false;
    }
    
    
    
    const CAmount& satoshis = tx.vout[vout].nValue;
    //use srcAddrPKHash and targetAddrPKHash
    CHashWriter ss(SER_GETHASH, 0);
    ss << srcAddrPKHash;
    ss << targetAddrPKHash;
    ss << satoshis;
    ss << nLockBlockHeight;
    
    std::cout << "[verifier] sig-data srcAddr=" << HexStr(targetAddrPKHash) << std::endl;
    std::cout << "[verifier] sig-data targetAddr=" << HexStr(targetAddrPKHash) << std::endl;
    std::cout << "[verifier] sig-data satoshis=" << satoshis << std::endl;
    std::cout << "[verifier] sig-data nLockBlockHeight=" << nLockBlockHeight << std::endl;
    
    uint256 juratSigDataHash = ss.GetHash();
    std::cout << "[verifier] sig-data hash=" << HexStr(juratSigDataHash) << std::endl;
    
    ///to prevent double counting of two sigs from same key
    std::set<int> keySet;
    verifiedKeys.clear();
    bool verified = true;
    for(int i=1; i<len-1;i=i+2){
        std::vector<unsigned char> sigData = witness.stack[i];
        if(sigData.size() == 0 )
        {
            verified = false;
            break;
        }
        
        int verifierIdx = sigData[sigData.size()-1];
        if(verifierIdx < 0 || verifierIdx >= N_JURAT_KEYS)
        {
            verified = false;
            break;
        }
        
        std::string strVerifierPubKey = judicialVerifierPubKeys[verifierIdx];
        std::vector<unsigned char> vchPubKey =  ParseHex(strVerifierPubKey);
        CPubKey verifierPubKey(vchPubKey.begin(), vchPubKey.end());
        std::vector<unsigned char> vchSignature(sigData.begin(), sigData.end()-1);
        bool isVerified = verifierPubKey.Verify(juratSigDataHash, vchSignature);
        if(!isVerified){
            verified = false;
            break;
        }
        
        std::cout << "[verifier] verified with pub-key: " << strVerifierPubKey << std::endl;
        verifiedKeys.insert(verifiedKeys.begin(),verifierIdx);
        keySet.insert(verifierIdx);
        
    }
    
    if(keySet.size() < N_JURAT_MIN_SIGS){
        verified = false;
    }
    
    std::cout << "[verifier] #sigs verified = " << verifiedKeys.size() << std::endl;
    
    return verified;
}


CScript getMultiSigRedeemScript()
{
    CScript script;
    script << N_JURAT_MIN_SIGS;
    for(int i=0;i<N_JURAT_KEYS;i++){
        std::vector<unsigned char> vchPubKey = ParseHex(judicialVerifierPubKeys[i]);
        script << vchPubKey;
    }
    script << N_JURAT_KEYS << OP_CHECKMULTISIG;
    
    return script;
}

///lightweight verification that checks if this tx is a jurat tx by verifying the a vout with OP_RETURN and jurat marker data
bool JuratVerifier::isJuratTx()
{
    int32_t vin;
    int32_t vout;
    int nLockBlockHeight;
    std::vector<unsigned char> targetAddrPKHash;
    
    bool isJuratTx = identifyJuratTx(m_tx, vin, vout, nLockBlockHeight, targetAddrPKHash);
    return isJuratTx;
}


///does full jurat verification using jurat public keys.
///needs crypto ecdsa library to be initialized before calling this
bool JuratVerifier::verify()
{
    
    std::vector<unsigned char> targetAddrPKHash;
    bool isJuratTx = identifyJuratTx(m_tx, vin, vout, nLockBlockHeight, targetAddrPKHash);
    if(!isJuratTx){
        return false;
    }
    
    isVerified = false;
    verifiedKeys.clear();
    isVerified = verifyJuratTxSig(m_tx, vin, vout, nLockBlockHeight,
                                  prevScriptPubKey,targetAddrPKHash, verifiedKeys);
    
    return isVerified;
}

CScript JuratVerifier::getScriptPubKey()
{
    CScript script = getMultiSigRedeemScript();
    const WitnessV0ScriptHash witnessScriptHash(script);
    std::vector<unsigned char> scriptHash256(witnessScriptHash.begin(), witnessScriptHash.end());
    CScript scriptPubKey;
    scriptPubKey << OP_0 << scriptHash256;
    return scriptPubKey;
    
}

CScriptWitness JuratVerifier::getModifiedWitness()
{
    
    int witness_stack_size = (int) m_tx.vin[vin].scriptWitness.stack.size();
    CScriptWitness witness;
    for( int i=0; i<witness_stack_size; i++){
        if(i%2==1 && i != witness_stack_size-1 ){
            //ignore odd #s except last one
            continue;
        }
        witness.stack.push_back(m_tx.vin[vin].scriptWitness.stack[i]);
    }
    
    return witness;
    
}

}


#endif /* jurat_hpp */
