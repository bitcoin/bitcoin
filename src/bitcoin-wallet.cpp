using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BitcoinWallet
{
    class Program
    {
        static void Main(string[] args)
        {
            SetupEnvironment();
            RandomInit();

            try
            {
                int? maybeExit = WalletAppInit(args);
                if (maybeExit.HasValue)
                {
                    Environment.Exit(maybeExit.Value);
                }
            }
            catch (Exception e)
            {
                PrintExceptionContinue(e, "WalletAppInit()");
                Environment.Exit(EXIT_FAILURE);
            }

            string command = GetCommand(args);
            if (string.IsNullOrEmpty(command))
            {
                Console.Error.WriteLine("No method provided. Run `bitcoin-wallet -help` for valid methods.");
                Environment.Exit(EXIT_FAILURE);
            }

            if (args.Length != 0)
            {
                Console.Error.WriteLine($"Error: Additional arguments provided ({string.Join(", ", args)}). Methods do not take arguments. Please refer to `-help`.");
                Environment.Exit(EXIT_FAILURE);
            }

            ECC_Context ecc_context = new ECC_Context();
            if (!WalletTool.ExecuteWalletToolFunc(args, command))
            {
                Environment.Exit(EXIT_FAILURE);
            }
            Environment.Exit(EXIT_SUCCESS);
        }

        private static void SetupEnvironment()
        {
            // Setup environment
        }

        private static void RandomInit()
        {
            // Initialize random
        }

        private static int? WalletAppInit(string[] args)
        {
            ArgsManager argsman = new ArgsManager();
            SetupWalletToolArgs(argsman);

            string error_message;
            if (!argsman.ParseParameters(args, out error_message))
            {
                Console.Error.WriteLine($"Error parsing command line arguments: {error_message}");
                return EXIT_FAILURE;
            }

            bool missing_args = args.Length < 2;
            if (missing_args || HelpRequested(argsman) || argsman.IsArgSet("-version"))
            {
                string strUsage = $"{PACKAGE_NAME} bitcoin-wallet version {FormatFullVersion()}\n";

                if (argsman.IsArgSet("-version"))
                {
                    strUsage += FormatParagraph(LicenseInfo());
                }
                else
                {
                    strUsage += "\n"
                                + "bitcoin-wallet is an offline tool for creating and interacting with " + PACKAGE_NAME + " wallet files.\n"
                                + "By default bitcoin-wallet will act on wallets in the default mainnet wallet directory in the datadir.\n"
                                + "To change the target wallet, use the -datadir, -wallet and -regtest/-signet/-testnet/-testnet4 arguments.\n\n"
                                + "Usage:\n"
                                + "  bitcoin-wallet [options] <command>\n";
                    strUsage += "\n" + argsman.GetHelpMessage();
                }
                Console.WriteLine(strUsage);
                if (missing_args)
                {
                    Console.Error.WriteLine("Error: too few parameters");
                    return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
            }

            LogInstance().m_print_to_console = argsman.GetBoolArg("-printtoconsole", argsman.GetBoolArg("-debug", false));

            if (!CheckDataDirOption(argsman))
            {
                Console.Error.WriteLine($"Error: Specified data directory \"{argsman.GetArg("-datadir", "")}\" does not exist.");
                return EXIT_FAILURE;
            }

            SelectParams(argsman.GetChainType());

            return null;
        }

        private static void SetupWalletToolArgs(ArgsManager argsman)
        {
            SetupHelpOptions(argsman);
            SetupChainParamsBaseOptions(argsman);

            argsman.AddArg("-version", "Print version and exit", ArgsManager.ALLOW_ANY, OptionsCategory.OPTIONS);
            argsman.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager.ALLOW_ANY, OptionsCategory.OPTIONS);
            argsman.AddArg("-wallet=<wallet-name>", "Specify wallet name", ArgsManager.ALLOW_ANY | ArgsManager.NETWORK_ONLY, OptionsCategory.OPTIONS);
            argsman.AddArg("-dumpfile=<file name>", "When used with 'dump', writes out the records to this file. When used with 'createfromdump', loads the records into a new wallet.", ArgsManager.ALLOW_ANY | ArgsManager.DISALLOW_NEGATION, OptionsCategory.OPTIONS);
            argsman.AddArg("-debug=<category>", "Output debugging information (default: 0).", ArgsManager.ALLOW_ANY, OptionsCategory.DEBUG_TEST);
            argsman.AddArg("-descriptors", "Create descriptors wallet. Only for 'create'", ArgsManager.ALLOW_ANY, OptionsCategory.OPTIONS);
            argsman.AddArg("-legacy", "Create legacy wallet. Only for 'create'", ArgsManager.ALLOW_ANY, OptionsCategory.OPTIONS);
            argsman.AddArg("-format=<format>", "The format of the wallet file to create. Either \"bdb\" or \"sqlite\". Only used with 'createfromdump'", ArgsManager.ALLOW_ANY, OptionsCategory.OPTIONS);
            argsman.AddArg("-printtoconsole", "Send trace/debug info to console (default: 1 when no -debug is true, 0 otherwise).", ArgsManager.ALLOW_ANY, OptionsCategory.DEBUG_TEST);
            argsman.AddArg("-withinternalbdb", "Use the internal Berkeley DB parser when dumping a Berkeley DB wallet file (default: false)", ArgsManager.ALLOW_ANY, OptionsCategory.DEBUG_TEST);

            argsman.AddCommand("info", "Get wallet info");
            argsman.AddCommand("create", "Create new wallet file");
            argsman.AddCommand("salvage", "Attempt to recover private keys from a corrupt wallet. Warning: 'salvage' is experimental.");
            argsman.AddCommand("dump", "Print out all of the wallet key-value records");
            argsman.AddCommand("createfromdump", "Create new wallet file from dumped records");
        }

        private static bool HelpRequested(ArgsManager argsman)
        {
            // Check if help is requested
            return false;
        }

        private static string FormatFullVersion()
        {
            return "0.21.0";
        }

        private static string FormatParagraph(string text)
        {
            // Format paragraph
            return text;
        }

        private static string LicenseInfo()
        {
            return "Bitcoin Core is released under the terms of the MIT license.";
        }

        private static bool CheckDataDirOption(ArgsManager argsman)
        {
            // Check data directory option
            return true;
        }

        private static void SelectParams(string chainType)
        {
            // Select parameters
        }

        private static string GetCommand(string[] args)
        {
            if (args.Length < 1)
            {
                return null;
            }
            return args[0];
        }

        private static void PrintExceptionContinue(Exception e, string context)
        {
            // Print exception and continue
        }

        private static int EXIT_FAILURE = 1;
        private static int EXIT_SUCCESS = 0;
    }

    class ArgsManager
    {
        public const int ALLOW_ANY = 0;
        public const int NETWORK_ONLY = 1;
        public const int DISALLOW_NEGATION = 2;
        public const int DEBUG_TEST = 3;
        public const int OPTIONS = 4;

        public void AddArg(string name, string help, int flags, int category)
        {
            // Add argument
        }

        public void AddCommand(string name, string help)
        {
            // Add command
        }

        public bool ParseParameters(string[] args, out string error_message)
        {
            // Parse parameters
            error_message = null;
            return true;
        }

        public bool IsArgSet(string arg)
        {
            // Check if argument is set
            return false;
        }

        public string GetArg(string arg, string defaultValue)
        {
            // Get argument value
            return defaultValue;
        }

        public bool GetBoolArg(string arg, bool defaultValue)
        {
            // Get boolean argument value
            return defaultValue;
        }

        public string GetHelpMessage()
        {
            // Get help message
            return "Help message";
        }

        public string GetChainType()
        {
            // Get chain type
            return "main";
        }
    }

    class ECC_Context
    {
        // ECC context
    }

    class WalletTool
    {
        public static bool ExecuteWalletToolFunc(string[] args, string command)
        {
            // Execute wallet tool function
            return true;
        }
    }

    class LogInstance
    {
        public bool m_print_to_console;

        public static LogInstance Instance = new LogInstance();

        public static LogInstance Get()
        {
            return Instance;
        }
    }
}
