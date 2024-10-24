using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

namespace Bitcoind
{
    class Program
    {
        static void Main(string[] args)
        {
            NodeContext node = new NodeContext();
            int exitStatus;
            var init = Interfaces.MakeNodeInit(node, args, out exitStatus);
            if (init == null)
            {
                Environment.Exit(exitStatus);
            }

            SetupEnvironment();

            // Connect bitcoind signal handlers
            Noui.Connect();

            Util.ThreadSetInternalName("init");

            // Interpret command line arguments
            ArgsManager argsManager = node.Args;
            if (!ParseArgs(node, args))
            {
                Environment.Exit(EXIT_FAILURE);
            }
            // Process early info return commands such as -help or -version
            if (ProcessInitCommands(argsManager))
            {
                Environment.Exit(EXIT_SUCCESS);
            }

            // Start application
            if (!AppInit(node) || !node.ShutdownSignal.Wait())
            {
                node.ExitStatus = EXIT_FAILURE;
            }
            Interrupt(node);
            Shutdown(node);

            Environment.Exit(node.ExitStatus);
        }

        private static bool ParseArgs(NodeContext node, string[] args)
        {
            ArgsManager argsManager = node.Args;
            // If Qt is used, parameters/bitcoin.conf are parsed in qt/bitcoin.cpp's main()
            SetupServerArgs(argsManager, node.Init.CanListenIpc());
            string error;
            if (!argsManager.ParseParameters(args, out error))
            {
                return InitError($"Error parsing command line arguments: {error}");
            }

            var configError = Common.InitConfig(argsManager);
            if (configError != null)
            {
                return InitError(configError.Message, configError.Details);
            }

            // Error out when loose non-argument tokens are encountered on command line
            for (int i = 1; i < args.Length; i++)
            {
                if (!IsSwitchChar(args[i][0]))
                {
                    return InitError($"Command line contains unexpected token '{args[i]}', see bitcoind -h for a list of options.");
                }
            }
            return true;
        }

        private static bool ProcessInitCommands(ArgsManager args)
        {
            // Process help and version before taking care about datadir
            if (HelpRequested(args) || args.IsArgSet("-version"))
            {
                string strUsage = $"{PACKAGE_NAME} version {FormatFullVersion()}\n";

                if (args.IsArgSet("-version"))
                {
                    strUsage += FormatParagraph(LicenseInfo());
                }
                else
                {
                    strUsage += "\nUsage:  bitcoind [options]                     Start " + PACKAGE_NAME + "\n"
                        + "\n";
                    strUsage += args.GetHelpMessage();
                }

                Console.WriteLine(strUsage);
                return true;
            }

            return false;
        }

        private static bool AppInit(NodeContext node)
        {
            bool fRet = false;
            ArgsManager args = node.Args;

            TokenPipeEnd daemonEp = null;
            try
            {
                // -server defaults to true for bitcoind but not for the GUI so do this here
                args.SoftSetBoolArg("-server", true);
                // Set this early so that parameter interactions go to console
                InitLogging(args);
                InitParameterInteraction(args);
                if (!AppInitBasicSetup(args, out node.ExitStatus))
                {
                    // InitError will have been called with detailed error, which ends up on console
                    return false;
                }
                if (!AppInitParameterInteraction(args))
                {
                    // InitError will have been called with detailed error, which ends up on console
                    return false;
                }

                node.Warnings = new Node.Warnings();

                node.Kernel = new Kernel.Context();
                node.EccContext = new ECC_Context();
                if (!AppInitSanityChecks(node.Kernel))
                {
                    // InitError will have been called with detailed error, which ends up on console
                    return false;
                }

                if (args.GetBoolArg("-daemon", DEFAULT_DAEMON) || args.GetBoolArg("-daemonwait", DEFAULT_DAEMONWAIT))
                {
                    Console.WriteLine($"{PACKAGE_NAME} starting");

                    // Daemonize
                    switch (ForkDaemon(1, 0, out daemonEp)) // don't chdir (1), do close FDs (0)
                    {
                        case 0: // Child: continue.
                            // If -daemonwait is not enabled, immediately send a success token the parent.
                            if (!args.GetBoolArg("-daemonwait", DEFAULT_DAEMONWAIT))
                            {
                                daemonEp.TokenWrite(1);
                                daemonEp.Close();
                            }
                            break;
                        case -1: // Error happened.
                            return InitError($"fork_daemon() failed: {SysErrorString(errno)}");
                        default: // Parent: wait and exit.
                            int token = daemonEp.TokenRead();
                            if (token != 0) // Success
                            {
                                Environment.Exit(EXIT_SUCCESS);
                            }
                            else // fRet = false or token read error (premature exit).
                            {
                                Console.Error.WriteLine("Error during initialization - check debug.log for details");
                                Environment.Exit(EXIT_FAILURE);
                            }
                            break;
                    }
                }
                // Lock data directory after daemonization
                if (!AppInitLockDataDirectory())
                {
                    // If locking the data directory failed, exit immediately
                    return false;
                }
                fRet = AppInitInterfaces(node) && AppInitMain(node);
            }
            catch (Exception e)
            {
                PrintExceptionContinue(e, "AppInit()");
            }

            if (daemonEp != null && daemonEp.IsOpen())
            {
                // Signal initialization status to parent, then close pipe.
                daemonEp.TokenWrite(fRet ? 1 : 0);
                daemonEp.Close();
            }
            return fRet;
        }

        private static void SetupEnvironment()
        {
            // Setup environment
        }

        private static void NouiConnect()
        {
            // Connect bitcoind signal handlers
        }

        private static void UtilThreadSetInternalName(string name)
        {
            // Set internal thread name
        }

        private static bool InitError(string message)
        {
            Console.Error.WriteLine(message);
            return false;
        }

        private static bool InitError(string message, string details)
        {
            Console.Error.WriteLine($"{message}: {details}");
            return false;
        }

        private static void InitLogging(ArgsManager args)
        {
            // Initialize logging
        }

        private static void InitParameterInteraction(ArgsManager args)
        {
            // Initialize parameter interaction
        }

        private static bool AppInitBasicSetup(ArgsManager args, out int exitStatus)
        {
            // Basic setup for app initialization
            exitStatus = 0;
            return true;
        }

        private static bool AppInitParameterInteraction(ArgsManager args)
        {
            // Parameter interaction for app initialization
            return true;
        }

        private static bool AppInitSanityChecks(Kernel.Context kernel)
        {
            // Sanity checks for app initialization
            return true;
        }

        private static bool AppInitLockDataDirectory()
        {
            // Lock data directory for app initialization
            return true;
        }

        private static bool AppInitInterfaces(NodeContext node)
        {
            // Initialize interfaces for app initialization
            return true;
        }

        private static bool AppInitMain(NodeContext node)
        {
            // Main app initialization
            return true;
        }

        private static void PrintExceptionContinue(Exception e, string context)
        {
            // Print exception and continue
        }

        private static void Interrupt(NodeContext node)
        {
            // Interrupt node
        }

        private static void Shutdown(NodeContext node)
        {
            // Shutdown node
        }

        private static bool IsSwitchChar(char c)
        {
            // Check if character is a switch character
            return c == '-';
        }

        private static string FormatFullVersion()
        {
            // Format full version
            return "0.21.0";
        }

        private static string FormatParagraph(string text)
        {
            // Format paragraph
            return text;
        }

        private static string LicenseInfo()
        {
            // Get license info
            return "Bitcoin Core is released under the terms of the MIT license.";
        }

        private static bool HelpRequested(ArgsManager args)
        {
            // Check if help is requested
            return false;
        }

        private static int ForkDaemon(int nochdir, int noclose, out TokenPipeEnd endpoint)
        {
            // Fork daemon process
            endpoint = null;
            return 0;
        }

        private static string SysErrorString(int errno)
        {
            // Get system error string
            return "System error";
        }
    }

    class NodeContext
    {
        public ArgsManager Args { get; set; }
        public int ExitStatus { get; set; }
        public Node.Warnings Warnings { get; set; }
        public Kernel.Context Kernel { get; set; }
        public ECC_Context EccContext { get; set; }
        public ShutdownSignal ShutdownSignal { get; set; }
    }

    class ArgsManager
    {
        public void SoftSetBoolArg(string arg, bool value)
        {
            // Soft set boolean argument
        }

        public bool ParseParameters(string[] args, out string error)
        {
            // Parse parameters
            error = null;
            return true;
        }

        public bool IsArgSet(string arg)
        {
            // Check if argument is set
            return false;
        }

        public string GetHelpMessage()
        {
            // Get help message
            return "Help message";
        }

        public bool GetBoolArg(string arg, bool defaultValue)
        {
            // Get boolean argument value
            return defaultValue;
        }
    }

    class TokenPipeEnd
    {
        public bool IsOpen()
        {
            // Check if pipe end is open
            return true;
        }

        public void TokenWrite(int token)
        {
            // Write token to pipe
        }

        public int TokenRead()
        {
            // Read token from pipe
            return 0;
        }

        public void Close()
        {
            // Close pipe end
        }
    }

    class ECC_Context
    {
        // ECC context
    }

    class Kernel
    {
        public class Context
        {
            // Kernel context
        }
    }

    class Node
    {
        public class Warnings
        {
            // Node warnings
        }
    }

    class ShutdownSignal
    {
        public bool Wait()
        {
            // Wait for shutdown signal
            return true;
        }
    }

    class Interfaces
    {
        public static Init MakeNodeInit(NodeContext node, string[] args, out int exitStatus)
        {
            // Make node initialization
            exitStatus = 0;
            return new Init();
        }
    }

    class Init
    {
        public bool CanListenIpc()
        {
            // Check if can listen IPC
            return true;
        }
    }

    class Common
    {
        public static ConfigError InitConfig(ArgsManager args)
        {
            // Initialize config
            return null;
        }
    }

    class ConfigError
    {
        public string Message { get; set; }
        public string Details { get; set; }
    }

    class Noui
    {
        public static void Connect()
        {
            // Connect noui
        }
    }

    class Util
    {
        public static void ThreadSetInternalName(string name)
        {
            // Set internal thread name
        }
    }
}
