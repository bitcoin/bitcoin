
//     // // ------- Sort budgets by Yes Count

//     // std::vector<std::pair<CGovernanceNode*, int> > vBudgetPorposalsSort;

//     // std::map<uint256, CGovernanceNode>::iterator it = mapGovernanceObjects.begin();
//     // while(it != mapGovernanceObjects.end()){
//     //     (*it).second.CleanAndRemove(false);

//     //     /*
//     //         list offset

//     //         count * 0.00 : Proposal (marked charity)  :   0% to 05%
//     //         count * 0.05 : Proposals                  :   5% to 100%
//     //         count * 1.00 : Contracts                  :   100% to 200%
//     //         count * 2.00 : High Priority Contracts    :   200%+
//     //     */
//     //     int nOffset = 0;

//     //     if((*it).second.GetGovernanceType() == Setting) {it++; continue;}
//     //     if((*it).second.GetGovernanceType() == Proposal) nOffset += 0;
//     //     if((*it).second.GetGovernanceType() == Contract) nOffset += mnodeman.CountEnabled();
        
//     //     vBudgetPorposalsSort.push_back(make_pair(&((*it).second), nOffset+((*it).second.GetYesCount()-(*it).second.GetNoCount())));

//     //     ++it;
//     // }
   
//     }

//     /*
//         TODO: There might be an issue with multisig in the coinbase on mainnet, we will add support for it in a future release.
//     */
