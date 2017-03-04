#include <stdio.h>
#include "key.h"
#include "base58.h" // CBitcoinSecret

int main()
{
    ECC_Start();

    CKey key;
    key.MakeNewKey(false);

    // private key
    CPrivKey privkey = key.GetPrivKey();
    CPrivKey privkey2 = key.GetPrivKey();
    std::string strPrivKey = CBitcoinSecret(key).ToString();
    printf("PRIVKEY: %s\n", strPrivKey.c_str());

    // public key
    CPubKey pubkey = key.GetPubKey();
    

    // public key hash
    std::string hash = pubkey.GetPublicKeyHash().ToString();
    printf("PUBLIC HASH: %s\n", hash.c_str());

    ECC_Stop();
    return 0;
}
