// Copyright (c) 2023 Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Warn about any use of LogPrintf that does not end with a newline.
#include <string>

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
#define LogPrintf(...) LogPrintLevel_(LogFlags::NONE, Level::None, __VA_ARGS__)

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.
#define LogPrint(category, ...) \
    do {                        \
        LogPrintf(__VA_ARGS__); \
    } while (0)


class CWallet
{
    std::string GetDisplayName() const
    {
        return "default wallet";
    }

public:
    template <typename... Params>
    void WalletLogPrintf(std::string fmt, Params... parameters) const
    {
        LogPrintf(("%s " + fmt).c_str(), GetDisplayName(), parameters...);
    };
};

void good_func()
{
    LogPrintf("hello world!\n");
}
void good_func2()
{
    CWallet wallet;
    wallet.WalletLogPrintf("hi\n");

    const CWallet& walletref = wallet;
    walletref.WalletLogPrintf("hi\n");

    auto* walletptr = new CWallet();
    walletptr->WalletLogPrintf("hi\n");
    delete walletptr;
}
void bad_func()
{
    LogPrintf("hello world!");
}
void bad_func2()
{
    LogPrintf("");
}
void bad_func3()
{
    // Ending in "..." has no special meaning.
    LogPrintf("hello world!...");
}
void bad_func4_ignored()
{
    LogPrintf("hello world!"); // NOLINT(bitcoin-unterminated-logprintf)
}
void bad_func5()
{
    CWallet wallet;
    wallet.WalletLogPrintf("hi");

    const CWallet& walletref = wallet;
    walletref.WalletLogPrintf("hi");

    auto* walletptr = new CWallet();
    walletptr->WalletLogPrintf("hi");
    delete walletptr;
}
