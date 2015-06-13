#ifndef OMNICORE_TEST_UTILS_TX_H
#define OMNICORE_TEST_UTILS_TX_H

class CTxOut;

CTxOut PayToPubKeyHash_Exodus();
CTxOut PayToPubKeyHash_ExodusCrowdsale(int nHeight);
CTxOut PayToPubKeyHash_Unrelated();
CTxOut PayToScriptHash_Unrelated();
CTxOut PayToPubKey_Unrelated();
CTxOut PayToBareMultisig_1of3();
CTxOut PayToBareMultisig_3of5();
CTxOut OpReturn_Empty();
CTxOut OpReturn_UnrelatedShort();
CTxOut OpReturn_Unrelated();
CTxOut OpReturn_Tagged_A();
CTxOut OpReturn_Tagged_B();
CTxOut OpReturn_Tagged_C();
CTxOut OpReturn_SimpleSend();
CTxOut NonStandardOutput();


#endif // OMNICORE_TEST_UTILS_TX_H
