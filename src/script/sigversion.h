#ifndef BITCOIN_SCRIPT_SIGVERSION_H
#define BITCOIN_SCRIPT_SIGVERSION_H
enum class SigVersion
{
    BASE = 0,        //!< Bare scripts and BIP16 P2SH-wrapped redeemscripts
    WITNESS_V0 = 1,  //!< Witness v0 (P2WPKH and P2WSH); see BIP 141
    TAPROOT = 2,     //!< Witness v1 with 32-byte program, not BIP16 P2SH-wrapped, key path spending; see BIP 341
    TAPSCRIPT = 3,   //!< Witness v1 with 32-byte program, not BIP16 P2SH-wrapped, script path spending, leaf version 0xc0; see BIP 342
    TAPSCRIPT_64BIT = 4, //!< Witness v1 with 32 byte program script path spending with 64bit arithmetic
};
#endif // BITCOIN_SCRIPT_SIGVERSION_H