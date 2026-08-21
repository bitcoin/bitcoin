// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/util.h>

#include <kj/common.h>
#include <kj/debug.h>
#include <kj/test.h>
#include <string>
#include <vector>

#ifdef WIN32
#include <shellapi.h>
#include <string>
#include <vector>
#endif

namespace mp {
namespace test {
namespace {

KJ_TEST("CommandLineFromArgv quoting")
{
    // Arguments without spaces, tabs, or quotes pass through unquoted, even if
    // they contain backslashes.
    KJ_EXPECT(CommandLineFromArgv({}) == "");
    KJ_EXPECT(CommandLineFromArgv({"simple"}) == "simple");
    KJ_EXPECT(CommandLineFromArgv({"a", "b", "c"}) == "a b c");
    KJ_EXPECT(CommandLineFromArgv({R"(C:\a\b)"}) == R"(C:\a\b)");
    KJ_EXPECT(CommandLineFromArgv({R"(\\.\pipe\mp-1234-1)"}) == R"(\\.\pipe\mp-1234-1)");
    KJ_EXPECT(CommandLineFromArgv({R"(\)"}) == R"(\)");

    // Empty arguments must be quoted so they are not dropped.
    KJ_EXPECT(CommandLineFromArgv({""}) == R"("")");
    KJ_EXPECT(CommandLineFromArgv({"a", "", "b"}) == R"(a "" b)");

    // Arguments with spaces or tabs are quoted.
    KJ_EXPECT(CommandLineFromArgv({"has space"}) == R"("has space")");
    KJ_EXPECT(CommandLineFromArgv({"has\ttab"}) == "\"has\ttab\"");
    KJ_EXPECT(CommandLineFromArgv({R"(C:\Program Files\bitcoin\bitcoin-node.exe)", "-ipcfd", "4"}) ==
              R"("C:\Program Files\bitcoin\bitcoin-node.exe" -ipcfd 4)");

    // Embedded quotes are backslash-escaped.
    KJ_EXPECT(CommandLineFromArgv({R"(say "hi")"}) == R"("say \"hi\"")");
    KJ_EXPECT(CommandLineFromArgv({R"(")"}) == R"("\"")");

    // Backslashes preceding a quote are doubled; other backslashes are not.
    KJ_EXPECT(CommandLineFromArgv({R"(back\\"slash quote)"}) == R"("back\\\\\"slash quote")");

    // Backslashes at the end of a quoted argument precede the closing quote,
    // so they must be doubled too, or the closing quote would be read as an
    // escaped literal quote and the argument would swallow the rest of the
    // command line.
    KJ_EXPECT(CommandLineFromArgv({R"(trailing backslash\)"}) == R"("trailing backslash\\")");
    KJ_EXPECT(CommandLineFromArgv({R"(trailing backslashes\\)"}) == R"("trailing backslashes\\\\")");
    KJ_EXPECT(CommandLineFromArgv({R"(mix \" of \\" things\)"}) == R"("mix \\\" of \\\\\" things\\")");
}

#ifdef WIN32
KJ_TEST("CommandLineFromArgv round-trips through CommandLineToArgvW")
{
    //! Argument vectors covering the CommandLineToArgvW quoting rules: plain
    //! arguments, spaces and tabs, embedded quotes, backslashes in various
    //! positions, and realistic Windows paths.
    const std::vector<std::vector<std::string>> quoting_cases{
        {"simple"},
        {"a", "b", "c"},
        {""},
        {"a", "", "b"},
        {"has space"},
        {"has\ttab"},
        {R"(say "hi")"},
        {R"(")"},
        {R"(\)"},
        {R"(C:\a\b)"},
        {R"(C:\Program Files\bitcoin\bitcoin-node.exe)", "-ipcfd", "4"},
        {R"(\\.\pipe\mp-1234-1)"},
        {R"(trailing backslash\)"},
        {R"(trailing backslashes\\)"},
        {R"(back\\"slash quote)"},
        {R"(mix \" of \\" things\)"},
    };

    for (const auto& argv : quoting_cases) {
        // Prepend a plain program name: CommandLineToArgvW parses the first
        // token with simpler rules (no backslash escaping), so only the
        // remaining arguments exercise the quoting logic under test.
        std::vector<std::string> args{"prog"};
        args.insert(args.end(), argv.begin(), argv.end());

        const std::string cmd{CommandLineFromArgv(args)};
        // Test arguments are ASCII, so widening by casting is fine.
        const std::wstring wcmd{cmd.begin(), cmd.end()};

        int argc{0};
        LPWSTR* wargv{CommandLineToArgvW(wcmd.c_str(), &argc)};
        KJ_ASSERT(wargv != nullptr, cmd);
        KJ_EXPECT(argc == static_cast<int>(args.size()), cmd, argc);
        for (int i = 0; i < argc && i < static_cast<int>(args.size()); ++i) {
            const std::wstring warg{wargv[i]};
            const std::string arg{warg.begin(), warg.end()};
            KJ_EXPECT(arg == args[i], cmd, i, arg);
        }
        LocalFree(wargv);
    }
}
#endif

} // namespace
} // namespace test
} // namespace mp
