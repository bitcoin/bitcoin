// Copyright (c) 2023 Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

// Test for bitcoin-unterminated-logprintf

enum LogFlags {
    NONE
};

enum Level {
    None
};

template <typename... Args>
static inline void LogPrintf_(const std::string& logging_function, const std::string& source_file, const int source_line, const LogFlags flag, const Level level, const char* fmt, const Args&... args)
{
}

#define LogPrintLevel_(category, level, ...) LogPrintf_(__func__, __FILE__, __LINE__, category, level, __VA_ARGS__)
#define LogInfo(...) LogPrintLevel_(LogFlags::NONE, Level::None, __VA_ARGS__)

#define LogDebug(category, ...) \
    do {                        \
        LogInfo(__VA_ARGS__); \
    } while (0)


class CWallet
{
    std::string GetDisplayName() const
    {
        return "default wallet";
    }

public:
    template <typename... Params>
    void WalletLogPrintf(const char* fmt, Params... parameters) const
    {
        LogInfo(("%s " + std::string{fmt}).c_str(), GetDisplayName(), parameters...);
    };
};

struct ScriptPubKeyMan
{
    std::string GetDisplayName() const
    {
        return "default wallet";
    }

    template <typename... Params>
    void WalletLogPrintf(const char* fmt, Params... parameters) const
    {
        LogInfo(("%s " + std::string{fmt}).c_str(), GetDisplayName(), parameters...);
    };
};

void good_func()
{
    LogInfo("hello world!\n");
}
void good_func2()
{
    CWallet wallet;
    wallet.WalletLogPrintf("hi\n");
    ScriptPubKeyMan spkm;
    spkm.WalletLogPrintf("hi\n");

    const CWallet& walletref = wallet;
    walletref.WalletLogPrintf("hi\n");

    auto* walletptr = new CWallet();
    walletptr->WalletLogPrintf("hi\n");
    delete walletptr;
}
void bad_func()
{
    LogInfo("hello world!");
}
void bad_func2()
{
    LogInfo("");
}
void bad_func3()
{
    // Ending in "..." has no special meaning.
    LogInfo("hello world!...");
}
void bad_func4_ignored()
{
    LogInfo("hello world!"); // NOLINT(bitcoin-unterminated-logprintf)
}
void bad_func5()
{
    CWallet wallet;
    wallet.WalletLogPrintf("hi");
    ScriptPubKeyMan spkm;
    spkm.WalletLogPrintf("hi");

    const CWallet& walletref = wallet;
    walletref.WalletLogPrintf("hi");

    auto* walletptr = new CWallet();
    walletptr->WalletLogPrintf("hi");
    delete walletptr;
}
