#pragma once

#include <string>

// Forward Declarations
class CTransaction;

namespace MWEB {

class Policy
{
public:
    /// <summary>
    /// Checks the transaction for violation of any MWEB-specific standard tx policies.
    /// </summary>
    /// <param name="tx">The transaction to check.</param>
    /// <param name="reason">The reason it's non-standard, if any.</param>
    /// <returns>True if the transaction is standard.</returns>
    static bool IsStandardTx(const CTransaction& tx, std::string& reason);
};

}