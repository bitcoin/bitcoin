// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governanceexceptions.h>

#include <iostream>
#include <sstream>

std::ostream& operator<<(std::ostream& os, governance_exception_type_enum_t eType)
{
    switch (eType) {
    case GOVERNANCE_EXCEPTION_NONE:
        os << "GOVERNANCE_EXCEPTION_NONE";
        break;
    case GOVERNANCE_EXCEPTION_WARNING:
        os << "GOVERNANCE_EXCEPTION_WARNING";
        break;
    case GOVERNANCE_EXCEPTION_PERMANENT_ERROR:
        os << "GOVERNANCE_EXCEPTION_PERMANENT_ERROR";
        break;
    case GOVERNANCE_EXCEPTION_TEMPORARY_ERROR:
        os << "GOVERNANCE_EXCEPTION_TEMPORARY_ERROR";
        break;
    case GOVERNANCE_EXCEPTION_INTERNAL_ERROR:
        os << "GOVERNANCE_EXCEPTION_INTERNAL_ERROR";
        break;
    }
    return os;
}

CGovernanceException::CGovernanceException(const std::string& strMessageIn,
                                           governance_exception_type_enum_t eTypeIn,
                                           int nNodePenaltyIn) :
    strMessage(),
    eType(eTypeIn),
    nNodePenalty(nNodePenaltyIn)
{
    std::ostringstream ostr;
    ostr << eType << ":" << strMessageIn;
    strMessage = ostr.str();
}
