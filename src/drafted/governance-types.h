// Copyright (c) 2014-2016 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERNANCE_TYPES_H
#define GOVERNANCE_TYPES_H

#define VOTE_ABSTAIN  0
#define VOTE_YES      1
#define VOTE_NO       2

// note: is there a reason these are static? 
//         http://stackoverflow.com/questions/3709207/c-semantics-of-static-const-vs-const 
static const CAmount BUDGET_FEE_TX = (5*COIN);
static const int64_t BUDGET_FEE_CONFIRMATIONS = 6;
static const int64_t BUDGET_VOTE_UPDATE_MIN = 60*60;
static const int64_t CONTRACT_ACTIVATION_TIME = 60*60*24*14;

// OVERRIDABLE TRAITS

const int GOVERNANCE_TRAIT_LEVEL = 10001;
const int GOVERNANCE_TRAIT_CATEGORY = 10002;
const int GOVERNANCE_TRAIT_STATUS = 10003;
const int GOVERNANCE_TRAIT_STATUS_MESSAGE = 10004;

// see govman.dash.org - 52.90.159.142

/*
    Main governance types are 1-to-1 matched with governance classes 
        - subtypes like a DAO is a categorical classification (extendable)
        - see governance-classes.h for more information
*/

enum GovernanceObjectType {
    // Programmatic Functionality Types
        Root = -3,
        AllTypes = -2,
        Error = -1,
    // --- Zero ---

    // Actions
    ValueOverride = 1, 

    // -------------------------------
    // DashNetwork - is the root node
    DashNetwork = 1000,
    DashNetworkVariable = 1001,
    Category = 1002,

    // Actors
    //   -- note: DAOs can't own property... property must be owned by
    //   --     legal entities in the local jurisdiction 
    //   --    this is the main reason for a two tiered company structure
    //   --  people that operate DAOs will own companies which can own things legally
    Group = 2000,
    User = 2001,
    Company = 2002,

    // Project - Generic Base
    Project = 3000,
    ProjectReport = 3001,
    ProjectMilestone = 3002,
    
    // Budgets & Funding
    Proposal = 4000,
    Contract = 4001
};

// Functions for converting between the governance types and strings

extern GovernanceObjectType GovernanceStringToType(std::string strType);
extern std::string GovernanceTypeToString(GovernanceObjectType type);

// Payments for a finalized budget

class CTxBudgetPayment
{
public:
    uint256 nProposalHash;
    CScript payee;
    CAmount nAmount;

    CTxBudgetPayment() {
        payee = CScript();
        nAmount = 0;
        nProposalHash = uint256();
    }

    ADD_SERIALIZE_METHODS;

    //for saving to the serialized db
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(CScriptBase*)(&payee));
        READWRITE(nAmount);
        READWRITE(nProposalHash);
    }
};


#endif