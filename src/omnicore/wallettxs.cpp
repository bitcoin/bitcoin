#include "omnicore/wallettxs.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/rules.h"
#include "omnicore/script.h"
#include "omnicore/utilsbitcoin.h"

#include "amount.h"
#include "base58.h"
#include "coincontrol.h"
#include "init.h"
#include "main.h"
#include "script/standard.h"
#include "sync.h"
#include "uint256.h"
#include "wallet.h"

#include <stdint.h>
#include <map>
#include <string>

namespace mastercore
{
/**
 * Checks, whether the output qualifies as input for a transaction.
 */
bool CheckInput(const CTxOut& txOut, int nHeight, CTxDestination& dest)
{
    txnouttype whichType;

    if (!GetOutputType(txOut.scriptPubKey, whichType)) {
        return false;
    }
    if (!IsAllowedInputType(whichType, nHeight)) {
        return false;
    }
    if (!ExtractDestination(txOut.scriptPubKey, dest)) {
        return false;
    }

    return true;
}
/**
 * Selects spendable outputs to create a transaction.
 */
int64_t SelectCoins(const std::string& fromAddress, CCoinControl& coinControl, int64_t additional)
{
    // total output funds collected
    int64_t nTotal = 0;

#ifdef ENABLE_WALLET
    if (NULL == pwalletMain) {
        return 0;
    }

    // assume 20 KB max. transaction size at 0.0001 per kilobyte
    int64_t nMax = (COIN * (20 * (0.0001)));

    // if referenceamount is set it is needed to be accounted for here too
    if (0 < additional) nMax += additional;

    int nHeight = GetHeight();
    LOCK2(cs_main, pwalletMain->cs_wallet);

    // iterate over the wallet
    for (std::map<uint256, CWalletTx>::const_iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it) {
        const uint256& txid = it->first;
        const CWalletTx& wtx = it->second;

        if (!wtx.IsTrusted()) {
            continue;
        }
        if (!wtx.GetAvailableCredit()) {
            continue;
        }

        for (unsigned int n = 0; n < wtx.vout.size(); n++) {
            const CTxOut& txOut = wtx.vout[n];

            CTxDestination dest;
            if (!CheckInput(txOut, nHeight, dest)) {
                continue;
            }
            if (!IsMine(*pwalletMain, dest)) {
                continue;
            }
            if (pwalletMain->IsSpent(txid, n)) {
                continue;
            }

            std::string sAddress = CBitcoinAddress(dest).ToString();
            if (msc_debug_tokens)
                PrintToLog("%s(): sender: %s, outpoint: %s:%d, value: %d\n", __func__, sAddress, txid.GetHex(), n, txOut.nValue);

            // only use funds from the sender's address
            if (fromAddress == sAddress) {
                COutPoint outpoint(txid, n);
                coinControl.Select(outpoint);

                nTotal += txOut.nValue;
                if (nMax <= nTotal) break;
            }
        }

        if (nMax <= nTotal) break;
    }
#endif

    return nTotal;
}


} // namespace mastercore
