#include <stdio.h>
#include "key.h"
#include "base58.h"
#include "BitcoinAddress.h"
#include <secp256k1.h>
#include "chainparams.h"

static const char* g_pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";


static bool MyDecodeBase58(const char* psz, std::vector<unsigned char>& vch)
{
    // Skip leading spaces.
    while (*psz && isspace(*psz))
        psz++;
    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;
    while (*psz == '1') {
        zeroes++;
        psz++;
    }
    // Allocate enough space in big-endian base256 representation.
    int size = strlen(psz) * 733 / 1000 + 1; // log(58) / log(256), rounded up.
    std::vector<unsigned char> b256(size);
    // Process the characters.
    while (*psz && !isspace(*psz)) {
        // Decode base58 character
        const char* ch = strchr(g_pszBase58, *psz);
        if (ch == NULL)
            return false;
        // Apply "b256 = b256 * 58 + ch".
        int carry = ch - g_pszBase58;
        int i = 0;
        for (
            std::vector<unsigned char>::reverse_iterator it = b256.rbegin();
            (carry != 0 || i < length) && (it != b256.rend());
            ++it, ++i
            ) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        length = i;
        psz++;
    }
    // Skip trailing spaces.
    while (isspace(*psz))
        psz++;
    if (*psz != 0)
        return false;
    // Skip leading zeroes in b256.
    std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
    while (it != b256.end() && *it == 0)
        it++;
    // Copy result into output vector.
    vch.reserve(zeroes + (b256.end() - it));
    vch.assign(zeroes, 0x00);
    while (it != b256.end())
        vch.push_back(*(it++));
    return true;
}

static std::string MyEncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
{
    // 先頭のゼロデータを飛ばす.
    // Skip & count leading zeroes.
    int zeroes = 0;
    while (pbegin != pend && *pbegin == 0) {
        pbegin++;
        zeroes++;
    }

    // Allocate enough space in big-endian base58 representation.
    int length = 0;
    int size = (pend - pbegin) * 138 / 100 + 1; // log(256) / log(58), rounded up.
    std::vector<unsigned char> b58(size); // 0～57 のいずれかを要素とする数値のリストを構築する.
    // Process the bytes.
    while (pbegin != pend) {
        int carry = *pbegin;
        int i = 0;
        // Apply "b58 = b58 * 256 + ch".
        for (
            std::vector<unsigned char>::reverse_iterator it = b58.rbegin();
            (carry != 0 || i < length) && (it != b58.rend());
            it++, i++
        ) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }

        assert(carry == 0);
        length = i;
        pbegin++;
    }
    // Skip leading zeroes in base58 result.
    std::vector<unsigned char>::iterator it = b58.begin() + (size - length);
    while (it != b58.end() && *it == 0)
        it++;
    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end())
        str += g_pszBase58[*(it++)];
    return str;
}

static void decodePrint(const std::string& s) {
    std::vector<unsigned char> decoded;
    MyDecodeBase58(s.c_str(), decoded);
    printf("{");
    for (int i = 0; i < decoded.size(); i++) {
        printf("%d, ", decoded[i]);
    }
    printf("}\n");
}

int main_impl();

int main()
{
    ECC_Start();

    try {
        main_impl();
    }
    catch (const std::exception& ex) {
        printf("Error: %s\n", ex.what());
    }

    ECC_Stop();
}

int main_impl()
{
    // BASE58関連実験
    if (0) {
        // std::vector<unsigned char> v(1, 239); // 要素数 1、中身は [239] なリストができる.
        // unsigned char* p = &v[0];

        unsigned char data[] = { 0,0,0,0,0x11,2,0xE }; // → "11116iLu" になる.
                                                       // unsigned char data[] = "abc";
                                                       // unsigned char data[] = { 0xFF };
        std::string s = MyEncodeBase58(data, data + sizeof(data));
        printf("%s\n", s.c_str());

        //decodePrint(s);
        //decodePrint("11116iLu");
        //decodePrint("1111 6iLu");
        decodePrint("11116iLu");

        // printf("0x%X\n", SECP256K1_EC_COMPRESSED);
        // printf("0x%X\n", SECP256K1_EC_UNCOMPRESSED);
    }


    // secret_key prefix の確認.
    if(1) {
        CKey key;

        // 9 で始まる 51 文字.
        SelectParams(NETWORK_REGTEST);
        key.MakeNewKey(false);
        printf("PRIVKEY in REGTEST: %s (%d)\n", key.GetBase58stringWithNetworkSecretKeyPrefix().c_str(), strlen(key.GetBase58stringWithNetworkSecretKeyPrefix().c_str()));

        // c で始まる 52 文字.
        SelectParams(NETWORK_REGTEST);
        key.MakeNewKey(true);
        printf("PRIVKEY in REGTEST: %s (%d)\n", key.GetBase58stringWithNetworkSecretKeyPrefix().c_str(), strlen(key.GetBase58stringWithNetworkSecretKeyPrefix().c_str()));

        // L で始まる 52 文字.
        SelectParams(NETWORK_MAIN);
        key.MakeNewKey(true);
        printf("PRIVKEY in MAINNET: %s (%d)\n", key.GetBase58stringWithNetworkSecretKeyPrefix().c_str(), strlen(key.GetBase58stringWithNetworkSecretKeyPrefix().c_str()));
    }
    

    // 鍵表示.
    if(0)for(int i = 0; i < 3; i++){
        // random 32bytes binary (※ただし楕円曲線として有効な範囲) (これをそのまま表示しようとすると64charsになりそうだが.)
        CKey key;
        key.MakeNewKey(false);

        // CPrivKey is a serialized private key, with all parameters included (279 bytes)
        // ※実際には 214bytes binary っぽいが？.
        CPrivKey privkey = key.GetPrivKey();
        printf("privkey.size = %d\n", privkey.size()); // 214

                                                       // public key (65bytes binary)
        CPubKey pubkey = key.GetPubKey();
        printf("pubkey.size = %d\n", pubkey.size()); // 65bytes これを文字列にしたら 130chars (HEX) になりそう

                                                     // public key hash (CHash256による生成) (uint256 (256bits) (32bytes) binary to string)
        uint256 hash = pubkey.GetPublicKeyHash(); // 256bits = 32bytes binary
        std::string hashStr = hash.ToHexString(); // 64長HEX文字列  例: "65d5f35db59351506de1da46cfc33005d5d6e2325667ed70c42f5bcb8a944b8d"
        printf("PUBLIC HASH: %s (%d)\n", hashStr.c_str(), hashStr.length());

        // ※ GetBase58stringWithNetworkSecretKeyPrefix は g_pCurrentParams に依存するため、
        //    先に g_pCurrentParams の指定が必要.
        SelectParams(NETWORK_MAIN);
        printf("PRIVKEY in MAINNET: %s (%d)\n", key.GetBase58stringWithNetworkSecretKeyPrefix().c_str(), strlen(key.GetBase58stringWithNetworkSecretKeyPrefix().c_str())); // ※MAINだと値が変わる.

        SelectParams(NETWORK_TESTNET);
        printf("PRIVKEY in TESTNET: %s (%d)\n", key.GetBase58stringWithNetworkSecretKeyPrefix().c_str(), strlen(key.GetBase58stringWithNetworkSecretKeyPrefix().c_str()));

        SelectParams(NETWORK_REGTEST);
        printf("PRIVKEY in REGTEST: %s (%d)\n", key.GetBase58stringWithNetworkSecretKeyPrefix().c_str(), strlen(key.GetBase58stringWithNetworkSecretKeyPrefix().c_str()));
    }



    
    return 0;
}
