#include "mastercore_parse_string.h"

#include <boost/lexical_cast.hpp>

#include <stdint.h>

#include <algorithm>
#include <string>

namespace mastercore
{
int64_t StrToInt64(const std::string& str, bool divisible)
{    
    // copy original, so it remains unchanged
    std::string strAmount (str);
    int64_t nAmount = 0;
    
    // check for a negative (minus sign) and invalidate if present
    size_t negSignPos = strAmount.find("-");
    if (negSignPos != std::string::npos) return 0;

    // convert the string into a usable int64
    if (divisible) {
        // check for existance of decimal point
        size_t pos = strAmount.find(".");
        if (pos == std::string::npos) {
            // no decimal point but divisible so pad 8 zeros on right
            strAmount += "00000000";
        } else {
            // check for existence of second decimal point, if so invalidate amount
            size_t posSecond = strAmount.find(".", pos + 1);
            if (posSecond != std::string::npos) return 0;
            
            if ((strAmount.size() - pos) < 9) {
                // there are decimals either exact or not enough, pad as needed
                std::string strRightOfDecimal = strAmount.substr(pos + 1);
                unsigned int zerosToPad = 8 - strRightOfDecimal.size();
                
                // do we need to pad?
                if (zerosToPad > 0) 
                {
                    for (unsigned int it = 0; it != zerosToPad; it++) {
                        strAmount += "0";
                    }
                }
            } else {
                // there are too many decimals, truncate after 8
                strAmount = strAmount.substr(0, pos + 9);
            }
        }
        strAmount.erase(std::remove(strAmount.begin(), strAmount.end(), '.'), strAmount.end());
        try {
            nAmount = boost::lexical_cast<int64_t>(strAmount);
        } catch (const boost::bad_lexical_cast &e) {}
    } else {
        size_t pos = strAmount.find(".");
        std::string newStrAmount = strAmount.substr(0, pos);
        try {
            nAmount = boost::lexical_cast<int64_t>(newStrAmount);
        } catch (const boost::bad_lexical_cast &e) {}
    }

    return nAmount;
}

// Strip trailing zeros from a string containing a divisible value - TEMP - MOVE TO UTILS WHEN MERGED
std::string StripTrailingZeros(const std::string& inputStr)
{
    size_t dot = inputStr.find(".");
    std::string outputStr = inputStr; // make a copy we will manipulate and return
    if (dot==std::string::npos) { // could not find a decimal marker, unsafe - return original input string
        return inputStr;
    }
    size_t lastZero = outputStr.find_last_not_of('0') + 1;
    if (lastZero > dot) { // trailing zeros are after decimal marker, safe to remove
        outputStr.erase ( lastZero, std::string::npos );
        if (outputStr.length() > 0) { std::string::iterator it = outputStr.end() - 1; if (*it == '.') { outputStr.erase(it); } } //get rid of trailing dot if needed
    } else { // last non-zero is before the decimal marker, this is a whole number
        outputStr.erase ( dot, std::string::npos );
    }
    return outputStr;
}

} // namespace mastercore
