
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


#endif