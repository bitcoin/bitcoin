using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace BitcoinCompressor
{
    public class Compressor
    {
        public static bool IsToKeyID(Script script, out KeyID hash)
        {
            hash = null;
            if (script.Size == 25 && script[0] == Opcode.OP_DUP && script[1] == Opcode.OP_HASH160
                && script[2] == 20 && script[23] == Opcode.OP_EQUALVERIFY
                && script[24] == Opcode.OP_CHECKSIG)
            {
                hash = new KeyID(script.Skip(3).Take(20).ToArray());
                return true;
            }
            return false;
        }

        public static bool IsToScriptID(Script script, out ScriptID hash)
        {
            hash = null;
            if (script.Size == 23 && script[0] == Opcode.OP_HASH160 && script[1] == 20
                && script[22] == Opcode.OP_EQUAL)
            {
                hash = new ScriptID(script.Skip(2).Take(20).ToArray());
                return true;
            }
            return false;
        }

        public static bool IsToPubKey(Script script, out PubKey pubkey)
        {
            pubkey = null;
            if (script.Size == 35 && script[0] == 33 && script[34] == Opcode.OP_CHECKSIG
                && (script[1] == 0x02 || script[1] == 0x03))
            {
                pubkey = new PubKey(script.Skip(1).Take(33).ToArray());
                return true;
            }
            if (script.Size == 67 && script[0] == 65 && script[66] == Opcode.OP_CHECKSIG
                && script[1] == 0x04)
            {
                pubkey = new PubKey(script.Skip(1).Take(65).ToArray());
                return pubkey.IsFullyValid();
            }
            return false;
        }

        public static bool CompressScript(Script script, out CompressedScript outScript)
        {
            outScript = null;
            if (IsToKeyID(script, out KeyID keyID))
            {
                outScript = new CompressedScript(21);
                outScript[0] = 0x00;
                Array.Copy(keyID.ToArray(), 0, outScript, 1, 20);
                return true;
            }
            if (IsToScriptID(script, out ScriptID scriptID))
            {
                outScript = new CompressedScript(21);
                outScript[0] = 0x01;
                Array.Copy(scriptID.ToArray(), 0, outScript, 1, 20);
                return true;
            }
            if (IsToPubKey(script, out PubKey pubkey))
            {
                outScript = new CompressedScript(33);
                Array.Copy(pubkey.ToArray(), 1, outScript, 1, 32);
                if (pubkey[0] == 0x02 || pubkey[0] == 0x03)
                {
                    outScript[0] = pubkey[0];
                    return true;
                }
                else if (pubkey[0] == 0x04)
                {
                    outScript[0] = (byte)(0x04 | (pubkey[64] & 0x01));
                    return true;
                }
            }
            return false;
        }

        public static uint GetSpecialScriptSize(uint nSize)
        {
            if (nSize == 0 || nSize == 1)
                return 20;
            if (nSize == 2 || nSize == 3 || nSize == 4 || nSize == 5)
                return 32;
            return 0;
        }

        public static bool DecompressScript(out Script script, uint nSize, CompressedScript inScript)
        {
            script = null;
            switch (nSize)
            {
                case 0x00:
                    script = new Script(25);
                    script[0] = Opcode.OP_DUP;
                    script[1] = Opcode.OP_HASH160;
                    script[2] = 20;
                    Array.Copy(inScript, 0, script, 3, 20);
                    script[23] = Opcode.OP_EQUALVERIFY;
                    script[24] = Opcode.OP_CHECKSIG;
                    return true;
                case 0x01:
                    script = new Script(23);
                    script[0] = Opcode.OP_HASH160;
                    script[1] = 20;
                    Array.Copy(inScript, 0, script, 2, 20);
                    script[22] = Opcode.OP_EQUAL;
                    return true;
                case 0x02:
                case 0x03:
                    script = new Script(35);
                    script[0] = 33;
                    script[1] = (byte)nSize;
                    Array.Copy(inScript, 0, script, 2, 32);
                    script[34] = Opcode.OP_CHECKSIG;
                    return true;
                case 0x04:
                case 0x05:
                    byte[] vch = new byte[33];
                    vch[0] = (byte)(nSize - 2);
                    Array.Copy(inScript, 0, vch, 1, 32);
                    PubKey pubkey = new PubKey(vch);
                    if (!pubkey.Decompress())
                        return false;
                    script = new Script(67);
                    script[0] = 65;
                    Array.Copy(pubkey.ToArray(), 0, script, 1, 65);
                    script[66] = Opcode.OP_CHECKSIG;
                    return true;
            }
            return false;
        }

        public static ulong CompressAmount(ulong n)
        {
            if (n == 0)
                return 0;
            int e = 0;
            while ((n % 10) == 0 && e < 9)
            {
                n /= 10;
                e++;
            }
            if (e < 9)
            {
                int d = (int)(n % 10);
                if (d < 1 || d > 9)
                    throw new ArgumentException("Invalid amount");
                n /= 10;
                return 1 + (ulong)((n * 9 + (ulong)d - 1) * 10 + e);
            }
            else
            {
                return 1 + (n - 1) * 10 + 9;
            }
        }

        public static ulong DecompressAmount(ulong x)
        {
            if (x == 0)
                return 0;
            x--;
            int e = (int)(x % 10);
            x /= 10;
            ulong n = 0;
            if (e < 9)
            {
                int d = (int)(x % 9) + 1;
                x /= 9;
                n = x * 10 + (ulong)d;
            }
            else
            {
                n = x + 1;
            }
            while (e > 0)
            {
                n *= 10;
                e--;
            }
            return n;
        }
    }

    public class Script : List<byte>
    {
        public Script() { }
        public Script(int capacity) : base(capacity) { }
        public Script(IEnumerable<byte> collection) : base(collection) { }
    }

    public class CompressedScript : List<byte>
    {
        public CompressedScript() { }
        public CompressedScript(int capacity) : base(capacity) { }
        public CompressedScript(IEnumerable<byte> collection) : base(collection) { }
    }

    public class KeyID
    {
        private byte[] data;

        public KeyID(byte[] data)
        {
            if (data.Length != 20)
                throw new ArgumentException("Invalid KeyID length");
            this.data = data;
        }

        public byte[] ToArray()
        {
            return data;
        }
    }

    public class ScriptID
    {
        private byte[] data;

        public ScriptID(byte[] data)
        {
            if (data.Length != 20)
                throw new ArgumentException("Invalid ScriptID length");
            this.data = data;
        }

        public byte[] ToArray()
        {
            return data;
        }
    }

    public class PubKey
    {
        private byte[] data;

        public PubKey(byte[] data)
        {
            if (data.Length != 33 && data.Length != 65)
                throw new ArgumentException("Invalid PubKey length");
            this.data = data;
        }

        public bool IsFullyValid()
        {
            // Implement validation logic
            return true;
        }

        public bool Decompress()
        {
            // Implement decompression logic
            return true;
        }

        public byte[] ToArray()
        {
            return data;
        }

        public byte this[int index]
        {
            get { return data[index]; }
        }
    }

    public enum Opcode : byte
    {
        OP_DUP = 0x76,
        OP_HASH160 = 0xa9,
        OP_EQUALVERIFY = 0x88,
        OP_CHECKSIG = 0xac,
        OP_EQUAL = 0x87
    }
}
