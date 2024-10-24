using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace BitcoinUtil
{
    class Program
    {
        private const int CONTINUE_EXECUTION = -1;

        static void Main(string[] args)
        {
            SetupEnvironment();

            try
            {
                int ret = AppInitUtil(args);
                if (ret != CONTINUE_EXECUTION)
                {
                    Environment.Exit(ret);
                }
            }
            catch (Exception e)
            {
                Console.Error.WriteLine("AppInitUtil() error: " + e.Message);
                Environment.Exit(EXIT_FAILURE);
            }

            var cmd = GetCommand(args);
            if (cmd == null)
            {
                Console.Error.WriteLine("Error: must specify a command");
                Environment.Exit(EXIT_FAILURE);
            }

            int ret = EXIT_FAILURE;
            string strPrint = "";
            try
            {
                if (cmd == "grind")
                {
                    ret = Grind(args, ref strPrint);
                }
                else
                {
                    throw new Exception("Unknown command");
                }
            }
            catch (Exception e)
            {
                strPrint = "error: " + e.Message;
            }

            if (!string.IsNullOrEmpty(strPrint))
            {
                Console.WriteLine(strPrint);
            }

            Environment.Exit(ret);
        }

        private static void SetupEnvironment()
        {
            // Setup environment
        }

        private static int AppInitUtil(string[] args)
        {
            SetupBitcoinUtilArgs();
            if (args.Length < 2 || args.Contains("-help") || args.Contains("-version"))
            {
                string strUsage = "Bitcoin-util utility version " + FormatFullVersion() + "\n";
                if (args.Contains("-version"))
                {
                    strUsage += LicenseInfo();
                }
                else
                {
                    strUsage += "\nUsage:  bitcoin-util [options] [commands]  Do stuff\n";
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

        private static void SetupBitcoinUtilArgs()
        {
            // Setup Bitcoin Util arguments
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

        private static string GetCommand(string[] args)
        {
            if (args.Length < 1)
            {
                return null;
            }
            return args[0];
        }

        private static int Grind(string[] args, ref string strPrint)
        {
            if (args.Length != 2)
            {
                strPrint = "Must specify block header to grind";
                return EXIT_FAILURE;
            }

            CBlockHeader header;
            if (!DecodeHexBlockHeader(args[1], out header))
            {
                strPrint = "Could not decode block header";
                return EXIT_FAILURE;
            }

            uint nBits = header.nBits;
            bool found = false;
            uint proposed_nonce = 0;

            int n_tasks = Math.Max(1, Environment.ProcessorCount);
            var tasks = new List<Task>();
            for (int i = 0; i < n_tasks; ++i)
            {
                int offset = i;
                tasks.Add(Task.Run(() => GrindTask(nBits, header, offset, n_tasks, ref found, ref proposed_nonce)));
            }
            Task.WaitAll(tasks.ToArray());

            if (found)
            {
                header.nNonce = proposed_nonce;
            }
            else
            {
                strPrint = "Could not satisfy difficulty target";
                return EXIT_FAILURE;
            }

            strPrint = EncodeHexBlockHeader(header);
            return EXIT_SUCCESS;
        }

        private static void GrindTask(uint nBits, CBlockHeader header, int offset, int step, ref bool found, ref uint proposed_nonce)
        {
            var target = new ArithUint256();
            bool neg, over;
            target.SetCompact(nBits, out neg, out over);
            if (target == 0 || neg || over) return;
            header.nNonce = (uint)offset;

            uint finish = uint.MaxValue - (uint)step;
            finish = finish - (finish % (uint)step) + (uint)offset;

            while (!found && header.nNonce < finish)
            {
                uint next = (finish - header.nNonce < 5000 * (uint)step) ? finish : header.nNonce + 5000 * (uint)step;
                do
                {
                    if (UintToArith256(header.GetHash()) <= target)
                    {
                        if (!found)
                        {
                            found = true;
                            proposed_nonce = header.nNonce;
                        }
                        return;
                    }
                    header.nNonce += (uint)step;
                } while (header.nNonce != next);
            }
        }

        private static bool DecodeHexBlockHeader(string hex, out CBlockHeader header)
        {
            // Decode hex block header
            header = new CBlockHeader();
            return true;
        }

        private static string EncodeHexBlockHeader(CBlockHeader header)
        {
            // Encode hex block header
            return "";
        }

        private static ArithUint256 UintToArith256(uint hash)
        {
            // Convert uint to ArithUint256
            return new ArithUint256();
        }
    }

    class CBlockHeader
    {
        public uint nBits;
        public uint nNonce;

        public uint GetHash()
        {
            // Get hash
            return 0;
        }
    }

    class ArithUint256
    {
        public void SetCompact(uint nBits, out bool neg, out bool over)
        {
            // Set compact
            neg = false;
            over = false;
        }

        public static implicit operator int(ArithUint256 v)
        {
            return 0;
        }
    }
}
