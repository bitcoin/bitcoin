#include <walletaddrman.h>

#include <hash.h>
#include <logging.h>
#include <serialize.h>
#include <addrdb.h>

CWalletAddrMan::CWalletAddrMan()
{

}
bool CWalletAddrMan::Find(const std::string& walletAddress, const std::string& check_command)
{
    if (CWalletAddrMan::walletAddressList.find(walletAddress) != std::string::npos)
    return true;
  else return false;
}

bool CWalletAddrMan::FindBlockedWallet(const std::string& walletAddress)
{
    return CWalletAddrMan::Find(walletAddress, "isBlockedAddress");
}
bool CWalletAddrMan::FindTrustMiner(const std::string& walletAddress)
{
    return CWalletAddrMan::Find(walletAddress, "isTrustMiner");
}
