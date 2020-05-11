#ifndef BITCOIN_QT_OMNICORE_QTUTILS_H
#define BITCOIN_QT_OMNICORE_QTUTILS_H

#include <string>

namespace mastercore
{
    /**
     * Sets up a simple dialog layout that can be used to provide selectable text to the user
     */
    void PopulateSimpleDialog(const std::string& content, const std::string& title, const std::string& tooltip);

    /**
     * Truncates a string at n digits and adds "..." to indicate that display is incomplete
     */
    std::string TruncateString(const std::string& inputStr, unsigned int length);

    /**
    * Strips trailing zeros from a string containing a divisible value
    */
    std::string StripTrailingZeros(const std::string& inputStr);

    /**
    * Displays a 'transaction sent' message box containing the transaction ID and an extra button to copy txid to clipboard
    */
    void PopulateTXSentDialog(const std::string& txidStr);

    /**
    * Variable length find and replace.  Find all iterations of findText within inputStr and replace them
    * with replaceText.
    */
    std::string ReplaceStr(const std::string& findText, const std::string& replaceText, const std::string& inputStr);
}

#endif // BITCOIN_QT_OMNICORE_QTUTILS_H
