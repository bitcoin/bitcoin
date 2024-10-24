using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Bitcoin
{
    public class ChainParams
    {
        public class SigNetOptions
        {
            public List<string> Seeds { get; set; } = new List<string>();
            public List<byte> Challenge { get; set; } = new List<byte>();
        }

        public class RegTestOptions
        {
            public bool FastPrune { get; set; }
            public Dictionary<string, int> ActivationHeights { get; set; } = new Dictionary<string, int>();
            public Dictionary<string, VersionBitsParameters> VersionBitsParameters { get; set; } = new Dictionary<string, VersionBitsParameters>();
        }

        public class VersionBitsParameters
        {
            public long StartTime { get; set; }
            public long Timeout { get; set; }
            public int MinActivationHeight { get; set; }
        }

        public static void ReadSigNetArgs(ArgsManager args, SigNetOptions options)
        {
            if (args.IsArgSet("-signetseednode"))
            {
                options.Seeds.AddRange(args.GetArgs("-signetseednode"));
            }
            if (args.IsArgSet("-signetchallenge"))
            {
                var signetChallenge = args.GetArgs("-signetchallenge");
                if (signetChallenge.Count != 1)
                {
                    throw new Exception("-signetchallenge cannot be multiple values.");
                }
                var val = TryParseHex(signetChallenge[0]);
                if (val == null)
                {
                    throw new Exception($"-signetchallenge must be hex, not '{signetChallenge[0]}'.");
                }
                options.Challenge.AddRange(val);
            }
        }

        public static void ReadRegTestArgs(ArgsManager args, RegTestOptions options)
        {
            if (args.GetBoolArg("-fastprune") != null) options.FastPrune = args.GetBoolArg("-fastprune").Value;

            foreach (var arg in args.GetArgs("-testactivationheight"))
            {
                var found = arg.IndexOf('@');
                if (found == -1)
                {
                    throw new Exception($"Invalid format ({arg}) for -testactivationheight=name@height.");
                }

                var value = arg.Substring(found + 1);
                if (!int.TryParse(value, out int height) || height < 0 || height >= int.MaxValue)
                {
                    throw new Exception($"Invalid height value ({value}) for -testactivationheight=name@height.");
                }

                var deploymentName = arg.Substring(0, found);
                if (GetBuriedDeployment(deploymentName) != null)
                {
                    options.ActivationHeights[deploymentName] = height;
                }
                else
                {
                    throw new Exception($"Invalid name ({deploymentName}) for -testactivationheight=name@height.");
                }
            }

            if (!args.IsArgSet("-vbparams")) return;

            foreach (var strDeployment in args.GetArgs("-vbparams"))
            {
                var vDeploymentParams = strDeployment.Split(':').ToList();
                if (vDeploymentParams.Count < 3 || vDeploymentParams.Count > 4)
                {
                    throw new Exception("Version bits parameters malformed, expecting deployment:start:end[:min_activation_height]");
                }
                var vbparams = new VersionBitsParameters();
                if (!long.TryParse(vDeploymentParams[1], out vbparams.StartTime))
                {
                    throw new Exception($"Invalid nStartTime ({vDeploymentParams[1]})");
                }
                if (!long.TryParse(vDeploymentParams[2], out vbparams.Timeout))
                {
                    throw new Exception($"Invalid nTimeout ({vDeploymentParams[2]})");
                }
                if (vDeploymentParams.Count >= 4)
                {
                    if (!int.TryParse(vDeploymentParams[3], out vbparams.MinActivationHeight))
                    {
                        throw new Exception($"Invalid min_activation_height ({vDeploymentParams[3]})");
                    }
                }
                else
                {
                    vbparams.MinActivationHeight = 0;
                }
                bool found = false;
                for (int j = 0; j < (int)Consensus.MAX_VERSION_BITS_DEPLOYMENTS; ++j)
                {
                    if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].Name)
                    {
                        options.VersionBitsParameters[VersionBitsDeploymentInfo[j].Name] = vbparams;
                        found = true;
                        LogPrintf($"Setting version bits activation parameters for {vDeploymentParams[0]} to start={vbparams.StartTime}, timeout={vbparams.Timeout}, min_activation_height={vbparams.MinActivationHeight}\n");
                        break;
                    }
                }
                if (!found)
                {
                    throw new Exception($"Invalid deployment ({vDeploymentParams[0]})");
                }
            }
        }

        private static ChainParams globalChainParams;

        public static ChainParams Params()
        {
            if (globalChainParams == null)
            {
                throw new Exception("globalChainParams is not set.");
            }
            return globalChainParams;
        }

        public static ChainParams CreateChainParams(ArgsManager args, ChainType chain)
        {
            switch (chain)
            {
                case ChainType.MAIN:
                    return Main();
                case ChainType.TESTNET:
                    return TestNet();
                case ChainType.TESTNET4:
                    return TestNet4();
                case ChainType.SIGNET:
                    var opts = new SigNetOptions();
                    ReadSigNetArgs(args, opts);
                    return SigNet(opts);
                case ChainType.REGTEST:
                    var regTestOpts = new RegTestOptions();
                    ReadRegTestArgs(args, regTestOpts);
                    return RegTest(regTestOpts);
                default:
                    throw new Exception("Invalid chain type.");
            }
        }

        public static void SelectParams(ChainType chain)
        {
            SelectBaseParams(chain);
            globalChainParams = CreateChainParams(gArgs, chain);
        }

        // Placeholder methods for Main, TestNet, TestNet4, SigNet, RegTest, GetBuriedDeployment, VersionBitsDeploymentInfo, LogPrintf, SelectBaseParams, gArgs, and Consensus
        public static ChainParams Main() => new ChainParams();
        public static ChainParams TestNet() => new ChainParams();
        public static ChainParams TestNet4() => new ChainParams();
        public static ChainParams SigNet(SigNetOptions options) => new ChainParams();
        public static ChainParams RegTest(RegTestOptions options) => new ChainParams();
        public static string GetBuriedDeployment(string name) => null;
        public static List<VersionBitsDeploymentInfo> VersionBitsDeploymentInfo => new List<VersionBitsDeploymentInfo>();
        public static void LogPrintf(string message) { }
        public static void SelectBaseParams(ChainType chain) { }
        public static ArgsManager gArgs => new ArgsManager();
        public static Consensus Consensus => new Consensus();

        // Placeholder classes for ArgsManager, ChainType, VersionBitsDeploymentInfo, and Consensus
        public class ArgsManager
        {
            public bool IsArgSet(string arg) => false;
            public List<string> GetArgs(string arg) => new List<string>();
            public bool? GetBoolArg(string arg) => null;
        }

        public enum ChainType
        {
            MAIN,
            TESTNET,
            TESTNET4,
            SIGNET,
            REGTEST
        }

        public class VersionBitsDeploymentInfo
        {
            public string Name { get; set; }
        }

        public class Consensus
        {
            public static int MAX_VERSION_BITS_DEPLOYMENTS => 0;
        }

        // Placeholder method for TryParseHex
        public static List<byte> TryParseHex(string hex) => new List<byte>();
    }
}
