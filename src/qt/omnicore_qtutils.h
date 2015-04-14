#ifndef OMNICORE_QTUTILS
#define OMNICORE_QTUTILS

#include <QDialog>
#include <string>

namespace mastercore
{
    /**
     * Sets up a simple dialog layout that can be used to provide selectable text to the user
     */
    void PopulateSimpleDialog(const std::string& content, const std::string& title, const std::string& tooltip);

    /**
    * Strips trailing zeros from a string containing a divisible value
    */
    std::string StripTrailingZeros(const std::string& inputStr);

    /**
    * Displays a 'transaction sent' message box containing the transaction ID and an extra button to copy txid to clipboard
    */
    void PopulateTXSentDialog(const std::string& txidStr);
}

#endif // OMNICORE_QTUTILS
