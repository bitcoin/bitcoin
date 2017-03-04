#include <stdio.h>
#include "key.h"
#include "base58.h" // CBitcoinSecret

int main()
{
    ECC_Start();

    CKey key;
    key.MakeNewKey(false);

    // ※ CBitcoinSecret は g_pCurrentParams に依存する (先に g_pCurrentParams の指定が必要)
    SelectParams(NETWORK_REGTEST);

    // private key
    CPrivKey privkey = key.GetPrivKey();
    printf("PRIVKEY in REGTEST: %s\n", CBitcoinSecret(key).ToString().c_str());

    SelectParams(NETWORK_TESTNET);
    printf("PRIVKEY in TESTNET: %s\n", CBitcoinSecret(key).ToString().c_str());

    SelectParams(NETWORK_MAIN);
    printf("PRIVKEY in MAIN: %s\n", CBitcoinSecret(key).ToString().c_str()); // ※MAINだと値が変わる


    // public key
    CPubKey pubkey = key.GetPubKey();
    

    // public key hash
    std::string hash = pubkey.GetPublicKeyHash().ToString();
    printf("PUBLIC HASH: %s\n", hash.c_str());

    ECC_Stop();
    return 0;
}
