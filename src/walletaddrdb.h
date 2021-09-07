#include <walletaddrman.h>
#include <addrdb.h>
#include <string>

class CWalletAddrDB
{
    private:
        CWallAddDb walletAdddb;

    public :
        CWalletAddrDB(uint8_t wallet_type) :  walletAdddb {wallet_type}
        {

        };
        bool Add(const std::string& walletAddress);
        
        bool Delete(const std::string& walletAddress);

        bool Find(const std::string& walletAddress);

        std::string GetWalletAddressinDb();
        
};