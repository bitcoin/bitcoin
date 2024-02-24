// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GOVERNANCE_EXCEPTIONS_H
#define BITCOIN_GOVERNANCE_EXCEPTIONS_H

#include <exception>
#include <iosfwd> // for ostream
#include <string>

enum governance_exception_type_enum_t {
    /// Default value, normally indicates no exception condition occurred
    GOVERNANCE_EXCEPTION_NONE = 0,
    /// Unusual condition requiring no caller action
    GOVERNANCE_EXCEPTION_WARNING = 1,
    /// Requested operation cannot be performed
    GOVERNANCE_EXCEPTION_PERMANENT_ERROR = 2,
    /// Requested operation not currently possible, may resubmit later
    GOVERNANCE_EXCEPTION_TEMPORARY_ERROR = 3,
    /// Unexpected error (ie. should not happen unless there is a bug in the code)
    GOVERNANCE_EXCEPTION_INTERNAL_ERROR = 4
};

std::ostream& operator<<(std::ostream& os, governance_exception_type_enum_t eType);

/**
 * A class which encapsulates information about a governance exception condition
 *
 * Derives from std::exception so is suitable for throwing
 * (ie. will be caught by a std::exception handler) but may also be used as a
 * normal object.
 */
class CGovernanceException : public std::exception
{
private:
    std::string strMessage;

    governance_exception_type_enum_t eType;

    int nNodePenalty;

public:
    explicit CGovernanceException(const std::string& strMessageIn = "",
        governance_exception_type_enum_t eTypeIn = GOVERNANCE_EXCEPTION_NONE,
        int nNodePenaltyIn = 0);

    ~CGovernanceException() noexcept override = default;

    const char* what() const noexcept override
    {
        return strMessage.c_str();
    }

    const std::string& GetMessage() const
    {
        return strMessage;
    }

    governance_exception_type_enum_t GetType() const
    {
        return eType;
    }

    int GetNodePenalty() const
    {
        return nNodePenalty;
    }
};

#endif // BITCOIN_GOVERNANCE_EXCEPTIONS_H
