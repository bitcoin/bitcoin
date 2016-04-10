
/*
	CATEGORY MAPPING

	* means the category has an associated class
	
	DASH NETWORK (ROOT)
		-> NETWORK VARIABLE
			-> switch, setting
		-> CATEGORIES
			-> LEVEL
				-> I, II, III, IV, V, VI, VII, VIII, IX, X, XI
			-> VALUEOVERRIDE
				-> NETWORK, OWNER 
			-> ACTOR
				-> GROUP*
					-> CORE, NONCORE
				-> USER*
					-> CORE, NONCORE
				-> COMPANY* / ORGANIZATION
					-> DAO
					-> COMMITTEE
						-> BUSINESS, RESEARCH, DEVELOPMENT, AMBASSADOR
					-> FORPROFIT
						-> LLC, INC
					-> NOTFORPROFIT
						-> 501c3, 501c6
			-> PROJECT*
				-> TYPES
					-> SOFTWARE
						-> CORE, NONCORE
					-> HARDWARE
					-> PR
				-> PROJECT REPORT*
					-> UPDATE
				-> PROJECT MILESTONE*
					-> START, ONGOING, COMPLETE, FAILURE
				-> PROPOSAL*
					-> FUNDING, GOVERNANCE, AMEND, GENERIC
				-> CONTRACT*
					-> TYPE
						-> INTERNAL, EXTERNAL
					-> STATUS
						-> OK
		-> GROUPS
			-> GROUP1
				-> USER1 (only users are allowed here in this scope)
				-> USER2
			-> GROUP2 (EVO)
				-> VALUEOVERRIDE (STORE=DASHDRIVE)
				-> USER1

		-> COMPANIES
			-> COMPANY1
			-> DAO1


*/

class CGovernanceCategories:

    vector<Category> Find(GovernanceObjectType):
        for every obj:
            if obj.type == typein and obj.parent == root:
                add obj
        return std::vector<cat> v;