using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Newtonsoft.Json.Linq;

namespace BitcoinTx
{
    class Program
    {
        private static bool fCreateBlank;
        private static Dictionary<string, JToken> registers = new Dictionary<string, JToken>();
        private const int CONTINUE_EXECUTION = -1;

        static void Main(string[] args)
        {
            SetupEnvironment();

            try
            {
                int ret = AppInitRawTx(args);
                if (ret != CONTINUE_EXECUTION)
                    Environment.Exit(ret);
            }
            catch (Exception e)
            {
                Console.Error.WriteLine("AppInitRawTx() error: " + e.Message);
                Environment.Exit(EXIT_FAILURE);
            }

            int ret = EXIT_FAILURE;
            try
            {
                ret = CommandLineRawTx(args);
            }
            catch (Exception e)
            {
                Console.Error.WriteLine("CommandLineRawTx() error: " + e.Message);
            }
            Environment.Exit(ret);
        }

        private static void SetupEnvironment()
        {
            // Setup environment
        }

        private static int AppInitRawTx(string[] args)
        {
            SetupBitcoinTxArgs();
            if (args.Length < 2 || args.Contains("-help") || args.Contains("-version"))
            {
                string strUsage = "Bitcoin TX utility version " + FormatFullVersion() + "\n";
                if (args.Contains("-version"))
                {
                    strUsage += LicenseInfo();
                }
                else
                {
                    strUsage += "\nUsage:  bitcoin-tx [options] <hex-tx> [commands]  Update hex-encoded bitcoin transaction\n"
                        + "or:     bitcoin-tx [options] -create [commands]   Create hex-encoded bitcoin transaction\n";
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

        private static void SetupBitcoinTxArgs()
        {
            // Setup Bitcoin TX arguments
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

        private static void RegisterSetJson(string key, string rawJson)
        {
            JToken val;
            try
            {
                val = JToken.Parse(rawJson);
            }
            catch (Exception)
            {
                throw new Exception("Cannot parse JSON for key " + key);
            }

            registers[key] = val;
        }

        private static void RegisterSet(string strInput)
        {
            // separate NAME:VALUE in string
            int pos = strInput.IndexOf(':');
            if (pos == -1 || pos == 0 || pos == strInput.Length - 1)
                throw new Exception("Register input requires NAME:VALUE");

            string key = strInput.Substring(0, pos);
            string valStr = strInput.Substring(pos + 1);

            RegisterSetJson(key, valStr);
        }

        private static void RegisterLoad(string strInput)
        {
            // separate NAME:FILENAME in string
            int pos = strInput.IndexOf(':');
            if (pos == -1 || pos == 0 || pos == strInput.Length - 1)
                throw new Exception("Register load requires NAME:FILENAME");

            string key = strInput.Substring(0, pos);
            string filename = strInput.Substring(pos + 1);

            string valStr;
            try
            {
                valStr = File.ReadAllText(filename);
            }
            catch (Exception)
            {
                throw new Exception("Cannot open file " + filename);
            }

            RegisterSetJson(key, valStr);
        }

        private static long ExtractAndValidateValue(string strValue)
        {
            if (long.TryParse(strValue, out long value))
            {
                return value;
            }
            else
            {
                throw new Exception("invalid TX output value");
            }
        }

        private static void MutateTxVersion(JObject tx, string cmdVal)
        {
            if (!uint.TryParse(cmdVal, out uint newVersion) || newVersion < 1 || newVersion > 2)
            {
                throw new Exception("Invalid TX version requested: '" + cmdVal + "'");
            }

            tx["version"] = newVersion;
        }

        private static void MutateTxLocktime(JObject tx, string cmdVal)
        {
            if (!long.TryParse(cmdVal, out long newLocktime) || newLocktime < 0 || newLocktime > 0xffffffff)
                throw new Exception("Invalid TX locktime requested: '" + cmdVal + "'");

            tx["locktime"] = (uint)newLocktime;
        }

        private static void MutateTxRBFOptIn(JObject tx, string strInIdx)
        {
            // parse requested index
            int inIdx = -1;
            if (strInIdx != "" && (!int.TryParse(strInIdx, out inIdx) || inIdx < 0 || inIdx >= tx["vin"].Count()))
            {
                throw new Exception("Invalid TX input index '" + strInIdx + "'");
            }

            // set the nSequence to MAX_INT - 2 (= RBF opt in flag)
            int cnt = 0;
            foreach (var txin in tx["vin"])
            {
                if (strInIdx == "" || cnt == inIdx)
                {
                    if ((uint)txin["sequence"] > 0xfffffffd)
                    {
                        txin["sequence"] = 0xfffffffd;
                    }
                }
                ++cnt;
            }
        }

        private static void MutateTxAddInput(JObject tx, string strInput)
        {
            var vStrInputParts = strInput.Split(':');

            // separate TXID:VOUT in string
            if (vStrInputParts.Length < 2)
                throw new Exception("TX input missing separator");

            // extract and validate TXID
            var txid = vStrInputParts[0];
            if (txid.Length != 64 || !IsHex(txid))
            {
                throw new Exception("invalid TX input txid");
            }

            // extract and validate vout
            var strVout = vStrInputParts[1];
            if (!int.TryParse(strVout, out int vout) || vout < 0 || vout > 0xffff)
                throw new Exception("invalid TX input vout '" + strVout + "'");

            // extract the optional sequence number
            uint nSequenceIn = 0xffffffff;
            if (vStrInputParts.Length > 2)
            {
                if (!uint.TryParse(vStrInputParts[2], out nSequenceIn))
                {
                    throw new Exception("invalid TX sequence id '" + vStrInputParts[2] + "'");
                }
            }

            // append to transaction input list
            var txin = new JObject
            {
                ["txid"] = txid,
                ["vout"] = vout,
                ["scriptSig"] = new JObject(),
                ["sequence"] = nSequenceIn
            };
            ((JArray)tx["vin"]).Add(txin);
        }

        private static void MutateTxAddOutAddr(JObject tx, string strInput)
        {
            // Separate into VALUE:ADDRESS
            var vStrInputParts = strInput.Split(':');

            if (vStrInputParts.Length != 2)
                throw new Exception("TX output missing or too many separators");

            // Extract and validate VALUE
            long value = ExtractAndValidateValue(vStrInputParts[0]);

            // extract and validate ADDRESS
            string strAddr = vStrInputParts[1];
            if (!IsValidAddress(strAddr))
            {
                throw new Exception("invalid TX output address");
            }
            var scriptPubKey = GetScriptForDestination(strAddr);

            // construct TxOut, append to transaction output list
            var txout = new JObject
            {
                ["value"] = value,
                ["scriptPubKey"] = scriptPubKey
            };
            ((JArray)tx["vout"]).Add(txout);
        }

        private static bool IsValidAddress(string address)
        {
            // Validate address
            return true;
        }

        private static JObject GetScriptForDestination(string address)
        {
            // Get script for destination
            return new JObject();
        }

        private static void MutateTxAddOutPubKey(JObject tx, string strInput)
        {
            // Separate into VALUE:PUBKEY[:FLAGS]
            var vStrInputParts = strInput.Split(':');

            if (vStrInputParts.Length < 2 || vStrInputParts.Length > 3)
                throw new Exception("TX output missing or too many separators");

            // Extract and validate VALUE
            long value = ExtractAndValidateValue(vStrInputParts[0]);

            // Extract and validate PUBKEY
            var pubkey = vStrInputParts[1];
            if (pubkey.Length != 66 || !IsHex(pubkey))
                throw new Exception("invalid TX output pubkey");
            var scriptPubKey = GetScriptForRawPubKey(pubkey);

            // Extract and validate FLAGS
            bool bSegWit = false;
            bool bScriptHash = false;
            if (vStrInputParts.Length == 3)
            {
                string flags = vStrInputParts[2];
                bSegWit = flags.Contains('W');
                bScriptHash = flags.Contains('S');
            }

            if (bSegWit)
            {
                if (pubkey.Length != 66)
                {
                    throw new Exception("Uncompressed pubkeys are not useable for SegWit outputs");
                }
                // Build a P2WPKH script
                scriptPubKey = GetScriptForDestination(pubkey);
            }
            if (bScriptHash)
            {
                // Get the ID for the script, and then construct a P2SH destination for it.
                scriptPubKey = GetScriptForDestination(scriptPubKey.ToString());
            }

            // construct TxOut, append to transaction output list
            var txout = new JObject
            {
                ["value"] = value,
                ["scriptPubKey"] = scriptPubKey
            };
            ((JArray)tx["vout"]).Add(txout);
        }

        private static JObject GetScriptForRawPubKey(string pubkey)
        {
            // Get script for raw pubkey
            return new JObject();
        }

        private static void MutateTxAddOutMultiSig(JObject tx, string strInput)
        {
            // Separate into VALUE:REQUIRED:NUMKEYS:PUBKEY1:PUBKEY2:....[:FLAGS]
            var vStrInputParts = strInput.Split(':');

            // Check that there are enough parameters
            if (vStrInputParts.Length < 3)
                throw new Exception("Not enough multisig parameters");

            // Extract and validate VALUE
            long value = ExtractAndValidateValue(vStrInputParts[0]);

            // Extract REQUIRED
            if (!uint.TryParse(vStrInputParts[1], out uint required))
                throw new Exception("invalid multisig required number");

            // Extract NUMKEYS
            if (!uint.TryParse(vStrInputParts[2], out uint numkeys))
                throw new Exception("invalid multisig total number");

            // Validate there are the correct number of pubkeys
            if (vStrInputParts.Length < numkeys + 3)
                throw new Exception("incorrect number of multisig pubkeys");

            if (required < 1 || required > 20 || numkeys < 1 || numkeys > 20 || numkeys < required)
                throw new Exception("multisig parameter mismatch. Required " + required + " of " + numkeys + " signatures.");

            // extract and validate PUBKEYs
            var pubkeys = new List<string>();
            for (int pos = 1; pos <= numkeys; pos++)
            {
                var pubkey = vStrInputParts[pos + 2];
                if (pubkey.Length != 66 || !IsHex(pubkey))
                    throw new Exception("invalid TX output pubkey");
                pubkeys.Add(pubkey);
            }

            // Extract FLAGS
            bool bSegWit = false;
            bool bScriptHash = false;
            if (vStrInputParts.Length == numkeys + 4)
            {
                string flags = vStrInputParts.Last();
                bSegWit = flags.Contains('W');
                bScriptHash = flags.Contains('S');
            }
            else if (vStrInputParts.Length > numkeys + 4)
            {
                // Validate that there were no more parameters passed
                throw new Exception("Too many parameters");
            }

            var scriptPubKey = GetScriptForMultisig(required, pubkeys);

            if (bSegWit)
            {
                foreach (var pubkey in pubkeys)
                {
                    if (pubkey.Length != 66)
                    {
                        throw new Exception("Uncompressed pubkeys are not useable for SegWit outputs");
                    }
                }
                // Build a P2WSH with the multisig script
                scriptPubKey = GetScriptForDestination(scriptPubKey.ToString());
            }
            if (bScriptHash)
            {
                if (scriptPubKey.ToString().Length > 520)
                {
                    throw new Exception("redeemScript exceeds size limit: " + scriptPubKey.ToString().Length + " > 520");
                }
                // Get the ID for the script, and then construct a P2SH destination for it.
                scriptPubKey = GetScriptForDestination(scriptPubKey.ToString());
            }

            // construct TxOut, append to transaction output list
            var txout = new JObject
            {
                ["value"] = value,
                ["scriptPubKey"] = scriptPubKey
            };
            ((JArray)tx["vout"]).Add(txout);
        }

        private static JObject GetScriptForMultisig(uint required, List<string> pubkeys)
        {
            // Get script for multisig
            return new JObject();
        }

        private static void MutateTxAddOutData(JObject tx, string strInput)
        {
            long value = 0;

            // separate [VALUE:]DATA in string
            int pos = strInput.IndexOf(':');

            if (pos == 0)
                throw new Exception("TX output value not specified");

            if (pos == -1)
            {
                pos = 0;
            }
            else
            {
                // Extract and validate VALUE
                value = ExtractAndValidateValue(strInput.Substring(0, pos));
                ++pos;
            }

            // extract and validate DATA
            var strData = strInput.Substring(pos);

            if (!IsHex(strData))
                throw new Exception("invalid TX output data");

            var data = ParseHex(strData);

            var txout = new JObject
            {
                ["value"] = value,
                ["scriptPubKey"] = new JObject
                {
                    ["asm"] = "OP_RETURN " + BitConverter.ToString(data).Replace("-", ""),
                    ["hex"] = BitConverter.ToString(data).Replace("-", ""),
                    ["type"] = "nulldata"
                }
            };
            ((JArray)tx["vout"]).Add(txout);
        }

        private static bool IsHex(string str)
        {
            // Check if string is hex
            return true;
        }

        private static byte[] ParseHex(string str)
        {
            // Parse hex string
            return new byte[0];
        }

        private static void MutateTxAddOutScript(JObject tx, string strInput)
        {
            // separate VALUE:SCRIPT[:FLAGS]
            var vStrInputParts = strInput.Split(':');
            if (vStrInputParts.Length < 2)
                throw new Exception("TX output missing separator");

            // Extract and validate VALUE
            long value = ExtractAndValidateValue(vStrInputParts[0]);

            // extract and validate script
            string strScript = vStrInputParts[1];
            var scriptPubKey = ParseScript(strScript);

            // Extract FLAGS
            bool bSegWit = false;
            bool bScriptHash = false;
            if (vStrInputParts.Length == 3)
            {
                string flags = vStrInputParts.Last();
                bSegWit = flags.Contains('W');
                bScriptHash = flags.Contains('S');
            }

            if (scriptPubKey.ToString().Length > 10000)
            {
                throw new Exception("script exceeds size limit: " + scriptPubKey.ToString().Length + " > 10000");
            }

            if (bSegWit)
            {
                scriptPubKey = GetScriptForDestination(scriptPubKey.ToString());
            }
            if (bScriptHash)
            {
                if (scriptPubKey.ToString().Length > 520)
                {
                    throw new Exception("redeemScript exceeds size limit: " + scriptPubKey.ToString().Length + " > 520");
                }
                scriptPubKey = GetScriptForDestination(scriptPubKey.ToString());
            }

            // construct TxOut, append to transaction output list
            var txout = new JObject
            {
                ["value"] = value,
                ["scriptPubKey"] = scriptPubKey
            };
            ((JArray)tx["vout"]).Add(txout);
        }

        private static JObject ParseScript(string strScript)
        {
            // Parse script
            return new JObject();
        }

        private static void MutateTxDelInput(JObject tx, string strInIdx)
        {
            // parse requested deletion index
            if (!int.TryParse(strInIdx, out int inIdx) || inIdx < 0 || inIdx >= tx["vin"].Count())
            {
                throw new Exception("Invalid TX input index '" + strInIdx + "'");
            }

            // delete input from transaction
            ((JArray)tx["vin"]).RemoveAt(inIdx);
        }

        private static void MutateTxDelOutput(JObject tx, string strOutIdx)
        {
            // parse requested deletion index
            if (!int.TryParse(strOutIdx, out int outIdx) || outIdx < 0 || outIdx >= tx["vout"].Count())
            {
                throw new Exception("Invalid TX output index '" + strOutIdx + "'");
            }

            // delete output from transaction
            ((JArray)tx["vout"]).RemoveAt(outIdx);
        }

        private static void MutateTxSign(JObject tx, string flagStr)
        {
            int nHashType = 1;

            if (flagStr.Length > 0)
                if (!FindSighashFlags(ref nHashType, flagStr))
                    throw new Exception("unknown sighash flag/sign option");

            // mergedTx will end up with all the signatures; it
            // starts as a clone of the raw tx:
            var mergedTx = new JObject(tx);
            var txv = new JObject(tx);
            var viewDummy = new JObject();
            var view = new JObject(viewDummy);

            if (!registers.ContainsKey("privatekeys"))
                throw new Exception("privatekeys register variable must be set.");
            var tempKeystore = new JObject();
            var keysObj = registers["privatekeys"];

            foreach (var key in keysObj)
            {
                if (key.Type != JTokenType.String)
                    throw new Exception("privatekey not a string");
                var privateKey = key.ToString();
                if (!IsValidPrivateKey(privateKey))
                {
                    throw new Exception("privatekey not valid");
                }
                AddKey(tempKeystore, privateKey);
            }

            // Add previous txouts given in the RPC call:
            if (!registers.ContainsKey("prevtxs"))
                throw new Exception("prevtxs register variable must be set.");
            var prevtxsObj = registers["prevtxs"];
            foreach (var prevOut in prevtxsObj)
            {
                if (prevOut.Type != JTokenType.Object)
                    throw new Exception("expected prevtxs internal object");

                var types = new Dictionary<string, JTokenType>
                {
                    { "txid", JTokenType.String },
                    { "vout", JTokenType.Integer },
                    { "scriptPubKey", JTokenType.String }
                };
                if (!CheckObject(prevOut, types))
                    throw new Exception("prevtxs internal object typecheck fail");

                var txid = prevOut["txid"].ToString();
                if (txid.Length != 64 || !IsHex(txid))
                {
                    throw new Exception("txid must be hexadecimal string (not '" + prevOut["txid"].ToString() + "')");
                }

                var nOut = (int)prevOut["vout"];
                if (nOut < 0)
                    throw new Exception("vout cannot be negative");

                var outPoint = new JObject
                {
                    ["txid"] = txid,
                    ["vout"] = nOut
                };
                var pkData = ParseHex(prevOut["scriptPubKey"].ToString());
                var scriptPubKey = new JObject
                {
                    ["asm"] = BitConverter.ToString(pkData).Replace("-", ""),
                    ["hex"] = BitConverter.ToString(pkData).Replace("-", "")
                };

                var coin = AccessCoin(view, outPoint);
                if (coin != null && coin["scriptPubKey"].ToString() != scriptPubKey.ToString())
                {
                    var err = "Previous output scriptPubKey mismatch:\n" + coin["scriptPubKey"].ToString() + "\nvs:\n" + scriptPubKey.ToString();
                    throw new Exception(err);
                }
                var newCoin = new JObject
                {
                    ["scriptPubKey"] = scriptPubKey,
                    ["value"] = 21000000
                };
                if (prevOut["amount"] != null)
                {
                    newCoin["value"] = AmountFromValue(prevOut["amount"]);
                }
                newCoin["height"] = 1;
                AddCoin(view, outPoint, newCoin);

                // if redeemScript given and private keys given,
                // add redeemScript to the tempKeystore so it can be signed:
                if ((IsPayToScriptHash(scriptPubKey) || IsPayToWitnessScriptHash(scriptPubKey)) && prevOut["redeemScript"] != null)
                {
                    var v = prevOut["redeemScript"];
                    var rsData = ParseHex(v.ToString());
                    var redeemScript = new JObject
                    {
                        ["asm"] = BitConverter.ToString(rsData).Replace("-", ""),
                        ["hex"] = BitConverter.ToString(rsData).Replace("-", "")
                    };
                    AddCScript(tempKeystore, redeemScript);
                }
            }

            var keystore = tempKeystore;

            bool fHashSingle = ((nHashType & ~0x80) == 3);

            // Sign what we can:
            for (int i = 0; i < mergedTx["vin"].Count(); i++)
            {
                var txin = mergedTx["vin"][i];
                var coin = AccessCoin(view, txin);
                if (coin == null)
                {
                    continue;
                }
                var prevPubKey = coin["scriptPubKey"];
                var amount = coin["value"];

                var sigdata = DataFromTransaction(mergedTx, i, coin);
                // Only sign SIGHASH_SINGLE if there's a corresponding output:
                if (!fHashSingle || (i < mergedTx["vout"].Count()))
                    ProduceSignature(keystore, MutableTransactionSignatureCreator(mergedTx, i, amount, nHashType), prevPubKey, sigdata);

                if (amount == 21000000 && sigdata["scriptWitness"] != null)
                {
                    throw new Exception("Missing amount for CTxOut with scriptPubKey=" + prevPubKey.ToString());
                }

                UpdateInput(txin, sigdata);
            }

            tx = mergedTx;
        }

        private static bool FindSighashFlags(ref int flags, string flagStr)
        {
            flags = 0;

            var sighashOptions = new Dictionary<string, int>
            {
                { "DEFAULT", 0 },
                { "ALL", 1 },
                { "NONE", 2 },
                { "SINGLE", 3 },
                { "ALL|ANYONECANPAY", 0x81 },
                { "NONE|ANYONECANPAY", 0x82 },
                { "SINGLE|ANYONECANPAY", 0x83 }
            };

            if (sighashOptions.ContainsKey(flagStr))
            {
                flags = sighashOptions[flagStr];
                return true;
            }

            return false;
        }

        private static bool IsValidPrivateKey(string privateKey)
        {
            // Validate private key
            return true;
        }

        private static void AddKey(JObject keystore, string privateKey)
        {
            // Add key to keystore
        }

        private static bool CheckObject(JToken obj, Dictionary<string, JTokenType> types)
        {
            // Check object types
            return true;
        }

        private static JObject AccessCoin(JObject view, JObject outPoint)
        {
            // Access coin
            return new JObject();
        }

        private static void AddCoin(JObject view, JObject outPoint, JObject coin)
        {
            // Add coin to view
        }

        private static bool IsPayToScriptHash(JObject scriptPubKey)
        {
            // Check if scriptPubKey is P2SH
            return true;
        }

        private static bool IsPayToWitnessScriptHash(JObject scriptPubKey)
        {
            // Check if scriptPubKey is P2WSH
            return true;
        }

        private static void AddCScript(JObject keystore, JObject redeemScript)
        {
            // Add redeem script to keystore
        }

        private static JObject DataFromTransaction(JObject tx, int index, JObject coin)
        {
            // Get data from transaction
            return new JObject();
        }

        private static void ProduceSignature(JObject keystore, JObject creator, JObject prevPubKey, JObject sigdata)
        {
            // Produce signature
        }

        private static JObject MutableTransactionSignatureCreator(JObject tx, int index, long amount, int nHashType)
        {
            // Create mutable transaction signature creator
            return new JObject();
        }

        private static void UpdateInput(JToken txin, JObject sigdata)
        {
            // Update input
        }

        private static void MutateTx(JObject tx, string command, string commandVal)
        {
            if (command == "nversion")
                MutateTxVersion(tx, commandVal);
            else if (command == "locktime")
                MutateTxLocktime(tx, commandVal);
            else if (command == "replaceable")
                MutateTxRBFOptIn(tx, commandVal);
            else if (command == "delin")
                MutateTxDelInput(tx, commandVal);
            else if (command == "in")
                MutateTxAddInput(tx, commandVal);
            else if (command == "delout")
                MutateTxDelOutput(tx, commandVal);
            else if (command == "outaddr")
                MutateTxAddOutAddr(tx, commandVal);
            else if (command == "outpubkey")
                MutateTxAddOutPubKey(tx, commandVal);
            else if (command == "outmultisig")
                MutateTxAddOutMultiSig(tx, commandVal);
            else if (command == "outscript")
                MutateTxAddOutScript(tx, commandVal);
            else if (command == "outdata")
                MutateTxAddOutData(tx, commandVal);
            else if (command == "sign")
                MutateTxSign(tx, commandVal);
            else if (command == "load")
                RegisterLoad(commandVal);
            else if (command == "set")
                RegisterSet(commandVal);
            else
                throw new Exception("unknown command");
        }

        private static void OutputTxJSON(JObject tx)
        {
            var entry = new JObject
            {
                ["txid"] = tx["txid"],
                ["hash"] = tx["hash"],
                ["version"] = tx["version"],
                ["size"] = tx["size"],
                ["vsize"] = tx["vsize"],
                ["weight"] = tx["weight"],
                ["locktime"] = tx["locktime"],
                ["vin"] = tx["vin"],
                ["vout"] = tx["vout"]
            };

            string jsonOutput = entry.ToString();
            Console.WriteLine(jsonOutput);
        }

        private static void OutputTxHash(JObject tx)
        {
            string strHexHash = tx["txid"].ToString();
            Console.WriteLine(strHexHash);
        }

        private static void OutputTxHex(JObject tx)
        {
            string strHex = tx.ToString();
            Console.WriteLine(strHex);
        }

        private static void OutputTx(JObject tx)
        {
            if (gArgs.GetBoolArg("-json", false))
                OutputTxJSON(tx);
            else if (gArgs.GetBoolArg("-txid", false))
                OutputTxHash(tx);
            else
                OutputTxHex(tx);
        }

        private static string ReadStdin()
        {
            using (var reader = new StreamReader(Console.OpenStandardInput(), Console.InputEncoding))
            {
                return reader.ReadToEnd().Trim();
            }
        }

        private static int CommandLineRawTx(string[] args)
        {
            string strPrint = "";
            int nRet = 0;
            try
            {
                // Skip switches; Permit common stdin convention "-"
                while (args.Length > 1 && args[1].StartsWith("-") && args[1].Length > 1)
                {
                    args = args.Skip(1).ToArray();
                }

                var tx = new JObject();
                int startArg;

                if (!fCreateBlank)
                {
                    // require at least one param
                    if (args.Length < 2)
                        throw new Exception("too few parameters");

                    // param: hex-encoded bitcoin transaction
                    string strHexTx = args[1];
                    if (strHexTx == "-") // "-" implies standard input
                        strHexTx = ReadStdin();

                    tx = JObject.Parse(strHexTx);
                    startArg = 2;
                }
                else
                    startArg = 1;

                for (int i = startArg; i < args.Length; i++)
                {
                    string arg = args[i];
                    string key, value;
                    int eqpos = arg.IndexOf('=');
                    if (eqpos == -1)
                        key = arg;
                    else
                    {
                        key = arg.Substring(0, eqpos);
                        value = arg.Substring(eqpos + 1);
                    }

                    MutateTx(tx, key, value);
                }

                OutputTx(tx);
            }
            catch (Exception e)
            {
                strPrint = "error: " + e.Message;
                nRet = EXIT_FAILURE;
            }

            if (strPrint != "")
            {
                Console.WriteLine(strPrint);
            }
            return nRet;
        }
    }
}
