#ifndef SYSCOIN_TEST_TEST_SYSCOIN_SERVICES_H
#define SYSCOIN_TEST_TEST_SYSCOIN_SERVICES_H

#include <stdio.h>
#include <univalue.h>
using namespace std;
/** Testing syscoin services setup that configures a complete environment with 3 nodes.
 */
UniValue CallRPC(const string &dataDir, const string& commandWithArgs, bool regTest = true, bool readJson = true);
void StartNode(const string &dataDir, bool regTest = true, const string& extraArgs="");
void StopNode(const string &dataDir="node1");
void StartNodes();
void StartMainNetNodes();
void StopMainNetNodes();
void StopNodes();
void GenerateBlocks(int nBlocks, const string& node="node1");
void GenerateMainNetBlocks(int nBlocks, const string& node);
string CallExternal(string &cmd);
void ExpireAlias(const string& alias);
void GetOtherNodes(const string& node, string& otherNode1, string& otherNode2);
string AliasNew(const string& node, const string& aliasname, const string& pubdata, string privdata="\"\"", string witness="\"\"");
string AliasUpdate(const string& node, const string& aliasname, const string& pubdata="\"\"", const string& privdata="\"\"", string addressStr = "\"\"", string witness="\"\"");
string AliasTransfer(const string& node, const string& aliasname, const string& tonode, const string& pubdata="\"\"", const string& privdata="\"\"", const string& witness="\"\"");
bool AliasFilter(const string& node, const string& regex);
const string CertNew(const string& node, const string& alias, const string& title, const string& data, const string& pubdata, const string& witness="\"\"");
void CertUpdate(const string& node, const string& guid, const string& title="\"\"", const string& pubdata="\"\"", const string& data="\"\"", const string& witness="\"\"");
void CertTransfer(const string& node, const string& tonode, const string& guid, const string& toalias, const string& witness="\"\"");
bool CertFilter(const string& node, const string& regex);
bool EscrowFilter(const string& node, const string& regex);
void CreateSysRatesIfNotExist();
const UniValue FindFeedback(const string& node, const string& txid);
const string OfferNew(const string& node, const string& aliasname, const string& category, const string& title, const string& qty, const string& price, const string& description, const string& currency, const string& certguid="\"\"", const string& paymentoptions="\"\"",  const string& witness="\"\"");
void OfferUpdate(const string& node, const string& aliasname, const string& offerguid,  const string& category="\"\"", const string& title="\"\"", const string& qty="\"\"", const string& price="\"\"", const string& description="\"\"", const string& currency="\"\"", const string &isprivate="\"\"", const string& certguid="\"\"",  const string &commission="\"\"", const string &paymentoptions="\"\"", const string& witness="\"\"");
bool OfferFilter(const string& node, const string& regex);
void OfferAddWhitelist(const string& node,const string& offer, const string& aliasname, const string& discount, const string& witness="\"\"");
void OfferRemoveWhitelist(const string& node, const string& offer, const string& aliasname, const string& witness="\"\"");
void OfferClearWhitelist(const string& node, const string& offer, const string& witness="\"\"");
void EscrowFeedback(const string& node, const string& role, const string& escrowguid, const string& feedback, const string& rating, char user, const string& witness="\"\"");
const string OfferLink(const string& node, const string& alias, const string& guid, const string& commission, const string& newdetails, const string& witness="\"\"");
const string OfferAccept(const string& ownernode, const string& node, const string& aliasname, const string& offerguid, const string& qty, const string& discountexpected = "\"\"", const string& witness="\"\"");
const string EscrowNew(const string& node, const string& sellernode, const string& buyeralias, const string& offerguid, const string& qty, const string& arbiteralias, const string& discountexpected="\"\"", const string& witness="\"\"");
void EscrowRelease(const string& node, const string& role, const string& guid, const string& witness="\"\"");
void EscrowClaimRelease(const string& node, const string& role, const string& guid);
void EscrowRefund(const string& node, const string& role, const string& guid, const string& witness="\"\"");
void EscrowClaimRefund(const string& node, const string& role, const string& guid);
// SYSCOIN testing setup
struct SyscoinTestingSetup {
    SyscoinTestingSetup();
    ~SyscoinTestingSetup();
};
struct BasicSyscoinTestingSetup {
    BasicSyscoinTestingSetup();
    ~BasicSyscoinTestingSetup();
};
struct SyscoinMainNetSetup {
	SyscoinMainNetSetup();
	~SyscoinMainNetSetup();
};
#endif
