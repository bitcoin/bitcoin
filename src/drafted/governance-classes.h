// Copyright (c) 2014-2017 The Dash Core developers

/*
 * FIELDS AND CLASSIFICATION
 * --------------------
 *
 *   -- fields can be updated by the network dynamically (adding a company type, etc)
 *   -- fields can be voted on, leveling them up/down
 *   -- levels determine pay in DO/DAO company types 
 *   
 * FIELDS
 * =========================
 *
 *   network-type mainnet, testnet, regtest
 *   actor-type group, user, dao, company, committee, foundation
 *   group-type core, noncore
 *   dao-type none
 *   company-type us.llc, us.501c3, us.501c6, us.inc, etc
 *   committee-type science, technology, economics
 *   foundation-type us.501c6, us.501c3
 *   contract-type blockchain/internal, external/contractor
 *   proposal-type explicit, governance, wikiamend, generic, black
 *   
 *   lvl 1,2,3,4.. roman
 *   status ok, error, active-removal
 *   proposal-rights explicit, explicit_generic, all
 *   contract-rights internal, external
 *   status-error missing-documentation, doa, report-outstanding
 *   milestone-status research, hiring, ongoing, complete, failure, overdue, error //programmatic workflow?
 *   milestone-status-error mia, option2, option3
 *   network-status ok, error
 *   network-error none, fork-detected, debug, full-blocks, network-outage, spam
 *   foundation-type maintainance, r&d, awareness, bridge(legacy), philanthropic, legal
 *   committee-type business, sciencific_advisory, research
 *   global-type-variable switch, int, string, enum // enum should be defined by another category
 *   category-type primary, secondary, tertiary, quaternary, quinary, senary, septenary, octonary
 *   contract-status ok, error
 *   cantract-status-error none, error1, error2
 *   committee-type research, business_advisory, economic_advisory
 *
 *   // note: How enums work
 *   enum-type one, two, three //defined in code, or the defintion could be packed ino this field
 *
 * CLASSES
 * =========================
 *
 *   // network
 *   CDashNetwork lvl, network-type, network-status, network-error, milestone-status*
 *   CCategory lvl, category-type, status, status-error
 *   CNetworkGlobalVariable lvl, global-type, status, status-error
 *   // base: actor
 *   CGroup lvl, actor-type, status, status-error, group-type
 *   CUser lvl, actor-type, status, status-error, user-type, contract-status, contract-status-error 
 *   CDAO lvl, actor-type, status, status-error, dao-type
 *   CCompany lvl, actor-type, secondary-type, ternary-type, status, status-error
 *   CCommittee lvl, actor-type, status, status-error, committee-type ov 
 *   CFoundation lvl, actor-type, status, status-error, foundation-type ov
 *   // base: project manangement
 *   CProposal lvl, proposal-type, status, status-error
 *   CContract lvl, contract-type, status, status-error, proposal-rights, contract-rights
 *   CProject lvl, project-type, status, status-error
 *   CProjectReport lvl, report-type, status, status-error
 *   CProjectMilestone lvl, milestone-type, status, status-error, milestone-status, milestone-status-error, ov
 *   CValueOverride lvl, vo-type, status, status-error
*/



/*
 *  CLASS INHERITANCE
 *  ============================================================
 *
 *   -- Each of the implementable classes use their own serializers
 *   -- Each class is responsible for it's own unique values
 *   -- Most of the values in these classes can be overriden 
 *
 *  CGovernanceNode (base)
 *
 *  TREE STRUCTURE
 *  ===========================================
 * 
 *  DASH NETWORK (ROOT)
 *      -> NETWORK GLOBOLS
 *          -> SWITCHES, SETTINGS
 *      -> CATEGORIES
 *          -> CATEGORY (DAO)
 *              -> CATEGORIES ()
 *          -> CATEGORY (CONTRACT)
 *              -> CATEGORIES (INTERNAL, EXTERNAL, ...)
 *      -> GROUPS
 *          -> GROUP 1 
 *              -> USER : ENDUSER
 *      -> COMPANIES
 *          -> DAO
 *              -> COMPANY, COMMITTEE, FOUNDATION, ..
 *              -> GROUP1 (CORE)
 *                  - USER : EVAN DUFFIELD
 *                      -> CONTRACT1 (INTERNAL CONTRACT)
 *                      -> PROJECT1
 *              -> PROJECT1
 *                   -> CONTRACT2 (EXTERNAL CONTRACT)
 *                   -> PROPOSAL (GENERIC FUNDING)
 *                       -> VO (OUTPUT VALUE == 3.23) // NETWORK OVERRIDE
 *                   -> REPORT1
 *                   -> REPORT2
 *                   -> REPORT3
 *                   -> MILESTONE1
 *                   -> MILESTONE2
 *                       -> OV (STATUS=OVERDUE)
**/


class CGovernanceObject : public CGovernanceNode
{
private:
    // some minimal caching is supported here
    int nLevel;
    std::string strCategory;

    // Current OBJECT STATUS (see http://govman.dash.org/index.php/Documentation_:_Status_Field)
    int nStatusID;
    std::string strStatusMessage;

    // minimal caching
    uint64_t nTimeValueOverrideCached;

public:

    virtual uint256 GetHash() = 0;
};

// // root node
class CDashNetwork : public CGovernanceObject
{
private:
    std::string strName;
    std::string strURL;


public:
    CDashNetwork(UniValue objIn)
    {
        strName = objIn["name"].get_str();
        strURL = objIn["name"].get_str();

        // we should pop these off one by one and check if the objIn.size() == 0
    }

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << strName;
        ss << strURL;
        ss << nTime;
        ss << vecSig;
        ss << nGovernanceType;
        uint256 h1 = ss.GetHash();

        return h1;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        // TODO : For testnet version bump
        READWRITE(nGovernanceType);

        READWRITE(LIMITED_STRING(strName, 20));
        READWRITE(LIMITED_STRING(strURL, 64));
        READWRITE(nTime);
        READWRITE(vecSig);
        READWRITE(nCollateralHash);
    }

};

// // can be under: DashNetwork
// //   -- signature requirements : Key1(User)
// class CDashNetworkVariable : public CGovernanceObject
// {
// private:

// public:

//     virtual uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     virtual inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO : For testnet version bump
//         READWRITE(nGovernanceType);

//         READWRITE(LIMITED_STRING(strName, 20));
//         READWRITE(LIMITED_STRING(strURL, 64));
//         READWRITE(nTime);
//         READWRITE(vecSig);
//         READWRITE(nCollateralHash);
//     }

// };


// /*
//     ValueOverride

// */

// // can be under: ANY
// //   -- signature requirements : Key1(User)
// template <typename VO> 
// class CValueOverride : public CGovernanceObject
// {

//     // bool GetValues(VO& a)
//     // {
//     //     return nValue1;
//     // }

//     // bool GetValues2(VO& a, VO& b)
//     // {
//     //     return nValue1;
//     // }


//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     virtual inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO : For testnet version bump
//         READWRITE(nGovernanceType);

//         READWRITE(LIMITED_STRING(strName, 20));
//         READWRITE(LIMITED_STRING(strURL, 64));
//         READWRITE(nTime);
//         READWRITE(vecSig);
//         READWRITE(nCollateralHash);
//     }

// };


// class CCategory : public CGovernanceObject
// {
//     // **** Statistics / Information ****

//     CAmount   GetRequiredFeeAmount() {return 1.00;}
//     CCategory GetLevel() {return (CCategory)GetCategory(0);}
//     CCategory GetCategoryType() {return (CCategory)GetCategory(1);}
//     CCategory GetStatus() {return (CCategory)GetCategory(2);}
//     CCategory GetStatusError() {return (CCategory)GetCategory(3);}

//     // isRootCategory()
//     // {
//     //     // root categories won't have categories as parents
//     //     return (IsType() == DashNetwork);
//     // }

//     // isSubcategoryOf(std::string strParentName)
//     // {
//     //     CCategory parent(strParentName);
//     //     if(!parent) return false;
//     //     return isSubcategoryOf(parent);        
//     // }

//     // isSubcategoryOf(CCategory parentIn)
//     // {
//     //     // are we related to this category?
//     //     if parent.GetHash() == pParent->GetHash():
//     //         return true
        
//     //     return false;
//     // }

//     // **** Governance Permissions ****

//     // only allow categories under categories
//     virtual bool CanAdd(CCategory obj) {return true;}
//     virtual bool RequiresSignatureToAddChild(CCategory obj) {return false;}

//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     virtual inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO : For testnet version bump
//         READWRITE(nGovernanceType);

//         READWRITE(LIMITED_STRING(strName, 20));
//         READWRITE(LIMITED_STRING(strURL, 64));
//         READWRITE(nTime);
//         READWRITE(vecSig);
//         READWRITE(nCollateralHash);
//     }
// };

// // base: actor class

// class CGovernanceActor : public CGovernanceObject
// {
//     /*

//         ???

//         GetAverageMonthlySpending();
//         GetYearlySpent();
//         GetContractCount(CCategory category)
//     */
// };

// class CGroup : public CGovernanceActor

//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

// };

// class CUser : public CGovernanceActor
// {

// public:
//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     virtual inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO : For testnet version bump
//         READWRITE(nGovernanceType);

//         READWRITE(LIMITED_STRING(strName, 20));
//         READWRITE(LIMITED_STRING(strURL, 64));
//         READWRITE(nTime);
//         READWRITE(vecSig);
//         READWRITE(nCollateralHash);
//     }

// };

// // base: actor classes

// class CCompany : public CGovernanceActor
// {

//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     virtual inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO : For testnet version bump
//         READWRITE(nGovernanceType);

//         READWRITE(LIMITED_STRING(strName, 20));
//         READWRITE(LIMITED_STRING(strURL, 64));
//         READWRITE(nTime);
//         READWRITE(vecSig);
//     }
        
// };

// class CProject : public CGovernanceObject
// {
// public:
//     std::string strName;
//     std::string GetURL();

//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     virtual inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO : For testnet version bump
//         READWRITE(nGovernanceType);

//         READWRITE(LIMITED_STRING(strName, 20));
//         READWRITE(LIMITED_STRING(strURL, 64));
//         READWRITE(nTime);
//         READWRITE(vecSig);
//     }

// };


// class CProjectReport : public CGovernanceObject
// {
// private:
//     std::string strName;
//     std::string GetURL;

// public:
//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     virtual inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO : For testnet version bump
//         READWRITE(nGovernanceType);

//         READWRITE(LIMITED_STRING(strName, 20));
//         READWRITE(LIMITED_STRING(strURL, 64));
//         READWRITE(nTime);
//         READWRITE(vecSig);
//     }

// };

// class CProjectMilestone : public CGovernanceObject
// {
// private:
//     // specialized class variables

// public:
//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     virtual inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO : For testnet version bump
//         READWRITE(nGovernanceType);

//         READWRITE(LIMITED_STRING(strName, 20));
//         READWRITE(LIMITED_STRING(strURL, 64));
//         READWRITE(nTime);
//         READWRITE(vecSig);
//     }

// };

// class CProposal : public CGovernanceObject
// {
// private:
//     // specialized class variables

//     std::string strName;
//     std::string strURL;
//     /*
//         proposal will be paid on this block
//             -- if it's not paid, it will expire unpaid
//     */ 
//     int nBlockStart;
//     CAmount nAmount;
//     CScript address;

// public:
//     // **** Statistics / Information ****

//     std::string GetName() {return strName; }
//     CScript GetPayee() {return address;}
//     std::string GetURL() {return strURL; }

//     // nothing can be under a proposal

//     // signatures required for everything
//     virtual bool RequiresSignatureToAddChild(CGovernanceNode obj) {return true;}

//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     virtual inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO : For testnet version bump
//         READWRITE(nGovernanceType);

//         READWRITE(LIMITED_STRING(strName, 20));
//         READWRITE(LIMITED_STRING(strURL, 64));
//         READWRITE(nTime);
//         READWRITE(vecSig);
//     }
// };


// class CContract : public CGovernanceObject
// {
// private:
//     std::string strName;
//     std::string strURL;
//     int nBlockStart; //starting block
//     int months_active; //nBlockEnd = nBlockStart + (blocks-per-month * months_active)
//     CAmount nAmount;
//     CScript address;


// public:

//     uint256 GetHash(){
//         CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
//         ss << strName;
//         ss << strURL;
//         ss << nBlockStart;
//         ss << nBlockEnd;
//         ss << nAmount;
//         ss << *(CScriptBase*)(&address);
//         ss << nGovernanceType;
//         uint256 h1 = ss.GetHash();

//         return h1;
//     }

//     ADD_SERIALIZE_METHODS;

//     template <typename Stream, typename Operation>
//     virtual inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
//         // TODO : For testnet version bump
//         READWRITE(nGovernanceType);

//         READWRITE(LIMITED_STRING(strName, 20));
//         READWRITE(LIMITED_STRING(strURL, 64));
//         READWRITE(nTime);
//         READWRITE(vecSig);
//     }

// };


bool CreateNewGovernanceObject(UniValue& govObjJson, CGovernanceNode& govObj, GovernanceObjectType govType, std::string& strError);


#endif
