using System;
using System.Collections.Generic;

namespace Bitcoin
{
    public class ChainParamsBase
    {
        public string DataDir { get; }
        public ushort RPCPort { get; }
        public ushort OnionServiceTargetPort { get; }

        public ChainParamsBase(string dataDir, ushort rpcPort, ushort onionServiceTargetPort)
        {
            DataDir = dataDir;
            RPCPort = rpcPort;
            OnionServiceTargetPort = onionServiceTargetPort;
        }
    }

    public class ArgsManager
    {
        public void AddArg(string name, string help, int flags, string category)
        {
            // Add argument
        }

        public void SelectConfigNetwork(string network)
        {
            // Select config network
        }
    }

    public static class ChainParamsBaseOptions
    {
        public static void SetupChainParamsBaseOptions(ArgsManager argsman)
        {
            argsman.AddArg("-chain=<chain>", "Use the chain <chain> (default: main). Allowed values: main, test, testnet4, signet, regtest", 0, "CHAINPARAMS");
            argsman.AddArg("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. This is intended for regression testing tools and app development. Equivalent to -chain=regtest.", 0 | 1, "CHAINPARAMS");
            argsman.AddArg("-testactivationheight=name@height.", "Set the activation height of 'name' (segwit, bip34, dersig, cltv, csv). (regtest-only)", 0 | 1, "DEBUG_TEST");
            argsman.AddArg("-testnet", "Use the testnet3 chain. Equivalent to -chain=test. Support for testnet3 is deprecated and will be removed in an upcoming release. Consider moving to testnet4 now by using -testnet4.", 0, "CHAINPARAMS");
            argsman.AddArg("-testnet4", "Use the testnet4 chain. Equivalent to -chain=testnet4.", 0, "CHAINPARAMS");
            argsman.AddArg("-vbparams=deployment:start:end[:min_activation_height]", "Use given start/end times and min_activation_height for specified version bits deployment (regtest-only)", 0 | 1, "CHAINPARAMS");
            argsman.AddArg("-signet", "Use the signet chain. Equivalent to -chain=signet. Note that the network is defined by the -signetchallenge parameter", 0, "CHAINPARAMS");
            argsman.AddArg("-signetchallenge", "Blocks must satisfy the given script to be considered valid (only for signet networks; defaults to the global default signet test network challenge)", 0 | 2, "CHAINPARAMS");
            argsman.AddArg("-signetseednode", "Specify a seed node for the signet network, in the hostname[:port] format, e.g. sig.net:1234 (may be used multiple times to specify multiple seed nodes; defaults to the global default signet test network seed node(s))", 0 | 2, "CHAINPARAMS");
        }
    }

    public static class ChainParamsBaseFactory
    {
        private static ChainParamsBase globalChainBaseParams;

        public static ChainParamsBase BaseParams()
        {
            if (globalChainBaseParams == null)
            {
                throw new Exception("globalChainBaseParams is not set.");
            }
            return globalChainBaseParams;
        }

        public static ChainParamsBase CreateBaseChainParams(ChainType chain)
        {
            switch (chain)
            {
                case ChainType.MAIN:
                    return new ChainParamsBase("", 8332, 8334);
                case ChainType.TESTNET:
                    return new ChainParamsBase("testnet3", 18332, 18334);
                case ChainType.TESTNET4:
                    return new ChainParamsBase("testnet4", 48332, 48334);
                case ChainType.SIGNET:
                    return new ChainParamsBase("signet", 38332, 38334);
                case ChainType.REGTEST:
                    return new ChainParamsBase("regtest", 18443, 18445);
                default:
                    throw new Exception("Invalid chain type.");
            }
        }

        public static void SelectBaseParams(ChainType chain)
        {
            globalChainBaseParams = CreateBaseChainParams(chain);
            ArgsManager argsManager = new ArgsManager();
            argsManager.SelectConfigNetwork(chain.ToString());
        }
    }

    public enum ChainType
    {
        MAIN,
        TESTNET,
        TESTNET4,
        SIGNET,
        REGTEST
    }
}
