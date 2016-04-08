
#include "governance-classes.h"

// void CGovernanceNodeBroadcast::Relay()
// {
//     CInv inv(MSG_GOVERNANCE_OBJECT, GetHash());
//     RelayInv(inv, MIN_BUDGET_PEER_PROTO_VERSION);
// }

bool CreateNewGovernanceObject(UniValue& govObjJson, CGovernanceNode& govObj, GovernanceObjectType govType, std::string& strError)
{
    if(govType == DashNetwork)
	{
        govObj = CDashNetwork(govObjJson);
        return true;
	}
    // if(govType == DashNetworkVariable)
    // {
    //     govObj = CDashNetworkVariable(govObjJson);
    // }
    // if(govType == Category)
    // {
    //     govObj = CCategory(govObjJson);
    // }
    // if(govType == Group)
    // {
    //     govObj = CGroup(govObjJson);
    // }
    // if(govType == User)
    // {
    //     govObj = CUser(govObjJson);
    // }
    // if(govType == Company)
    // {
    //     govObj = CCompany(govObjJson);
    // }
    // if(govType == Project)
    // {	
    //     govObj = CProject(govObjJson);
    // }
    // if(govType == Project)
    // {
    //     govObj = CProject(govObjJson);
    // }
    // if(govType == ProjectReport)
    // {
    //     govObj = CProjectReport(govObjJson);
    // }
    // if(govType == ProjectMilestone)
    //     govObj = CProjectMilestone(govObjJson);
    // if(govType == Proposal)
    //     govObj = CProposal(govObjJson);
    // if(govType == Contract)
    //     govObj = CContract(govObjJson);
    // if(govType == ValueOverride)
    //     govObj = CValueOverride(govObjJson);
}