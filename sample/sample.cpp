#include <stdio.h>
#include "key.h"

int main()
{
    ECC_Start();

    CKey key;
    key.MakeNewKey(false);
    CPubKey pubkey = key.GetPubKey();
    std::string hash = pubkey.GetHash().ToString();
    printf("%s\n", hash.c_str());

    ECC_Stop();
    return 0;
}
