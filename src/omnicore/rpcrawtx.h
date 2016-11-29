#ifndef OMNICORE_RPCRAWTX_H
#define OMNICORE_RPCRAWTX_H

#include <univalue.h>

UniValue omni_decodetransaction(const UniValue& params, bool fHelp);
UniValue omni_createrawtx_opreturn(const UniValue& params, bool fHelp);
UniValue omni_createrawtx_multisig(const UniValue& params, bool fHelp);
UniValue omni_createrawtx_input(const UniValue& params, bool fHelp);
UniValue omni_createrawtx_reference(const UniValue& params, bool fHelp);
UniValue omni_createrawtx_change(const UniValue& params, bool fHelp);


#endif // OMNICORE_RPCRAWTX_H