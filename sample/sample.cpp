#include <stdio.h>
#include "key.h"
#include "base58.h" // CBitcoinSecret

int main()
{
    ECC_Start();

    // random 32bytes binary (※ただし楕円曲線として有効な範囲)
    CKey key;
    key.MakeNewKey(false);

    // CPrivKey is a serialized private key, with all parameters included (279 bytes)
    // ※実際には 214bytes binary っぽいが？.
    CPrivKey privkey = key.GetPrivKey();

    // public key (65bytes binary)
    CPubKey pubkey = key.GetPubKey();

    // public key hash (CHash256による生成) (uint256 (256bits) (32bytes) binary to string)
    uint256 hash = pubkey.GetPublicKeyHash(); // 256bits = 32bytes binary
    std::string hashStr = hash.ToHexString(); // 64長HEX文字列  例: "65d5f35db59351506de1da46cfc33005d5d6e2325667ed70c42f5bcb8a944b8d"
    printf("PUBLIC HASH: %s\n", hashStr.c_str());

    // ※ CBitcoinSecret は g_pCurrentParams に依存するため、先に g_pCurrentParams の指定が必要.
    SelectParams(NETWORK_REGTEST);

    printf("PRIVKEY in REGTEST: %s\n", CBitcoinSecret(key).ToString().c_str());

    SelectParams(NETWORK_TESTNET);
    printf("PRIVKEY in TESTNET: %s\n", CBitcoinSecret(key).ToString().c_str());

    SelectParams(NETWORK_MAIN);
    printf("PRIVKEY in MAIN: %s\n", CBitcoinSecret(key).ToString().c_str()); // ※MAINだと値が変わる.



    ECC_Stop();
    return 0;
}
