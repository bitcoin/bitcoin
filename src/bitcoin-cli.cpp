using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace BitcoinCli
{
    class Program
    {
        private const string DEFAULT_RPCCONNECT = "127.0.0.1";
        private const int DEFAULT_HTTP_CLIENT_TIMEOUT = 900;
        private const int DEFAULT_WAIT_CLIENT_TIMEOUT = 0;
        private const bool DEFAULT_NAMED = false;
        private const string DEFAULT_NBLOCKS = "1";
        private const string DEFAULT_COLOR_SETTING = "auto";

        static void Main(string[] args)
        {
            try
            {
                int ret = AppInitRPC(args);
                if (ret != CONTINUE_EXECUTION)
                    Environment.Exit(ret);
            }
            catch (Exception e)
            {
                Console.Error.WriteLine("AppInitRPC() error: " + e.Message);
                Environment.Exit(EXIT_FAILURE);
            }

            int ret = EXIT_FAILURE;
            try
            {
                ret = CommandLineRPC(args);
            }
            catch (Exception e)
            {
                Console.Error.WriteLine("CommandLineRPC() error: " + e.Message);
            }
            Environment.Exit(ret);
        }

        private static int AppInitRPC(string[] args)
        {
            SetupCliArgs();
            if (args.Length < 2 || args.Contains("-help") || args.Contains("-version"))
            {
                string strUsage = "Bitcoin RPC client version " + FormatFullVersion() + "\n";
                if (args.Contains("-version"))
                {
                    strUsage += LicenseInfo();
                }
                else
                {
                    strUsage += "\nUsage:  bitcoin-cli [options] <command> [params]  Send command to Bitcoin\n"
                        + "or:     bitcoin-cli [options] -named <command> [name=value]...  Send command to Bitcoin (with named arguments)\n"
                        + "or:     bitcoin-cli [options] help                List commands\n"
                        + "or:     bitcoin-cli [options] help <command>      Get help for a command\n";
                    strUsage += "\n" + GetHelpMessage();
                }
                Console.WriteLine(strUsage);
                if (args.Length < 2)
                {
                    Console.Error.WriteLine("Error: too few parameters");
                    return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
            }
            if (!CheckDataDirOption())
            {
                Console.Error.WriteLine("Error: Specified data directory does not exist.");
                return EXIT_FAILURE;
            }
            if (!ReadConfigFiles())
            {
                Console.Error.WriteLine("Error reading configuration file.");
                return EXIT_FAILURE;
            }
            try
            {
                SelectBaseParams(GetChainType());
            }
            catch (Exception e)
            {
                Console.Error.WriteLine("Error: " + e.Message);
                return EXIT_FAILURE;
            }
            return CONTINUE_EXECUTION;
        }

        private static void SetupCliArgs()
        {
            // Setup CLI arguments
        }

        private static string FormatFullVersion()
        {
            return "0.21.0";
        }

        private static string LicenseInfo()
        {
            return "Bitcoin Core is released under the terms of the MIT license.";
        }

        private static string GetHelpMessage()
        {
            return "Help message";
        }

        private static bool CheckDataDirOption()
        {
            return true;
        }

        private static bool ReadConfigFiles()
        {
            return true;
        }

        private static void SelectBaseParams(string chainType)
        {
            // Select base parameters
        }

        private static string GetChainType()
        {
            return "main";
        }

        private static int CommandLineRPC(string[] args)
        {
            string strPrint = "";
            int nRet = 0;
            try
            {
                // Skip switches
                while (args.Length > 1 && args[1].StartsWith("-"))
                {
                    args = args.Skip(1).ToArray();
                }
                string rpcPass = "";
                if (args.Contains("-stdinrpcpass"))
                {
                    Console.Write("RPC password> ");
                    rpcPass = Console.ReadLine();
                    args = args.Where(arg => arg != "-stdinrpcpass").ToArray();
                }
                List<string> arguments = args.ToList();
                if (args.Contains("-stdinwalletpassphrase"))
                {
                    if (arguments.Count < 1 || !arguments[0].StartsWith("walletpassphrase"))
                    {
                        throw new Exception("-stdinwalletpassphrase is only applicable for walletpassphrase(change)");
                    }
                    Console.Write("Wallet passphrase> ");
                    string walletPass = Console.ReadLine();
                    arguments.Insert(1, walletPass);
                }
                if (args.Contains("-stdin"))
                {
                    string line;
                    while ((line = Console.ReadLine()) != null)
                    {
                        arguments.Add(line);
                    }
                }
                CheckMultipleCLIArgs();
                BaseRequestHandler rh;
                string method = "";
                if (args.Contains("-getinfo"))
                {
                    rh = new GetinfoRequestHandler();
                }
                else if (args.Contains("-netinfo"))
                {
                    if (args.Length > 1 && args[1] == "help")
                    {
                        Console.WriteLine(new NetinfoRequestHandler().HelpDoc);
                        return 0;
                    }
                    rh = new NetinfoRequestHandler();
                }
                else if (args.Contains("-generate"))
                {
                    var getnewaddress = GetNewAddress();
                    var error = getnewaddress["error"];
                    if (error == null)
                    {
                        SetGenerateToAddressArgs(getnewaddress["result"].ToString(), arguments);
                        rh = new GenerateToAddressRequestHandler();
                    }
                    else
                    {
                        ParseError(error, ref strPrint, ref nRet);
                    }
                }
                else if (args.Contains("-addrinfo"))
                {
                    rh = new AddrinfoRequestHandler();
                }
                else
                {
                    rh = new DefaultRequestHandler();
                    if (arguments.Count < 1)
                    {
                        throw new Exception("too few parameters (need at least command)");
                    }
                    method = arguments[0];
                    arguments.RemoveAt(0);
                }
                if (nRet == 0)
                {
                    var walletName = args.Contains("-rpcwallet") ? args[Array.IndexOf(args, "-rpcwallet") + 1] : null;
                    var reply = ConnectAndCallRPC(rh, method, arguments, walletName);
                    var result = reply["result"];
                    var error = reply["error"];
                    if (error == null)
                    {
                        if (args.Contains("-getinfo"))
                        {
                            if (!args.Contains("-rpcwallet"))
                            {
                                GetWalletBalances(ref result);
                            }
                            ParseGetInfoResult(ref result);
                        }
                        ParseResult(result, ref strPrint);
                    }
                    else
                    {
                        ParseError(error, ref strPrint, ref nRet);
                    }
                }
            }
            catch (Exception e)
            {
                strPrint = "error: " + e.Message;
                nRet = EXIT_FAILURE;
            }
            if (!string.IsNullOrEmpty(strPrint))
            {
                Console.WriteLine(strPrint);
            }
            return nRet;
        }

        private static void CheckMultipleCLIArgs()
        {
            // Check multiple CLI arguments
        }

        private static JObject GetNewAddress()
        {
            return new JObject();
        }

        private static void SetGenerateToAddressArgs(string address, List<string> args)
        {
            if (args.Count > 2) throw new Exception("too many arguments (maximum 2 for nblocks and maxtries)");
            if (args.Count == 0)
            {
                args.Add(DEFAULT_NBLOCKS);
            }
            else if (args[0] == "0")
            {
                throw new Exception("the first argument (number of blocks to generate, default: " + DEFAULT_NBLOCKS + ") must be an integer value greater than zero");
            }
            args.Insert(1, address);
        }

        private static JObject ConnectAndCallRPC(BaseRequestHandler rh, string method, List<string> args, string rpcwallet = null)
        {
            return new JObject();
        }

        private static void ParseError(JToken error, ref string strPrint, ref int nRet)
        {
            if (error.Type == JTokenType.Object)
            {
                var errCode = error["code"];
                var errMsg = error["message"];
                if (errCode != null)
                {
                    strPrint = "error code: " + errCode.ToString() + "\n";
                }
                if (errMsg != null)
                {
                    strPrint += "error message:\n" + errMsg.ToString();
                }
                if (errCode != null && errCode.ToString() == "RPC_WALLET_NOT_SPECIFIED")
                {
                    strPrint += " Or for the CLI, specify the \"-rpcwallet=<walletname>\" option before the command";
                    strPrint += " (run \"bitcoin-cli -h\" for help or \"bitcoin-cli listwallets\" to see which wallets are currently loaded).";
                }
            }
            else
            {
                strPrint = "error: " + error.ToString();
            }
            nRet = Math.Abs(int.Parse(error["code"].ToString()));
        }

        private static void ParseResult(JToken result, ref string strPrint)
        {
            if (result == null) return;
            strPrint = result.Type == JTokenType.String ? result.ToString() : result.ToString(Newtonsoft.Json.Formatting.Indented);
        }

        private static void GetWalletBalances(ref JToken result)
        {
            // Get wallet balances
        }

        private static void ParseGetInfoResult(ref JToken result)
        {
            // Parse getinfo result
        }
    }

    abstract class BaseRequestHandler
    {
        public abstract JObject PrepareRequest(string method, List<string> args);
        public abstract JObject ProcessReply(JObject batchIn);
    }

    class GetinfoRequestHandler : BaseRequestHandler
    {
        public override JObject PrepareRequest(string method, List<string> args)
        {
            if (args.Count > 0)
            {
                throw new Exception("-getinfo takes no arguments");
            }
            var result = new JArray
            {
                new JObject { { "method", "getnetworkinfo" }, { "params", new JArray() }, { "id", 0 } },
                new JObject { { "method", "getblockchaininfo" }, { "params", new JArray() }, { "id", 1 } },
                new JObject { { "method", "getwalletinfo" }, { "params", new JArray() }, { "id", 2 } },
                new JObject { { "method", "getbalances" }, { "params", new JArray() }, { "id", 3 } }
            };
            return new JObject { { "batch", result } };
        }

        public override JObject ProcessReply(JObject batchIn)
        {
            var result = new JObject();
            var batch = batchIn["batch"].ToObject<List<JObject>>();
            if (batch[0]["error"] != null) return batch[0];
            if (batch[1]["error"] != null) return batch[1];
            result["version"] = batch[0]["result"]["version"];
            result["blocks"] = batch[1]["result"]["blocks"];
            result["headers"] = batch[1]["result"]["headers"];
            result["verificationprogress"] = batch[1]["result"]["verificationprogress"];
            result["timeoffset"] = batch[0]["result"]["timeoffset"];
            result["connections"] = new JObject
            {
                { "in", batch[0]["result"]["connections_in"] },
                { "out", batch[0]["result"]["connections_out"] },
                { "total", batch[0]["result"]["connections"] }
            };
            result["networks"] = batch[0]["result"]["networks"];
            result["difficulty"] = batch[1]["result"]["difficulty"];
            result["chain"] = batch[1]["result"]["chain"];
            if (batch[2]["result"] != null)
            {
                result["has_wallet"] = true;
                result["keypoolsize"] = batch[2]["result"]["keypoolsize"];
                result["walletname"] = batch[2]["result"]["walletname"];
                if (batch[2]["result"]["unlocked_until"] != null)
                {
                    result["unlocked_until"] = batch[2]["result"]["unlocked_until"];
                }
                result["paytxfee"] = batch[2]["result"]["paytxfee"];
            }
            if (batch[3]["result"] != null)
            {
                result["balance"] = batch[3]["result"]["mine"]["trusted"];
            }
            result["relayfee"] = batch[0]["result"]["relayfee"];
            result["warnings"] = batch[0]["result"]["warnings"];
            return new JObject { { "result", result }, { "error", null }, { "id", 1 } };
        }
    }

    class NetinfoRequestHandler : BaseRequestHandler
    {
        public string HelpDoc => "-netinfo level|\"help\" \n\n"
            + "Returns a network peer connections dashboard with information from the remote server.\n"
            + "This human-readable interface will change regularly and is not intended to be a stable API.\n"
            + "Under the hood, -netinfo fetches the data by calling getpeerinfo and getnetworkinfo.\n"
            + "An optional integer argument from 0 to 4 can be passed for different peers listings; 4 to 255 are parsed as 4.\n"
            + "Pass \"help\" to see this detailed help documentation.\n"
            + "If more than one argument is passed, only the first one is read and parsed.\n"
            + "Suggestion: use with the Linux watch(1) command for a live dashboard; see example below.\n\n"
            + "Arguments:\n"
            + "1. level (integer 0-4, optional)  Specify the info level of the peers dashboard (default 0):\n"
            + "                                  0 - Peer counts for each reachable network as well as for block relay peers\n"
            + "                                      and manual peers, and the list of local addresses and ports\n"
            + "                                  1 - Like 0 but preceded by a peers listing (without address and version columns)\n"
            + "                                  2 - Like 1 but with an address column\n"
            + "                                  3 - Like 1 but with a version column\n"
            + "                                  4 - Like 1 but with both address and version columns\n"
            + "2. help (string \"help\", optional) Print this help documentation instead of the dashboard.\n\n"
            + "Result:\n\n"
            + "* The peers listing in levels 1-4 displays all of the peers sorted by direction and minimum ping time:\n\n"
            + "  Column   Description\n"
            + "  ------   -----------\n"
            + "  <->      Direction\n"
            + "           \"in\"  - inbound connections are those initiated by the peer\n"
            + "           \"out\" - outbound connections are those initiated by us\n"
            + "  type     Type of peer connection\n"
            + "           \"full\"   - full relay, the default\n"
            + "           \"block\"  - block relay; like full relay but does not relay transactions or addresses\n"
            + "           \"manual\" - peer we manually added using RPC addnode or the -addnode/-connect config options\n"
            + "           \"feeler\" - short-lived connection for testing addresses\n"
            + "           \"addr\"   - address fetch; short-lived connection for requesting addresses\n"
            + "  net      Network the peer connected through (\"ipv4\", \"ipv6\", \"onion\", \"i2p\", \"cjdns\", or \"npr\" (not publicly routable))\n"
            + "  v        Version of transport protocol used for the connection\n"
            + "  mping    Minimum observed ping time, in milliseconds (ms)\n"
            + "  ping     Last observed ping time, in milliseconds (ms)\n"
            + "  send     Time since last message sent to the peer, in seconds\n"
            + "  recv     Time since last message received from the peer, in seconds\n"
            + "  txn      Time since last novel transaction received from the peer and accepted into our mempool, in minutes\n"
            + "           \"*\" - we do not relay transactions to this peer (relaytxes is false)\n"
            + "  blk      Time since last novel block passing initial validity checks received from the peer, in minutes\n"
            + "  hb       High-bandwidth BIP152 compact block relay\n"
            + "           \".\" (to)   - we selected the peer as a high-bandwidth peer\n"
            + "           \"*\" (from) - the peer selected us as a high-bandwidth peer\n"
            + "  addrp    Total number of addresses processed, excluding those dropped due to rate limiting\n"
            + "           \".\" - we do not relay addresses to this peer (addr_relay_enabled is false)\n"
            + "  addrl    Total number of addresses dropped due to rate limiting\n"
            + "  age      Duration of connection to the peer, in minutes\n"
            + "  asmap    Mapped AS (Autonomous System) number at the end of the BGP route to the peer, used for diversifying\n"
            + "           peer selection (only displayed if the -asmap config option is set)\n"
            + "  id       Peer index, in increasing order of peer connections since node startup\n"
            + "  address  IP address and port of the peer\n"
            + "  version  Peer version and subversion concatenated, e.g. \"70016/Satoshi:21.0.0/\"\n\n"
            + "* The peer counts table displays the number of peers for each reachable network as well as\n"
            + "  the number of block relay peers and manual peers.\n\n"
            + "* The local addresses table lists each local address broadcast by the node, the port, and the score.\n\n"
            + "Examples:\n\n"
            + "Peer counts table of reachable networks and list of local addresses\n"
            + "> bitcoin-cli -netinfo\n\n"
            + "The same, preceded by a peers listing without address and version columns\n"
            + "> bitcoin-cli -netinfo 1\n\n"
            + "Full dashboard\n"
            + "> bitcoin-cli -netinfo 4\n\n"
            + "Full live dashboard, adjust --interval or --no-title as needed (Linux)\n"
            + "> watch --interval 1 --no-title bitcoin-cli -netinfo 4\n\n"
            + "See this help\n"
            + "> bitcoin-cli -netinfo help\n";

        public override JObject PrepareRequest(string method, List<string> args)
        {
            if (args.Count > 0)
            {
                if (args[0] == "help")
                {
                    throw new Exception(HelpDoc);
                }
                if (!int.TryParse(args[0], out int level) || level < 0 || level > 4)
                {
                    throw new Exception("Invalid -netinfo argument: " + args[0] + "\nFor more information, run: bitcoin-cli -netinfo help");
                }
            }
            var result = new JArray
            {
                new JObject { { "method", "getpeerinfo" }, { "params", new JArray() }, { "id", 0 } },
                new JObject { { "method", "getnetworkinfo" }, { "params", new JArray() }, { "id", 1 } }
            };
            return new JObject { { "batch", result } };
        }

        public override JObject ProcessReply(JObject batchIn)
        {
            var result = new JObject();
            var batch = batchIn["batch"].ToObject<List<JObject>>();
            if (batch[0]["error"] != null) return batch[0];
            if (batch[1]["error"] != null) return batch[1];
            result["version"] = batch[0]["result"]["version"];
            result["blocks"] = batch[1]["result"]["blocks"];
            result["headers"] = batch[1]["result"]["headers"];
            result["verificationprogress"] = batch[1]["result"]["verificationprogress"];
            result["timeoffset"] = batch[0]["result"]["timeoffset"];
            result["connections"] = new JObject
            {
                { "in", batch[0]["result"]["connections_in"] },
                { "out", batch[0]["result"]["connections_out"] },
                { "total", batch[0]["result"]["connections"] }
            };
            result["networks"] = batch[0]["result"]["networks"];
            result["difficulty"] = batch[1]["result"]["difficulty"];
            result["chain"] = batch[1]["result"]["chain"];
            if (batch[2]["result"] != null)
            {
                result["has_wallet"] = true;
                result["keypoolsize"] = batch[2]["result"]["keypoolsize"];
                result["walletname"] = batch[2]["result"]["walletname"];
                if (batch[2]["result"]["unlocked_until"] != null)
                {
                    result["unlocked_until"] = batch[2]["result"]["unlocked_until"];
                }
                result["paytxfee"] = batch[2]["result"]["paytxfee"];
            }
            if (batch[3]["result"] != null)
            {
                result["balance"] = batch[3]["result"]["mine"]["trusted"];
            }
            result["relayfee"] = batch[0]["result"]["relayfee"];
            result["warnings"] = batch[0]["result"]["warnings"];
            return new JObject { { "result", result }, { "error", null }, { "id", 1 } };
        }
    }

    class GenerateToAddressRequestHandler : BaseRequestHandler
    {
        public override JObject PrepareRequest(string method, List<string> args)
        {
            var addressStr = args[1];
            var paramsArray = RPCConvertValues("generatetoaddress", args);
            return new JObject { { "method", "generatetoaddress" }, { "params", paramsArray }, { "id", 1 } };
        }

        public override JObject ProcessReply(JObject reply)
        {
            var result = new JObject();
            result["address"] = reply["result"]["address"];
            result["blocks"] = reply["result"]["blocks"];
            return new JObject { { "result", result }, { "error", null }, { "id", 1 } };
        }
    }

    class AddrinfoRequestHandler : BaseRequestHandler
    {
        public override JObject PrepareRequest(string method, List<string> args)
        {
            if (args.Count > 0)
            {
                throw new Exception("-addrinfo takes no arguments");
            }
            var paramsArray = RPCConvertValues("getnodeaddresses", new List<string> { "0" });
            return new JObject { { "method", "getnodeaddresses" }, { "params", paramsArray }, { "id", 1 } };
        }

        public override JObject ProcessReply(JObject reply)
        {
            if (reply["error"] != null) return reply;
            var nodes = reply["result"].ToObject<List<JObject>>();
            if (nodes.Count > 0 && nodes[0]["network"] == null)
            {
                throw new Exception("-addrinfo requires bitcoind server to be running v22.0 and up");
            }
            var counts = new int[NETWORKS.Length];
            foreach (var node in nodes)
            {
                var networkName = node["network"].ToString();
                var networkId = Array.IndexOf(NETWORKS, networkName);
                if (networkId == -1) continue;
                counts[networkId]++;
            }
            var result = new JObject();
            var addresses = new JObject();
            int total = 0;
            for (int i = 1; i < NETWORKS.Length - 1; i++)
            {
                addresses[NETWORKS[i]] = counts[i];
                total += counts[i];
            }
            addresses["total"] = total;
            result["addresses_known"] = addresses;
            return new JObject { { "result", result }, { "error", null }, { "id", 1 } };
        }
    }

    class DefaultRequestHandler : BaseRequestHandler
    {
        public override JObject PrepareRequest(string method, List<string> args)
        {
            var paramsArray = gArgs.GetBoolArg("-named", DEFAULT_NAMED) ? RPCConvertNamedValues(method, args) : RPCConvertValues(method, args);
            return new JObject { { "method", method }, { "params", paramsArray }, { "id", 1 } };
        }

        public override JObject ProcessReply(JObject reply)
        {
            return reply;
        }
    }

    static class gArgs
    {
        public static bool GetBoolArg(string arg, bool defaultValue)
        {
            return defaultValue;
        }

        public static string GetArg(string arg, string defaultValue)
        {
            return defaultValue;
        }

        public static bool IsArgSet(string arg)
        {
            return false;
        }

        public static void ForceSetArg(string arg, string value)
        {
        }
    }

    static class RPCConvertValues
    {
        public static JArray Convert(string method, List<string> args)
        {
            return new JArray();
        }
    }

    static class RPCConvertNamedValues
    {
        public static JArray Convert(string method, List<string> args)
        {
            return new JArray();
        }
    }
}
