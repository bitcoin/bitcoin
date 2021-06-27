// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governance-object.h>
#include <governance/governance-validators.h>

#include <key_io.h>
#include <timedata.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <algorithm>

const size_t MAX_DATA_SIZE = 512;
const size_t MAX_NAME_SIZE = 40;

CProposalValidator::CProposalValidator(const std::string& strHexData, bool fAllowLegacyFormat) :
    objJSON(UniValue::VOBJ),
    fJSONValid(false),
    fAllowLegacyFormat(fAllowLegacyFormat),
    strErrorMessages()
{
    if (!strHexData.empty()) {
        ParseStrHexData(strHexData);
    }
}

void CProposalValidator::ParseStrHexData(const std::string& strHexData)
{
    std::vector<unsigned char> v = ParseHex(strHexData);
    if (v.size() > MAX_DATA_SIZE) {
        strErrorMessages = strprintf("data exceeds %lu characters;", MAX_DATA_SIZE);
        return;
    }
    ParseJSONData(std::string(v.begin(), v.end()));
}

bool CProposalValidator::Validate(bool fCheckExpiration)
{
    if (!fJSONValid) {
        strErrorMessages += "JSON parsing error;";
        return false;
    }
    if (!ValidateType()) {
        strErrorMessages += "Invalid type;";
        return false;
    }
    if (!ValidateName()) {
        strErrorMessages += "Invalid name;";
        return false;
    }
    if (!ValidateStartEndEpoch(fCheckExpiration)) {
        strErrorMessages += "Invalid start:end range;";
        return false;
    }
    if (!ValidatePaymentAmount()) {
        strErrorMessages += "Invalid payment amount;";
        return false;
    }
    if (!ValidatePaymentAddress()) {
        strErrorMessages += "Invalid payment address;";
        return false;
    }
    if (!ValidateURL()) {
        strErrorMessages += "Invalid URL;";
        return false;
    }
    return true;
}

bool CProposalValidator::ValidateType()
{
    int64_t nType;
    if (!GetDataValue("type", nType)) {
        strErrorMessages += "type field not found;";
        return false;
    }

    if (nType != GOVERNANCE_OBJECT_PROPOSAL) {
        strErrorMessages += strprintf("type is not %d;", GOVERNANCE_OBJECT_PROPOSAL);
        return false;
    }

    return true;
}

bool CProposalValidator::ValidateName()
{
    std::string strName;
    if (!GetDataValue("name", strName)) {
        strErrorMessages += "name field not found;";
        return false;
    }

    if (strName.size() > MAX_NAME_SIZE) {
        strErrorMessages += strprintf("name exceeds %lu characters;", MAX_NAME_SIZE);
        return false;
    }

    static const std::string strAllowedChars = "-_abcdefghijklmnopqrstuvwxyz0123456789";

    std::transform(strName.begin(), strName.end(), strName.begin(), ::tolower);

    if (strName.find_first_not_of(strAllowedChars) != std::string::npos) {
        strErrorMessages += "name contains invalid characters;";
        return false;
    }

    return true;
}

bool CProposalValidator::ValidateStartEndEpoch(bool fCheckExpiration)
{
    int64_t nStartEpoch = 0;
    int64_t nEndEpoch = 0;

    if (!GetDataValue("start_epoch", nStartEpoch)) {
        strErrorMessages += "start_epoch field not found;";
        return false;
    }

    if (!GetDataValue("end_epoch", nEndEpoch)) {
        strErrorMessages += "end_epoch field not found;";
        return false;
    }

    if (nEndEpoch <= nStartEpoch) {
        strErrorMessages += "end_epoch <= start_epoch;";
        return false;
    }

    if (fCheckExpiration && nEndEpoch <= GetAdjustedTime()) {
        strErrorMessages += "expired;";
        return false;
    }

    return true;
}

bool CProposalValidator::ValidatePaymentAmount()
{
    double dValue = 0.0;

    if (!GetDataValue("payment_amount", dValue)) {
        strErrorMessages += "payment_amount field not found;";
        return false;
    }

    if (dValue <= 0.0) {
        strErrorMessages += "payment_amount is negative;";
        return false;
    }

    // TODO: Should check for an amount which exceeds the budget but this is
    // currently difficult because start and end epochs are defined in terms of
    // clock time instead of block height.

    return true;
}

bool CProposalValidator::ValidatePaymentAddress()
{
    std::string strPaymentAddress;

    if (!GetDataValue("payment_address", strPaymentAddress)) {
        strErrorMessages += "payment_address field not found;";
        return false;
    }

    if (std::find_if(strPaymentAddress.begin(), strPaymentAddress.end(), ::isspace) != strPaymentAddress.end()) {
        strErrorMessages += "payment_address can't have whitespaces;";
        return false;
    }

    CTxDestination dest = DecodeDestination(strPaymentAddress);
    if (!IsValidDestination(dest)) {
        strErrorMessages += "payment_address is invalid;";
        return false;
    }

    const CScriptID *scriptID = boost::get<CScriptID>(&dest);
    if (scriptID) {
        strErrorMessages += "script addresses are not supported;";
        return false;
    }

    return true;
}

bool CProposalValidator::ValidateURL()
{
    std::string strURL;
    if (!GetDataValue("url", strURL)) {
        strErrorMessages += "url field not found;";
        return false;
    }

    if (std::find_if(strURL.begin(), strURL.end(), ::isspace) != strURL.end()) {
        strErrorMessages += "url can't have whitespaces;";
        return false;
    }

    if (strURL.size() < 4U) {
        strErrorMessages += "url too short;";
        return false;
    }

    if (!CheckURL(strURL)) {
        strErrorMessages += "url invalid;";
        return false;
    }

    return true;
}

void CProposalValidator::ParseJSONData(const std::string& strJSONData)
{
    fJSONValid = false;

    if (strJSONData.empty()) {
        return;
    }

    try {
        UniValue obj(UniValue::VOBJ);

        obj.read(strJSONData);

        if (obj.isObject()) {
            objJSON = obj;
        } else {
            if (fAllowLegacyFormat) {
                std::vector<UniValue> arr1 = obj.getValues();
                std::vector<UniValue> arr2 = arr1.at(0).getValues();
                objJSON = arr2.at(1);
            } else {
                throw std::runtime_error("Legacy proposal serialization format not allowed");
            }
        }

        fJSONValid = true;
    } catch (std::exception& e) {
        strErrorMessages += std::string(e.what()) + std::string(";");
    } catch (...) {
        strErrorMessages += "Unknown exception;";
    }
}

bool CProposalValidator::GetDataValue(const std::string& strKey, std::string& strValueRet)
{
    bool fOK = false;
    try {
        strValueRet = objJSON[strKey].get_str();
        fOK = true;
    } catch (std::exception& e) {
        strErrorMessages += std::string(e.what()) + std::string(";");
    } catch (...) {
        strErrorMessages += "Unknown exception;";
    }
    return fOK;
}

bool CProposalValidator::GetDataValue(const std::string& strKey, int64_t& nValueRet)
{
    bool fOK = false;
    try {
        const UniValue uValue = objJSON[strKey];
        switch (uValue.getType()) {
        case UniValue::VNUM:
            nValueRet = uValue.get_int64();
            fOK = true;
            break;
        default:
            break;
        }
    } catch (std::exception& e) {
        strErrorMessages += std::string(e.what()) + std::string(";");
    } catch (...) {
        strErrorMessages += "Unknown exception;";
    }
    return fOK;
}

bool CProposalValidator::GetDataValue(const std::string& strKey, double& dValueRet)
{
    bool fOK = false;
    try {
        const UniValue uValue = objJSON[strKey];
        switch (uValue.getType()) {
        case UniValue::VNUM:
            dValueRet = uValue.get_real();
            fOK = true;
            break;
        default:
            break;
        }
    } catch (std::exception& e) {
        strErrorMessages += std::string(e.what()) + std::string(";");
    } catch (...) {
        strErrorMessages += "Unknown exception;";
    }
    return fOK;
}

/*
  The purpose of this function is to replicate the behavior of the
  Python urlparse function used by sentinel (urlparse.py).  This function
  should return false whenever urlparse raises an exception and true
  otherwise.
 */
bool CProposalValidator::CheckURL(const std::string& strURLIn)
{
    std::string strRest(strURLIn);
    std::string::size_type nPos = strRest.find(':');

    if (nPos != std::string::npos) {
        if (nPos < strRest.size()) {
            strRest = strRest.substr(nPos + 1);
        } else {
            strRest = "";
        }
    }

    // Process netloc
    if ((strRest.size() > 2) && (strRest.substr(0, 2) == "//")) {
        static const std::string strNetlocDelimiters = "/?#";

        strRest = strRest.substr(2);

        std::string::size_type nPos2 = strRest.find_first_of(strNetlocDelimiters);

        std::string strNetloc = strRest.substr(0, nPos2);

        if ((strNetloc.find('[') != std::string::npos) && (strNetloc.find(']') == std::string::npos)) {
            return false;
        }

        if ((strNetloc.find(']') != std::string::npos) && (strNetloc.find('[') == std::string::npos)) {
            return false;
        }
    }

    return true;
}
