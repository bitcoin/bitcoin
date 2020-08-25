// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <hash.h>
#include <messagesigner.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <key_io.h>
#include <util/message.h>
bool CMessageSigner::GetKeysFromSecret(const std::string& strSecret, CKey& keyRet, CPubKey& pubkeyRet)
{
    keyRet = DecodeSecret(strSecret);
    pubkeyRet = keyRet.GetPubKey();
    return true;
}
bool CMessageSigner::SignMessage(const std::string& strMessage, std::vector<unsigned char>& vchSigRet, const CKey& key)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << MESSAGE_MAGIC;
    ss << strMessage;

    return CHashSigner::SignHash(ss.GetSHA256(), key, vchSigRet);
}

bool CMessageSigner::VerifyMessage(const CPubKey& pubkey, const std::vector<unsigned char>& vchSig, const std::string& strMessage)
{
    return VerifyMessage(pubkey.GetID(), vchSig, strMessage);
}

bool CMessageSigner::VerifyMessage(const CKeyID& keyID, const std::vector<unsigned char>& vchSig, const std::string& strMessage)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << MESSAGE_MAGIC;
    ss << strMessage;

    return CHashSigner::VerifyHash(ss.GetSHA256(), keyID, vchSig);
}
bool CHashSigner::SignHash(const uint256& hash, const CKey& key, std::vector<unsigned char>& vchSigRet)
{
    return key.SignCompact(hash, vchSigRet);
}

bool CHashSigner::VerifyHash(const uint256& hash, const CPubKey& pubkey, const std::vector<unsigned char>& vchSig)
{
    return VerifyHash(hash, pubkey.GetID(), vchSig);
}

bool CHashSigner::VerifyHash(const uint256& hash, const CKeyID& keyID, const std::vector<unsigned char>& vchSig)
{
    CPubKey pubkeyFromSig;
    if(!pubkeyFromSig.RecoverCompact(hash, vchSig)) {
        return false;
    }

    if(pubkeyFromSig.GetID() != keyID) {
        return false;
    }

    return true;
}
