// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The Syscoin Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SYSCOIN_GOVERNANCEMISC_H
#define SYSCOIN_GOVERNANCEMISC_H

#include <validation.h>
#include <governance.h>
#include <init.h>

class CGovernanceVote;

/**
*	Triggers and Settings - 12.2
*	-----------------------------------
*
*	This allows the network fine grained control of the p2p side, including but not limited to:
*		- Which blocks are valid
*		- What it costs to do various things on the network
*
*/


// class CGovernanceTrigger
// {
// 	static &T IsBlockBanned(int n)
// 	{

// 	}
// };

// /*

	
// */

// class CGovernanceSettings
// {
// 	template<typename T>
// 	// strName=trigger, strParamater=ban-block ... obj= tigger.ban-block(args)
// 	static &T GetSetting(std::string strName, &T networkDefault)
// 	{
// 		/*
// 			- get setting from masternode network
// 		*/

// 		return networkDefault;
// 	}
// };

#endif // SYSCOIN_GOVERNANCEMISC_H
