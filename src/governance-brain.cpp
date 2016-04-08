
#include "governance-brain.h"

/*
enum GovernanceObjectType {
        AllTypes = -2,
        Error = -1,

    ValueOverride = 1, 
    DashNetwork = 1000,
    DashNetworkVariable = 1001,

    Category = 1002,
    Group = 2000,
    User = 2001,
    Company = 2002,

    Project = 3000,
    ProjectReport = 3001,
    ProjectMilestone = 3002,
    
    Proposal = 4000,
    Contract = 4001
};
*/

bool CGovernanceBrain::IsPlacementValid(GovernanceType childType, GovernanceType parentType)
{
    switch(parentType)
    {
        case(None):
            if(childType == DashNetwork) return true;
            return false;
        case(DashNetwork):
            if(childType == Company) return true;
            if(childType == Group) return true;
            if(childType == DashNetworkVariable) return true;
            return false;
        case(Group):
            if(childType == Group) return true;
            return false; 
        case(None):
            if(childType == None) return true;
            return false; 
        case(None):
            if(childType == None) return true;
            return false; 
        case(None):
            if(childType == None) return true;
            return false; 
    }
    if(parentType == None && childType == DashNetwork) return true;
}