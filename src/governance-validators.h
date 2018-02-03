// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GOVERNANCE_VALIDATORS_H
#define GOVERNANCE_VALIDATORS_H

#include <string>

#include <univalue.h>

class CProposalValidator  {
public:
    CProposalValidator(const std::string& strDataHexIn = std::string());

    void Clear();

    void SetHexData(const std::string& strDataHexIn);

    bool Validate();

    bool ValidateJSON();

    bool ValidateName();

    bool ValidateStartEndEpoch();

    bool ValidatePaymentAmount();

    bool ValidatePaymentAddress();

    bool ValidateURL();

    const std::string& GetErrorMessages()
    {
        return strErrorMessages;
    }

private:
    void ParseJSONData();

    bool GetDataValue(const std::string& strKey, std::string& strValue);

    bool GetDataValue(const std::string& strKey, int64_t& nValue);

    bool GetDataValue(const std::string& strKey, double& dValue);

    static std::string StripWhitespace(const std::string& strIn);

    static bool CheckURL(const std::string& strURLIn);

private:
    std::string            strData;

    UniValue               objJSON;

    bool                   fJSONValid;

    std::string            strErrorMessages;

};

#endif
