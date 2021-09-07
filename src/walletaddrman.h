// Copyright (c) 2021- Counos Platform Team

#ifndef XBIT_WALLETADDRMAN_H
#define XBIT_WALLETADDRMAN_H

#include <clientversion.h>
#include <config/xbit-config.h>
#include <netaddress.h>
#include <protocol.h>
#include <random.h>
#include <sync.h>
#include <timedata.h>
#include <tinyformat.h>
#include <util/system.h>

#include <fs.h>
#include <hash.h>
#include <iostream>
#include <map>
#include <set>
#include <stdint.h>
#include <streams.h>
#include <vector>


class CWalletAddrMan
{

protected:
    //! critical section to protect the inner data structures
    mutable RecursiveMutex cs;

private:
    //! Serialization versions.
    enum Format : uint8_t {
        V0_STARTER = 0,    //

    };
    static constexpr Format FILE_FORMAT = Format::V0_STARTER;
;

protected:


public:
  std::string command;
  std::string walletAddressList;
  CWalletAddrMan();
  bool Find(const std::string& walletAddress, const std::string& check_command);
  bool FindTrustMiner(const std::string& walletAddress);
  bool FindBlockedWallet(const std::string& walletAddress);
   
    template <typename Stream>
    void Serialize(Stream& s_) const
    {
        LOCK(cs);

        // Always serialize in the latest version (FILE_FORMAT).

        OverrideStream<Stream> s(&s_, s_.GetType(), s_.GetVersion());

        s << static_cast<uint8_t>(FILE_FORMAT);

        s << command;
        s << walletAddressList.size();
        s << walletAddressList;

        uint256 asmap_version;

        s << asmap_version;
  }
  template <typename Stream>
  void Unserialize(Stream& s_)
  {
      LOCK(cs);

      Clear();

      Format format;
      s_ >> Using<CustomUintFormatter<1>>(format);

      int stream_version = s_.GetVersion();
      

      OverrideStream<Stream> s(&s_, s_.GetType(), stream_version);

      s >> command;
      s >> walletAddressList.size();
      s >> walletAddressList;

      uint256 asmap_version;

      s >> asmap_version;
  }
  void Clear()
  {
      LOCK(cs);
      command = "";
      walletAddressList="";
  }
  CWalletAddrMan(const std::string& Command)
  {
      Clear();
      command = Command;
  }
};
#endif // XBIT_WALLETADDRMAN_H
