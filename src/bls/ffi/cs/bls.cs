þ½Ž¿using System;
using System.Text;
using System.Runtime.InteropServices;

namespace mcl
{
    class BLS
    {
        public const int BN254 = 0;
        public const int BLS12_381 = 5;

        const int IoEcComp = 512; // fixed byte representation
        public const int FR_UNIT_SIZE = 4;
        public const int FP_UNIT_SIZE = 6; // 4 if bls256.dll is used
        public const int COMPILED_TIME_VAR = FR_UNIT_SIZE * 10 + FP_UNIT_SIZE;

        public const int ID_UNIT_SIZE = FR_UNIT_SIZE;
        public const int SECRETKEY_UNIT_SIZE = FR_UNIT_SIZE;
        public const int PUBLICKEY_UNIT_SIZE = FP_UNIT_SIZE * 3 * 2;
        public const int SIGNATURE_UNIT_SIZE = FP_UNIT_SIZE * 3;

        public const int ID_SERIALIZE_SIZE = FR_UNIT_SIZE * 8;
        public const int SECRETKEY_SERIALIZE_SIZE = FR_UNIT_SIZE * 8;
        public const int PUBLICKEY_SERIALIZE_SIZE = FP_UNIT_SIZE * 8 * 2;
        public const int SIGNATURE_SERIALIZE_SIZE = FP_UNIT_SIZE * 8;

        public const string dllName = FP_UNIT_SIZE == 4 ? "bls256.dll" : "bls384_256.dll";
        [DllImport(dllName)]
        public static extern int blsInit(int curveType, int compiledTimeVar);

        [DllImport(dllName)] public static extern void blsIdSetInt(ref Id id, int x);
        [DllImport(dllName)] public static extern int blsIdSetDecStr(ref Id id, [In][MarshalAs(UnmanagedType.LPStr)] string buf, ulong bufSize);
        [DllImport(dllName)] public static extern int blsIdSetHexStr(ref Id id, [In][MarshalAs(UnmanagedType.LPStr)] string buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong blsIdGetDecStr([Out]StringBuilder buf, ulong maxBufSize, in Id id);
        [DllImport(dllName)] public static extern ulong blsIdGetHexStr([Out]StringBuilder buf, ulong maxBufSize, in Id id);

        [DllImport(dllName)] public static extern ulong blsIdSerialize([Out]byte[] buf, ulong maxBufSize, in Id id);
        [DllImport(dllName)] public static extern ulong blsSecretKeySerialize([Out]byte[] buf, ulong maxBufSize, in SecretKey sec);
        [DllImport(dllName)] public static extern ulong blsPublicKeySerialize([Out]byte[] buf, ulong maxBufSize, in PublicKey pub);
        [DllImport(dllName)] public static extern ulong blsSignatureSerialize([Out]byte[] buf, ulong maxBufSize, in Signature sig);
        [DllImport(dllName)] public static extern ulong blsIdDeserialize(ref Id id, [In]byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong blsSecretKeyDeserialize(ref SecretKey sec, [In]byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong blsPublicKeyDeserialize(ref PublicKey pub, [In]byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong blsSignatureDeserialize(ref Signature sig, [In]byte[] buf, ulong bufSize);

        [DllImport(dllName)] public static extern int blsIdIsEqual(in Id lhs, in Id rhs);
        [DllImport(dllName)] public static extern int blsSecretKeyIsEqual(in SecretKey lhs, in SecretKey rhs);
        [DllImport(dllName)] public static extern int blsPublicKeyIsEqual(in PublicKey lhs, in PublicKey rhs);
        [DllImport(dllName)] public static extern int blsSignatureIsEqual(in Signature lhs, in Signature rhs);
        // add
        [DllImport(dllName)] public static extern void blsSecretKeyAdd(ref SecretKey sec, in SecretKey rhs);
        [DllImport(dllName)] public static extern void blsPublicKeyAdd(ref PublicKey pub, in PublicKey rhs);
        [DllImport(dllName)] public static extern void blsSignatureAdd(ref Signature sig, in Signature rhs);
        // sub
        [DllImport(dllName)] public static extern void blsSecretKeySub(ref SecretKey sec, in SecretKey rhs);
        [DllImport(dllName)] public static extern void blsPublicKeySub(ref PublicKey pub, in PublicKey rhs);
        [DllImport(dllName)] public static extern void blsSignatureSub(ref Signature sig, in Signature rhs);

        // neg
        [DllImport(dllName)] public static extern void blsSecretKeyNeg(ref SecretKey x);
        [DllImport(dllName)] public static extern void blsPublicKeyNeg(ref PublicKey x);
        [DllImport(dllName)] public static extern void blsSignatureNeg(ref Signature x);
        // mul Fr
        [DllImport(dllName)] public static extern void blsSecretKeyMul(ref SecretKey sec, in SecretKey rhs);
        [DllImport(dllName)] public static extern void blsPublicKeyMul(ref PublicKey pub, in SecretKey rhs);
        [DllImport(dllName)] public static extern void blsSignatureMul(ref Signature sig, in SecretKey rhs);

        // mulVec
        [DllImport(dllName)] public static extern int blsPublicKeyMulVec(ref PublicKey pub, in PublicKey pubVec, in SecretKey idVec, ulong n);
        [DllImport(dllName)] public static extern int blsSignatureMulVec(ref Signature sig, in Signature sigVec, in SecretKey idVec, ulong n);
        // zero
        [DllImport(dllName)] public static extern int blsSecretKeyIsZero(in SecretKey x);
        [DllImport(dllName)] public static extern int blsPublicKeyIsZero(in PublicKey x);
        [DllImport(dllName)] public static extern int blsSignatureIsZero(in Signature x);
        //	hash buf and set
        [DllImport(dllName)] public static extern int blsHashToSecretKey(ref SecretKey sec, [In][MarshalAs(UnmanagedType.LPStr)] string buf, ulong bufSize);
        /*
			set secretKey if system has /dev/urandom or CryptGenRandom
			return 0 if success else -1
		*/
        [DllImport(dllName)] public static extern int blsSecretKeySetByCSPRNG(ref SecretKey sec);

        [DllImport(dllName)] public static extern void blsGetPublicKey(ref PublicKey pub, in SecretKey sec);
        [DllImport(dllName)] public static extern void blsGetPop(ref Signature sig, in SecretKey sec);

        // return 0 if success
        [DllImport(dllName)] public static extern int blsSecretKeyShare(ref SecretKey sec, in SecretKey msk, ulong k, in Id id);
        [DllImport(dllName)] public static extern int blsPublicKeyShare(ref PublicKey pub, in PublicKey mpk, ulong k, in Id id);


        [DllImport(dllName)] public static extern int blsSecretKeyRecover(ref SecretKey sec, in SecretKey secVec, in Id idVec, ulong n);
        [DllImport(dllName)] public static extern int blsPublicKeyRecover(ref PublicKey pub, in PublicKey pubVec, in Id idVec, ulong n);
        [DllImport(dllName)] public static extern int blsSignatureRecover(ref Signature sig, in Signature sigVec, in Id idVec, ulong n);

        [DllImport(dllName)] public static extern void blsSign(ref Signature sig, in SecretKey sec, [In][MarshalAs(UnmanagedType.LPStr)] string m, ulong size);

        // return 1 if valid
        [DllImport(dllName)] public static extern int blsVerify(in Signature sig, in PublicKey pub, [In][MarshalAs(UnmanagedType.LPStr)] string m, ulong size);
        [DllImport(dllName)] public static extern int blsVerifyPop(in Signature sig, in PublicKey pub);

        //////////////////////////////////////////////////////////////////////////
        // the following apis will be removed

        // mask buf with (1 << (bitLen(r) - 1)) - 1 if buf >= r
        [DllImport(dllName)] public static extern int blsIdSetLittleEndian(ref Id id, [In][MarshalAs(UnmanagedType.LPStr)] string buf, ulong bufSize);
        /*
			return written byte size if success else 0
		*/
        [DllImport(dllName)] public static extern ulong blsIdGetLittleEndian([Out]StringBuilder buf, ulong maxBufSize, in Id id);

        // return 0 if success
        // mask buf with (1 << (bitLen(r) - 1)) - 1 if buf >= r
        [DllImport(dllName)] public static extern int blsSecretKeySetLittleEndian(ref SecretKey sec, [In]byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern int blsSecretKeySetDecStr(ref SecretKey sec, [In][MarshalAs(UnmanagedType.LPStr)] string buf, ulong bufSize);
        [DllImport(dllName)] public static extern int blsSecretKeySetHexStr(ref SecretKey sec, [In][MarshalAs(UnmanagedType.LPStr)] string buf, ulong bufSize);
        /*
			return written byte size if success else 0
		*/
        [DllImport(dllName)] public static extern ulong blsSecretKeyGetLittleEndian([Out]byte[] buf, ulong maxBufSize, in SecretKey sec);
        /*
			return strlen(buf) if success else 0
			buf is '\0' terminated
		*/
        [DllImport(dllName)] public static extern ulong blsSecretKeyGetDecStr([Out]StringBuilder buf, ulong maxBufSize, in SecretKey sec);
        [DllImport(dllName)] public static extern ulong blsSecretKeyGetHexStr([Out]StringBuilder buf, ulong maxBufSize, in SecretKey sec);
        [DllImport(dllName)] public static extern int blsPublicKeySetHexStr(ref PublicKey pub, [In][MarshalAs(UnmanagedType.LPStr)] string buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong blsPublicKeyGetHexStr([Out]StringBuilder buf, ulong maxBufSize, in PublicKey pub);
        [DllImport(dllName)] public static extern int blsSignatureSetHexStr(ref Signature sig, [In][MarshalAs(UnmanagedType.LPStr)] string buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong blsSignatureGetHexStr([Out]StringBuilder buf, ulong maxBufSize, in Signature sig);

        public static void Init(int curveType = BN254) {
            if (!System.Environment.Is64BitProcess) {
                throw new PlatformNotSupportedException("not 64-bit system");
            }
            int err = blsInit(curveType, COMPILED_TIME_VAR);
            if (err != 0) {
                throw new ArgumentException("blsInit");
            }
        }
        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct Id
        {
            private fixed ulong v[ID_UNIT_SIZE];
            public byte[] Serialize() {
                byte[] buf = new byte[ID_SERIALIZE_SIZE];
                ulong n = blsIdSerialize(buf, (ulong)buf.Length, this);
                if (n == 0) {
                    throw new ArithmeticException("blsIdSerialize");
                }
                return buf;
            }
            public void Deserialize(byte[] buf) {
                ulong n = blsIdDeserialize(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("blsIdDeserialize");
                }
            }
            public bool IsEqual(in Id rhs) {
                return blsIdIsEqual(this, rhs) != 0;
            }
            public void SetDecStr(string s) {
                if (blsIdSetDecStr(ref this, s, (ulong)s.Length) != 0) {
                    throw new ArgumentException("blsIdSetDecSt:" + s);
                }
            }
            public void SetHexStr(string s) {
                if (blsIdSetHexStr(ref this, s, (ulong)s.Length) != 0) {
                    throw new ArgumentException("blsIdSetHexStr:" + s);
                }
            }
            public void SetInt(int x) {
                blsIdSetInt(ref this, x);
            }
            public string GetDecStr() {
                StringBuilder sb = new StringBuilder(1024);
                ulong size = blsIdGetDecStr(sb, (ulong)sb.Capacity, this);
                if (size == 0) {
                    throw new ArgumentException("blsIdGetDecStr");
                }
                return sb.ToString(0, (int)size);
            }
            public string GetHexStr() {
                StringBuilder sb = new StringBuilder(1024);
                ulong size = blsIdGetHexStr(sb, (ulong)sb.Capacity, this);
                if (size == 0) {
                    throw new ArgumentException("blsIdGetHexStr");
                }
                return sb.ToString(0, (int)size);
            }
        }
        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct SecretKey
        {
            private fixed ulong v[SECRETKEY_UNIT_SIZE];
            public byte[] Serialize() {
                byte[] buf = new byte[SECRETKEY_SERIALIZE_SIZE];
                ulong n = blsSecretKeySerialize(buf, (ulong)buf.Length, this);
                if (n == 0) {
                    throw new ArithmeticException("blsSecretKeySerialize");
                }
                return buf;
            }
            public void Deserialize(byte[] buf) {
                ulong n = blsSecretKeyDeserialize(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("blsSecretKeyDeserialize");
                }
            }
            public bool IsEqual(in SecretKey rhs) {
                return blsSecretKeyIsEqual(this, rhs) != 0;
            }
            public bool IsZero()
            {
                return blsSecretKeyIsZero(this) != 0;
            }
            public void SetHexStr(string s) {
                if (blsSecretKeySetHexStr(ref this, s, (ulong)s.Length) != 0) {
                    throw new ArgumentException("blsSecretKeySetHexStr:" + s);
                }
            }
            public string GetHexStr() {
                StringBuilder sb = new StringBuilder(1024);
                ulong size = blsSecretKeyGetHexStr(sb, (ulong)sb.Capacity, this);
                if (size == 0) {
                    throw new ArgumentException("blsSecretKeyGetHexStr");
                }
                return sb.ToString(0, (int)size);
            }
            public void Add(in SecretKey rhs) {
                blsSecretKeyAdd(ref this, rhs);
            }
            public void Sub(in SecretKey rhs)
            {
                blsSecretKeySub(ref this, rhs);
            }
            public void Neg()
            {
                blsSecretKeyNeg(ref this);
            }
            public void Mul(in SecretKey rhs)
            {
                blsSecretKeyMul(ref this, rhs);
            }
            public void SetByCSPRNG() {
                blsSecretKeySetByCSPRNG(ref this);
            }
            public void SetHashOf(string s) {
                if (blsHashToSecretKey(ref this, s, (ulong)s.Length) != 0) {
                    throw new ArgumentException("blsHashToSecretKey");
                }
            }
            public PublicKey GetPublicKey() {
                PublicKey pub;
                blsGetPublicKey(ref pub, this);
                return pub;
            }
            public Signature Sign(string m) {
                Signature sig;
                blsSign(ref sig, this, m, (ulong)m.Length);
                return sig;
            }
            public Signature GetPop() {
                Signature sig;
                blsGetPop(ref sig, this);
                return sig;
            }
        }
        // secretKey = sum_{i=0}^{msk.Length - 1} msk[i] * id^i
        public static SecretKey ShareSecretKey(in SecretKey[] msk, in Id id) {
            SecretKey sec;
            if (blsSecretKeyShare(ref sec, msk[0], (ulong)msk.Length, id) != 0) {
                throw new ArgumentException("GetSecretKeyForId:" + id.ToString());
            }
            return sec;
        }
        public static SecretKey RecoverSecretKey(in SecretKey[] secVec, in Id[] idVec) {
            SecretKey sec;
            if (blsSecretKeyRecover(ref sec, secVec[0], idVec[0], (ulong)secVec.Length) != 0) {
                throw new ArgumentException("Recover");
            }
            return sec;
        }
        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct PublicKey
        {
            private fixed ulong v[PUBLICKEY_UNIT_SIZE];
            public byte[] Serialize() {
                byte[] buf = new byte[PUBLICKEY_SERIALIZE_SIZE];
                ulong n = blsPublicKeySerialize(buf, (ulong)buf.Length, this);
                if (n == 0) {
                    throw new ArithmeticException("blsPublicKeySerialize");
                }
                return buf;
            }
            public void Deserialize(byte[] buf) {
                ulong n = blsPublicKeyDeserialize(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("blsPublicKeyDeserialize");
                }
            }
            public bool IsEqual(in PublicKey rhs) {
                return blsPublicKeyIsEqual(this, rhs) != 0;
            }
            public bool IsZero()
            {
                return blsPublicKeyIsZero(this) != 0;
            }
            public void SetStr(string s) {
                if (blsPublicKeySetHexStr(ref this, s, (ulong)s.Length) != 0) {
                    throw new ArgumentException("blsPublicKeySetStr:" + s);
                }
            }
            public string GetHexStr() {
                StringBuilder sb = new StringBuilder(1024);
                ulong size = blsPublicKeyGetHexStr(sb, (ulong)sb.Capacity, this);
                if (size == 0) {
                    throw new ArgumentException("blsPublicKeyGetStr");
                }
                return sb.ToString(0, (int)size);
            }
            public void Add(in PublicKey rhs) {
                blsPublicKeyAdd(ref this, rhs);
            }
            public void Sub(in PublicKey rhs)
            {
                blsPublicKeySub(ref this, rhs);
            }
            public void Neg()
            {
                blsPublicKeyNeg(ref this);
            }
            public void Mul(in SecretKey rhs)
            {
                blsPublicKeyMul(ref this, rhs);
            }
            public bool Verify(in Signature sig, string m) {
                return blsVerify(sig, this, m, (ulong)m.Length) == 1;
            }
            public bool VerifyPop(in Signature pop) {
                return blsVerifyPop(pop, this) == 1;
            }
        }
        // publicKey = sum_{i=0}^{mpk.Length - 1} mpk[i] * id^i
        public static PublicKey SharePublicKey(in PublicKey[] mpk, in Id id) {
            PublicKey pub;
            if (blsPublicKeyShare(ref pub, mpk[0], (ulong)mpk.Length, id) != 0) {
                throw new ArgumentException("GetPublicKeyForId:" + id.ToString());
            }
            return pub;
        }
        public static PublicKey RecoverPublicKey(in PublicKey[] pubVec, in Id[] idVec) {
            PublicKey pub;
            if (blsPublicKeyRecover(ref pub, pubVec[0], idVec[0], (ulong)pubVec.Length) != 0) {
                throw new ArgumentException("Recover");
            }
            return pub;
        }
        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct Signature
        {
            private fixed ulong v[SIGNATURE_UNIT_SIZE];
            public byte[] Serialize() {
                byte[] buf = new byte[SIGNATURE_SERIALIZE_SIZE];
                ulong n = blsSignatureSerialize(buf, (ulong)buf.Length, this);
                if (n == 0) {
                    throw new ArithmeticException("blsSignatureSerialize");
                }
                return buf;
            }
            public void Deserialize(byte[] buf) {
                ulong n = blsSignatureDeserialize(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("blsSignatureDeserialize");
                }
            }
            public bool IsEqual(in Signature rhs) {
                return blsSignatureIsEqual(this, rhs) != 0;
            }
            public bool IsZero()
            {
                return blsSignatureIsZero(this) != 0;
            }
            public void SetStr(string s) {
                if (blsSignatureSetHexStr(ref this, s, (ulong)s.Length) != 0) {
                    throw new ArgumentException("blsSignatureSetStr:" + s);
                }
            }
            public string GetHexStr() {
                StringBuilder sb = new StringBuilder(1024);
                ulong size = blsSignatureGetHexStr(sb, (ulong)sb.Capacity, this);
                if (size == 0) {
                    throw new ArgumentException("blsSignatureGetStr");
                }
                return sb.ToString(0, (int)size);
            }
            public void Add(in Signature rhs) {
                blsSignatureAdd(ref this, rhs);
            }
            public void Sub(in Signature rhs)
            {
                blsSignatureSub(ref this, rhs);
            }
            public void Neg()
            {
                blsSignatureNeg(ref this);
            }
            public void Mul(in SecretKey rhs)
            {
                blsSignatureMul(ref this, rhs);
            }
        }
        public static Signature RecoverSign(in Signature[] sigVec, in Id[] idVec) {
            Signature sig;
            if (blsSignatureRecover(ref sig, sigVec[0], idVec[0], (ulong)sigVec.Length) != 0) {
                throw new ArgumentException("Recover");
            }
            return sig;
        }
        public static PublicKey MulVec(in PublicKey[] pubVec, in SecretKey[] secVec)
        {
            if (pubVec.Length != secVec.Length) {
                throw new ArithmeticException("PublicKey.MulVec");
            }
            PublicKey pub;
            blsPublicKeyMulVec(ref pub, pubVec[0], secVec[0], (ulong)pubVec.Length);
            return pub;
        }
        public static Signature MulVec(in Signature[] sigVec, in SecretKey[] secVec)
        {
            if (sigVec.Length != secVec.Length) {
                throw new ArithmeticException("Signature.MulVec");
            }
            Signature sig;
            blsSignatureMulVec(ref sig, sigVec[0], secVec[0], (ulong)sigVec.Length);
            return sig;
        }
    }
}
