
// // Copyright (c) 2014-2017 The Dash Core developers
// // Distributed under the MIT/X11 software license, see the accompanying
// // file COPYING or http://www.opensource.org/licenses/mit-license.php.

// #ifndef GOVERNANCE_KEYS_H
// #define GOVERNANCE_KEYS_H

// #include <string>
// #include <vector>
// #include <map>

// #include <univalue.h>
// #include "support/allocators/secure.h"
// #include ""

// vector<CGovernanceKey> vGovernanceKeys;
// CCriticalSection cs_vGovernanceKeys;

// bool CGovernanceKeyManager::InitGovernanceKeys(std::string strError)
// {

//     {
//         LOCK(cs_vGovernanceKeys);
//         vGovernanceKeys = mapMultiArgs["-addgovkey"];
//     }

//     BOOST_FOREACH(SecureString& strSecure, vGovernanceKeys)
//     {
//     	std::vector<std::string> vecTokenized = SplitBy(strSubCommand, ":");

//     	if(vecTokenized.size() == 2) continue;

// 	    CBitcoinSecret vchSecret;
// 	    bool fGood = vchSecret.SetString(vecTokenized[0]);

// 	    if(!fGood) {
// 	    	strError = "Invalid Governance Key : " + vecTokenized[0];
// 	    	return false;
// 	    }

//     	CGovernanceKey key(vecTokenized[0], vecTokenized[1]);
//     	vGovernanceKeys.push_back(key);
//     }
// }