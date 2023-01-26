using System;
using System.Text;
using System.Runtime.InteropServices;

namespace mcl {
    public class MCL {
        public const int BN254 = 0;
        public const int BN_SNARK = 4;
        public const int BLS12_381 = 5;
        public const int MCL_MAP_TO_MODE_HASH_TO_CURVE = 5;
        public const int FR_UNIT_SIZE = 4;
        public const int FP_UNIT_SIZE = 6; // 4 if mclbn256.dll is used

        public const int G1_UNIT_SIZE = FP_UNIT_SIZE * 3;
        public const int G2_UNIT_SIZE = FP_UNIT_SIZE * 2 * 3;
        public const int GT_UNIT_SIZE = FP_UNIT_SIZE * 12;

        public const string dllName = "mclbn384_256";
        [DllImport(dllName)] public static extern int mclBn_init(int curve, int compiledTimeVar);
        [DllImport(dllName)] public static extern void mclBn_setETHserialization(int enable);
        [DllImport(dllName)] public static extern int mclBn_setMapToMode(int mode);
        [DllImport(dllName)] public static extern void mclBnFr_clear(ref Fr x);
        [DllImport(dllName)] public static extern void mclBnFr_setInt(ref Fr y, int x);
        [DllImport(dllName)] public static extern int mclBnFr_setStr(ref Fr x, [In][MarshalAs(UnmanagedType.LPStr)] string buf, long bufSize, int ioMode);
        [DllImport(dllName)] public static extern int mclBnFr_isValid(in Fr x);
        [DllImport(dllName)] public static extern int mclBnFr_isEqual(in Fr x, in Fr y);
        [DllImport(dllName)] public static extern int mclBnFr_isZero(in Fr x);
        [DllImport(dllName)] public static extern int mclBnFr_isOne(in Fr x);
        [DllImport(dllName)] public static extern void mclBnFr_setByCSPRNG(ref Fr x);

        [DllImport(dllName)] public static extern int mclBnFr_setHashOf(ref Fr x, [In][MarshalAs(UnmanagedType.LPStr)] string buf, long bufSize);
        [DllImport(dllName)] public static extern int mclBnFr_getStr([Out] StringBuilder buf, long maxBufSize, in Fr x, int ioMode);

        [DllImport(dllName)] public static extern void mclBnFr_neg(ref Fr y, in Fr x);
        [DllImport(dllName)] public static extern void mclBnFr_inv(ref Fr y, in Fr x);
        [DllImport(dllName)] public static extern void mclBnFr_sqr(ref Fr y, in Fr x);
        [DllImport(dllName)] public static extern void mclBnFr_add(ref Fr z, in Fr x, in Fr y);
        [DllImport(dllName)] public static extern void mclBnFr_sub(ref Fr z, in Fr x, in Fr y);
        [DllImport(dllName)] public static extern void mclBnFr_mul(ref Fr z, in Fr x, in Fr y);
        [DllImport(dllName)] public static extern void mclBnFr_div(ref Fr z, in Fr x, in Fr y);

        [DllImport(dllName)] public static extern void mclBnFp_clear(ref Fp x);
        [DllImport(dllName)] public static extern void mclBnFp_setInt(ref Fp y, int x);
        [DllImport(dllName)] public static extern int mclBnFp_setStr(ref Fp x, [In][MarshalAs(UnmanagedType.LPStr)] string buf, long bufSize, int ioMode);
        [DllImport(dllName)] public static extern int mclBnFp_isValid(in Fp x);
        [DllImport(dllName)] public static extern int mclBnFp_isEqual(in Fp x, in Fp y);
        [DllImport(dllName)] public static extern int mclBnFp_isZero(in Fp x);
        [DllImport(dllName)] public static extern int mclBnFp_isOne(in Fp x);
        [DllImport(dllName)] public static extern void mclBnFp_setByCSPRNG(ref Fp x);

        [DllImport(dllName)] public static extern int mclBnFp_setHashOf(ref Fp x, [In][MarshalAs(UnmanagedType.LPStr)] string buf, long bufSize);
        [DllImport(dllName)] public static extern int mclBnFp_getStr([Out] StringBuilder buf, long maxBufSize, in Fp x, int ioMode);

        [DllImport(dllName)] public static extern void mclBnFp_neg(ref Fp y, in Fp x);
        [DllImport(dllName)] public static extern void mclBnFp_inv(ref Fp y, in Fp x);
        [DllImport(dllName)] public static extern void mclBnFp_sqr(ref Fp y, in Fp x);
        [DllImport(dllName)] public static extern void mclBnFp_add(ref Fp z, in Fp x, in Fp y);
        [DllImport(dllName)] public static extern void mclBnFp_sub(ref Fp z, in Fp x, in Fp y);
        [DllImport(dllName)] public static extern void mclBnFp_mul(ref Fp z, in Fp x, in Fp y);
        [DllImport(dllName)] public static extern void mclBnFp_div(ref Fp z, in Fp x, in Fp y);

        [DllImport(dllName)] public static extern void mclBnG1_clear(ref G1 x);
        [DllImport(dllName)] public static extern int mclBnG1_setStr(ref G1 x, [In][MarshalAs(UnmanagedType.LPStr)] string buf, long bufSize, int ioMode);
        [DllImport(dllName)] public static extern int mclBnG1_isValid(in G1 x);
        [DllImport(dllName)] public static extern int mclBnG1_isEqual(in G1 x, in G1 y);
        [DllImport(dllName)] public static extern int mclBnG1_isZero(in G1 x);
        [DllImport(dllName)] public static extern int mclBnG1_hashAndMapTo(ref G1 x, [In][MarshalAs(UnmanagedType.LPStr)] string buf, long bufSize);
        [DllImport(dllName)] public static extern long mclBnG1_getStr([Out] StringBuilder buf, long maxBufSize, in G1 x, int ioMode);
        [DllImport(dllName)] public static extern void mclBnG1_neg(ref G1 y, in G1 x);
        [DllImport(dllName)] public static extern void mclBnG1_dbl(ref G1 y, in G1 x);
        [DllImport(dllName)] public static extern void mclBnG1_normalize(ref G1 y, in G1 x);
        [DllImport(dllName)] public static extern void mclBnG1_add(ref G1 z, in G1 x, in G1 y);
        [DllImport(dllName)] public static extern void mclBnG1_sub(ref G1 z, in G1 x, in G1 y);
        [DllImport(dllName)] public static extern void mclBnG1_mul(ref G1 z, in G1 x, in Fr y);
        [DllImport(dllName)] public static extern void mclBnG1_mulVec(ref G1 z, [In] G1[] x, [In] Fr[] y, long n);

        [DllImport(dllName)] public static extern void mclBnG2_clear(ref G2 x);
        [DllImport(dllName)] public static extern int mclBnG2_setStr(ref G2 x, [In][MarshalAs(UnmanagedType.LPStr)] string buf, long bufSize, int ioMode);
        [DllImport(dllName)] public static extern int mclBnG2_isValid(in G2 x);
        [DllImport(dllName)] public static extern int mclBnG2_isEqual(in G2 x, in G2 y);
        [DllImport(dllName)] public static extern int mclBnG2_isZero(in G2 x);
        [DllImport(dllName)] public static extern int mclBnG2_hashAndMapTo(ref G2 x, [In][MarshalAs(UnmanagedType.LPStr)] string buf, long bufSize);
        [DllImport(dllName)] public static extern long mclBnG2_getStr([Out] StringBuilder buf, long maxBufSize, in G2 x, int ioMode);
        [DllImport(dllName)] public static extern void mclBnG2_neg(ref G2 y, in G2 x);
        [DllImport(dllName)] public static extern void mclBnG2_dbl(ref G2 y, in G2 x);
        [DllImport(dllName)] public static extern void mclBnG2_normalize(ref G2 y, in G2 x);
        [DllImport(dllName)] public static extern void mclBnG2_add(ref G2 z, in G2 x, in G2 y);
        [DllImport(dllName)] public static extern void mclBnG2_sub(ref G2 z, in G2 x, in G2 y);
        [DllImport(dllName)] public static extern void mclBnG2_mul(ref G2 z, in G2 x, in Fr y);
        [DllImport(dllName)] public static extern void mclBnG2_mulVec(ref G2 z, [In] G2[] x, [In] Fr[] y, long n);

        [DllImport(dllName)] public static extern void mclBnGT_clear(ref GT x);
        [DllImport(dllName)] public static extern int mclBnGT_setStr(ref GT x, [In][MarshalAs(UnmanagedType.LPStr)] string buf, long bufSize, int ioMode);
        [DllImport(dllName)] public static extern int mclBnGT_isEqual(in GT x, in GT y);
        [DllImport(dllName)] public static extern int mclBnGT_isZero(in GT x);
        [DllImport(dllName)] public static extern int mclBnGT_isOne(in GT x);
        [DllImport(dllName)] public static extern long mclBnGT_getStr([Out] StringBuilder buf, long maxBufSize, in GT x, int ioMode);
        [DllImport(dllName)] public static extern void mclBnGT_neg(ref GT y, in GT x);
        [DllImport(dllName)] public static extern void mclBnGT_inv(ref GT y, in GT x);
        [DllImport(dllName)] public static extern void mclBnGT_sqr(ref GT y, in GT x);
        [DllImport(dllName)] public static extern void mclBnGT_add(ref GT z, in GT x, in GT y);
        [DllImport(dllName)] public static extern void mclBnGT_sub(ref GT z, in GT x, in GT y);
        [DllImport(dllName)] public static extern void mclBnGT_mul(ref GT z, in GT x, in GT y);
        [DllImport(dllName)] public static extern void mclBnGT_div(ref GT z, in GT x, in GT y);
        [DllImport(dllName)] public static extern void mclBnGT_pow(ref GT z, in GT x, in Fr y);
        [DllImport(dllName)] public static extern void mclBn_pairing(ref GT z, in G1 x, in G2 y);
        [DllImport(dllName)] public static extern void mclBn_finalExp(ref GT y, in GT x);
        [DllImport(dllName)] public static extern void mclBn_millerLoop(ref GT z, in G1 x, in G2 y);
        [DllImport(dllName)] public static extern ulong mclBnFp_setLittleEndianMod(ref Fp y, [In] byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong mclBnFr_setLittleEndianMod(ref Fr y, [In] byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong mclBnFp_setBigEndianMod(ref Fp y, [In] byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong mclBnFr_setBigEndianMod(ref Fr y, [In] byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern int mclBn_getFrByteSize();
        [DllImport(dllName)] public static extern int mclBn_getFpByteSize();
        [DllImport(dllName)] public static extern ulong mclBnFp_serialize([Out] byte[] buf, ulong maxBufSize, in Fp x);
        [DllImport(dllName)] public static extern ulong mclBnFr_serialize([Out] byte[] buf, ulong maxBufSize, in Fr x);
        [DllImport(dllName)] public static extern ulong mclBnG1_serialize([Out] byte[] buf, ulong maxBufSize, in G1 x);
        [DllImport(dllName)] public static extern ulong mclBnG2_serialize([Out] byte[] buf, ulong maxBufSize, in G2 x);
        [DllImport(dllName)] public static extern ulong mclBnFr_deserialize(ref Fr x, [In] byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong mclBnFp_deserialize(ref Fp x, [In] byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong mclBnG1_deserialize(ref G1 x, [In] byte[] buf, ulong bufSize);
        [DllImport(dllName)] public static extern ulong mclBnG2_deserialize(ref G2 x, [In] byte[] buf, ulong bufSize);

        [DllImport(dllName)] public static extern int mclBn_FrEvaluatePolynomial(ref Fr z, [In] Fr[] cVec, ulong cSize, in Fr x);
        [DllImport(dllName)] public static extern int mclBn_G1EvaluatePolynomial(ref G1 z, [In] G1[] cVec, ulong cSize, in Fr x);
        [DllImport(dllName)] public static extern int mclBn_G2EvaluatePolynomial(ref G2 z, [In] G2[] cVec, ulong cSize, in Fr x);
        [DllImport(dllName)] public static extern int mclBn_FrLagrangeInterpolation(ref Fr z, [In] Fr[] xVec, [In] Fr[] yVec, ulong k);
        [DllImport(dllName)] public static extern int mclBn_G1LagrangeInterpolation(ref G1 z, [In] Fr[] xVec, [In] G1[] yVec, ulong k);
        [DllImport(dllName)] public static extern int mclBn_G2LagrangeInterpolation(ref G2 z, [In] Fr[] xVec, [In] G2[] yVec, ulong k);
        public static void Init(int curveType = BN254)
        {
            if (!System.Environment.Is64BitProcess) {
                throw new PlatformNotSupportedException("not 64-bit system");
            }
            const int COMPILED_TIME_VAR = FR_UNIT_SIZE * 10 + FP_UNIT_SIZE;
            if (mclBn_init(curveType, COMPILED_TIME_VAR) != 0) {
                throw new ArgumentException("mclBn_init");
            }
        }
        public static void ETHmode()
        {
            mclBn_setETHserialization(1);
            mclBn_setMapToMode(MCL_MAP_TO_MODE_HASH_TO_CURVE);
        }
        public static void Add(ref Fr z, in Fr x, in Fr y)
        {
            mclBnFr_add(ref z, x, y);
        }
        public static void Sub(ref Fr z, in Fr x, in Fr y)
        {
            mclBnFr_sub(ref z, x, y);
        }
        public static void Mul(ref Fr z, in Fr x, in Fr y)
        {
            mclBnFr_mul(ref z, x, y);
        }
        public static void Div(ref Fr z, in Fr x, in Fr y)
        {
            mclBnFr_div(ref z, x, y);
        }
        public static void Neg(ref Fr y, in Fr x)
        {
            mclBnFr_neg(ref y, x);
        }
        public static void Inv(ref Fr y, in Fr x)
        {
            mclBnFr_inv(ref y, x);
        }
        public static void Sqr(ref Fr y, in Fr x)
        {
            mclBnFr_sqr(ref y, x);
        }

        public static void Add(ref Fp z, in Fp x, in Fp y)
        {
            mclBnFp_add(ref z, x, y);
        }
        public static void Sub(ref Fp z, in Fp x, in Fp y)
        {
            mclBnFp_sub(ref z, x, y);
        }
        public static void Mul(ref Fp z, in Fp x, in Fp y)
        {
            mclBnFp_mul(ref z, x, y);
        }
        public static void Div(ref Fp z, in Fp x, in Fp y)
        {
            mclBnFp_div(ref z, x, y);
        }
        public static void Neg(ref Fp y, in Fp x)
        {
            mclBnFp_neg(ref y, x);
        }
        public static void Inv(ref Fp y, in Fp x)
        {
            mclBnFp_inv(ref y, x);
        }
        public static void Sqr(ref Fp y, in Fp x)
        {
            mclBnFp_sqr(ref y, x);
        }
        public static void Add(ref G1 z, in G1 x, in G1 y)
        {
            mclBnG1_add(ref z, x, y);
        }
        public static void Sub(ref G1 z, in G1 x, in G1 y)
        {
            mclBnG1_sub(ref z, x, y);
        }
        public static void Mul(ref G1 z, in G1 x, in Fr y)
        {
            mclBnG1_mul(ref z, x, y);
        }
        public static void Neg(ref G1 y, in G1 x)
        {
            mclBnG1_neg(ref y, x);
        }
        public static void Dbl(ref G1 y, in G1 x)
        {
            mclBnG1_dbl(ref y, x);
        }
        public static void Normalize(ref G1 y, in G1 x)
        {
            mclBnG1_normalize(ref y, x);
        }
        public static void MulVec(ref G1 z, in G1[] x, in Fr[] y)
        {
            int n = x.Length;
            if (n <= 0 || n != y.Length) {
                throw new ArgumentException("bad length");
            }
            mclBnG1_mulVec(ref z, x, y, (long)n);
        }
        public static void Add(ref G2 z, in G2 x, in G2 y)
        {
            mclBnG2_add(ref z, x, y);
        }
        public static void Sub(ref G2 z, in G2 x, in G2 y)
        {
            mclBnG2_sub(ref z, x, y);
        }
        public static void Mul(ref G2 z, in G2 x, in Fr y)
        {
            mclBnG2_mul(ref z, x, y);
        }
        public static void Neg(ref G2 y, in G2 x)
        {
            mclBnG2_neg(ref y, x);
        }
        public static void Dbl(ref G2 y, in G2 x)
        {
            mclBnG2_dbl(ref y, x);
        }
        public static void Normalize(ref G2 y, in G2 x)
        {
            mclBnG2_normalize(ref y, x);
        }
        public static void MulVec(ref G2 z, in G2[] x, in Fr[] y)
        {
            int n = x.Length;
            if (n <= 0 || n != y.Length) {
                throw new ArgumentException("bad length");
            }
            mclBnG2_mulVec(ref z, x, y, (long)n);
        }
        public static void Add(ref GT z, in GT x, in GT y)
        {
            mclBnGT_add(ref z, x, y);
        }
        public static void Sub(ref GT z, in GT x, in GT y)
        {
            mclBnGT_sub(ref z, x, y);
        }
        public static void Mul(ref GT z, in GT x, in GT y)
        {
            mclBnGT_mul(ref z, x, y);
        }
        public static void Div(ref GT z, in GT x, in GT y)
        {
            mclBnGT_div(ref z, x, y);
        }
        public static void Neg(ref GT y, in GT x)
        {
            mclBnGT_neg(ref y, x);
        }
        public static void Inv(ref GT y, in GT x)
        {
            mclBnGT_inv(ref y, x);
        }
        public static void Sqr(ref GT y, in GT x)
        {
            mclBnGT_sqr(ref y, x);
        }
        public static void Pow(ref GT z, in GT x, in Fr y)
        {
            mclBnGT_pow(ref z, x, y);
        }
        public static void Pairing(ref GT z, in G1 x, in G2 y)
        {
            mclBn_pairing(ref z, x, y);
        }
        public static void FinalExp(ref GT y, in GT x)
        {
            mclBn_finalExp(ref y, x);
        }
        public static void MillerLoop(ref GT z, in G1 x, in G2 y)
        {
            mclBn_millerLoop(ref z, x, y);
        }
        // y = f(x) with a polynomial f(t) = sum_i cVec[i] t^i
        public static void Share(ref Fr y, in Fr[] cVec, in Fr x)
        {
            ulong k = (ulong)cVec.Length;
            int ret = mclBn_FrEvaluatePolynomial(ref y, cVec, k, x);
            if (ret != 0) {
                throw new ArgumentException("mclBn_FrEvaluatePolynomial");
            }
        }
        public static void Share(ref G1 y, in G1[] cVec, in Fr x)
        {
            ulong k = (ulong)cVec.Length;
            int ret = mclBn_G1EvaluatePolynomial(ref y, cVec, k, x);
            if (ret != 0) {
                throw new ArgumentException("mclBn_G1EvaluatePolynomial");
            }
        }
        public static void Share(ref G2 y, in G2[] cVec, in Fr x)
        {
            ulong k = (ulong)cVec.Length;
            int ret = mclBn_G2EvaluatePolynomial(ref y, cVec, k, x);
            if (ret != 0) {
                throw new ArgumentException("mclBn_G2EvaluatePolynomial");
            }
        }
        // recover z by Lagrange interpolation with {xVec[i], yVec[i]}
        public static void Recover(ref Fr z, in Fr[] xVec, in Fr[] yVec)
        {
            if (xVec.Length != yVec.Length) {
                throw new ArgumentException("bad length");
            }
            ulong k = (ulong)xVec.Length;
            int ret = mclBn_FrLagrangeInterpolation(ref z, xVec, yVec, k);
            if (ret != 0) {
                throw new ArgumentException("mclBn_FrLagrangeInterpolation:" + ret.ToString());
            }
        }
        public static void Recover(ref G1 z, in Fr[] xVec, in G1[] yVec)
        {
            if (xVec.Length != yVec.Length) {
                throw new ArgumentException("bad length");
            }
            ulong k = (ulong)xVec.Length;
            int ret = mclBn_G1LagrangeInterpolation(ref z, xVec, yVec, k);
            if (ret != 0) {
                throw new ArgumentException("mclBn_G1LagrangeInterpolation:" + ret.ToString());
            }
        }
        public static void Recover(ref G2 z, in Fr[] xVec, in G2[] yVec)
        {
            if (xVec.Length != yVec.Length) {
                throw new ArgumentException("bad length");
            }
            ulong k = (ulong)xVec.Length;
            int ret = mclBn_G2LagrangeInterpolation(ref z, xVec, yVec, k);
            if (ret != 0) {
                throw new ArgumentException("mclBn_G2LagrangeInterpolation:" + ret.ToString());
            }
        }
        [StructLayout(LayoutKind.Sequential)]
        struct U128 {
            private ulong v0, v1;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Fr {
            private U128 v0, v1;
            public static Fr One()
            {
                var x = new Fr();
                x.SetInt(1);
                return x;
            }
            public static Fr Zero() => new Fr();
            public void Clear()
            {
                mclBnFr_clear(ref this);
            }
            public void SetInt(int x)
            {
                mclBnFr_setInt(ref this, x);
            }
            public void SetStr(string s, int ioMode)
            {
                if (mclBnFr_setStr(ref this, s, s.Length, ioMode) != 0) {
                    throw new ArgumentException("mclBnFr_setStr" + s);
                }
            }
            public bool IsValid()
            {
                return mclBnFr_isValid(this) == 1;
            }
            public bool Equals(in Fr rhs)
            {
                return mclBnFr_isEqual(this, rhs) == 1;
            }
            public bool IsZero()
            {
                return mclBnFr_isZero(this) == 1;
            }
            public bool IsOne()
            {
                return mclBnFr_isOne(this) == 1;
            }
            public void SetByCSPRNG()
            {
                mclBnFr_setByCSPRNG(ref this);
            }
            public void SetHashOf(String s)
            {
                if (mclBnFr_setHashOf(ref this, s, s.Length) != 0) {
                    throw new InvalidOperationException("mclBnFr_setHashOf:" + s);
                }
            }
            public string GetStr(int ioMode)
            {
                StringBuilder sb = new StringBuilder(1024);
                long size = mclBnFr_getStr(sb, sb.Capacity, this, ioMode);
                if (size == 0) {
                    throw new InvalidOperationException("mclBnFr_getStr:");
                }
                return sb.ToString();
            }
            public byte[] Serialize()
            {
                byte[] buf = new byte[mclBn_getFrByteSize()];
                ulong n = mclBnFr_serialize(buf, (ulong)buf.Length, this);
                if (n != (ulong)buf.Length) {
                    throw new ArithmeticException("mclBnFr_serialize");
                }
                return buf;
            }
            public void Deserialize(byte[] buf)
            {
                ulong n = mclBnFr_deserialize(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("mclBnFr_deserialize");
                }
            }
            public void SetLittleEndianMod(byte[] buf)
            {
                ulong n = mclBnFr_setLittleEndianMod(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("mclBnFr_setLittleEndianMod");
                }
            }
            public void SetBigEndianMod(byte[] buf)
            {
                ulong n = mclBnFr_setBigEndianMod(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("mclBnFr_setBigEndianMod");
                }
            }

            public void Neg(in Fr x)
            {
                mclBnFr_neg(ref this, x);
            }
            public void Inv(in Fr x)
            {
                mclBnFr_inv(ref this, x);
            }
            public void Sqr(in Fr x)
            {
                MCL.Sqr(ref this, x);
            }
            public void Add(in Fr x, in Fr y)
            {
                MCL.Add(ref this, x, y);
            }
            public void Sub(in Fr x, in Fr y)
            {
                MCL.Sub(ref this, x, y);
            }
            public void Mul(in Fr x, in Fr y)
            {
                MCL.Mul(ref this, x, y);
            }
            public void Div(in Fr x, in Fr y)
            {
                MCL.Div(ref this, x, y);
            }
            public static Fr operator -(in Fr x)
            {
                Fr y = new Fr();
                y.Neg(x);
                return y;
            }
            public static Fr operator +(in Fr x, in Fr y)
            {
                Fr z = new Fr();
                z.Add(x, y);
                return z;
            }
            public static Fr operator -(in Fr x, in Fr y)
            {
                Fr z = new Fr();
                z.Sub(x, y);
                return z;
            }
            public static Fr operator *(in Fr x, in Fr y)
            {
                Fr z = new Fr();
                z.Mul(x, y);
                return z;
            }
            public static Fr operator /(in Fr x, in Fr y)
            {
                Fr z = new Fr();
                z.Div(x, y);
                return z;
            }
        }
        [StructLayout(LayoutKind.Sequential)]
        public struct Fp {
            private U128 v0, v1, v2;
            public static Fp One()
            {
                var x = new Fp();
                x.SetInt(1);
                return x;
            }
            public static Fp Zero() => new Fp();
            public void Clear()
            {
                mclBnFp_clear(ref this);
            }
            public void SetInt(int x)
            {
                mclBnFp_setInt(ref this, x);
            }
            public void SetStr(string s, int ioMode)
            {
                if (mclBnFp_setStr(ref this, s, s.Length, ioMode) != 0) {
                    throw new ArgumentException("mclBnFp_setStr" + s);
                }
            }
            public bool IsValid()
            {
                return mclBnFp_isValid(this) == 1;
            }
            public bool Equals(in Fp rhs)
            {
                return mclBnFp_isEqual(this, rhs) == 1;
            }
            public bool IsZero()
            {
                return mclBnFp_isZero(this) == 1;
            }
            public bool IsOne()
            {
                return mclBnFp_isOne(this) == 1;
            }
            public void SetByCSPRNG()
            {
                mclBnFp_setByCSPRNG(ref this);
            }
            public string GetStr(int ioMode)
            {
                StringBuilder sb = new StringBuilder(1024);
                long size = mclBnFp_getStr(sb, sb.Capacity, this, ioMode);
                if (size == 0) {
                    throw new InvalidOperationException("mclBnFp_getStr:");
                }
                return sb.ToString();
            }
            public byte[] Serialize()
            {
                byte[] buf = new byte[mclBn_getFpByteSize()];
                ulong n = mclBnFp_serialize(buf, (ulong)buf.Length, this);
                if (n != (ulong)buf.Length) {
                    throw new ArithmeticException("mclBnFp_serialize");
                }
                return buf;
            }
            public void Deserialize(byte[] buf)
            {
                ulong n = mclBnFp_deserialize(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("mclBnFp_deserialize");
                }
            }
            public void SetLittleEndianMod(byte[] buf)
            {
                ulong n = mclBnFp_setLittleEndianMod(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("mclBnFp_setLittleEndianMod");
                }
            }
            public void SetBigEndianMod(byte[] buf)
            {
                ulong n = mclBnFp_setBigEndianMod(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("mclBnFp_setBigEndianMod");
                }
            }
            public void Neg(in Fp x)
            {
                MCL.Neg(ref this, x);
            }
            public void Inv(in Fp x)
            {
                MCL.Inv(ref this, x);
            }
            public void Sqr(in Fp x)
            {
                MCL.Sqr(ref this, x);
            }
            public void Add(in Fp x, in Fp y)
            {
                MCL.Add(ref this, x, y);
            }
            public void Sub(in Fp x, in Fp y)
            {
                MCL.Sub(ref this, x, y);
            }
            public void Mul(in Fp x, in Fp y)
            {
                MCL.Mul(ref this, x, y);
            }
            public void Div(in Fp x, in Fp y)
            {
                MCL.Div(ref this, x, y);
            }
            public static Fp operator -(in Fp x)
            {
                Fp y = new Fp();
                y.Neg(x);
                return y;
            }
            public static Fp operator +(in Fp x, in Fp y)
            {
                Fp z = new Fp();
                z.Add(x, y);
                return z;
            }
            public static Fp operator -(in Fp x, in Fp y)
            {
                Fp z = new Fp();
                z.Sub(x, y);
                return z;
            }
            public static Fp operator *(in Fp x, in Fp y)
            {
                Fp z = new Fp();
                z.Mul(x, y);
                return z;
            }
            public static Fp operator /(in Fp x, in Fp y)
            {
                Fp z = new Fp();
                z.Div(x, y);
                return z;
            }
        }
        [StructLayout(LayoutKind.Sequential)]
        public struct Fp2 {
            public Fp a, b;
        }
        [StructLayout(LayoutKind.Sequential)]
        public struct G1 {
            public Fp x, y, z;
            public static G1 Zero()
            {
                var g1 = new G1();
                return g1;
            }
            public void Clear()
            {
                mclBnG1_clear(ref this);
            }
            public void SetStr(string s, int ioMode)
            {
                if (mclBnG1_setStr(ref this, s, s.Length, ioMode) != 0) {
                    throw new ArgumentException("mclBnG1_setStr:" + s);
                }
            }
            public bool IsValid()
            {
                return mclBnG1_isValid(this) == 1;
            }
            public bool Equals(in G1 rhs)
            {
                return mclBnG1_isEqual(this, rhs) == 1;
            }
            public bool IsZero()
            {
                return mclBnG1_isZero(this) == 1;
            }
            public void HashAndMapTo(String s)
            {
                if (mclBnG1_hashAndMapTo(ref this, s, s.Length) != 0) {
                    throw new ArgumentException("mclBnG1_hashAndMapTo:" + s);
                }
            }
            public void SetHashOf(String s)
            {
                HashAndMapTo(s);
            }
            public string GetStr(int ioMode)
            {
                StringBuilder sb = new StringBuilder(1024);
                long size = mclBnG1_getStr(sb, sb.Capacity, this, ioMode);
                if (size == 0) {
                    throw new InvalidOperationException("mclBnG1_getStr:");
                }
                return sb.ToString();
            }
            public byte[] Serialize()
            {
                byte[] buf = new byte[mclBn_getFpByteSize()];
                ulong n = mclBnG1_serialize(buf, (ulong)buf.Length, this);
                if (n != (ulong)buf.Length) {
                    throw new ArithmeticException("mclBnG1_serialize");
                }
                return buf;
            }
            public void Deserialize(byte[] buf)
            {
                ulong n = mclBnG1_deserialize(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("mclBnG1_deserialize");
                }
            }
            public void Neg(in G1 x)
            {
                MCL.Neg(ref this, x);
            }
            public void Dbl(in G1 x)
            {
                MCL.Dbl(ref this, x);
            }
            public void Normalize(in G1 x)
            {
                MCL.Normalize(ref this, x);
            }
            public void Add(in G1 x, in G1 y)
            {
                MCL.Add(ref this, x, y);
            }
            public void Sub(in G1 x, in G1 y)
            {
                MCL.Sub(ref this, x, y);
            }
            public void Mul(in G1 x, in Fr y)
            {
                MCL.Mul(ref this, x, y);
            }
            public static G1 operator -(in G1 x)
            {
                var result = new G1();
                result.Neg(x);
                return result;
            }
            public static G1 operator +(in G1 left, in G1 right)
            {
                var result = new G1();
                result.Add(left, right);
                return result;
            }
            public static G1 operator -(in G1 left, in G1 right)
            {
                var result = new G1();
                result.Sub(left, right);
                return result;
            }
            public static G1 operator *(in G1 left, in Fr right)
            {
                var result = new G1();
                result.Mul(left, right);
                return result;
            }
        }
        [StructLayout(LayoutKind.Sequential)]
        public struct G2 {
            public Fp2 x, y, z;
            public static G2 Zero()
            {
                var g2 = new G2();
                return g2;
            }
            public void Clear()
            {
                mclBnG2_clear(ref this);
            }
            public void SetStr(string s, int ioMode)
            {
                if (mclBnG2_setStr(ref this, s, s.Length, ioMode) != 0) {
                    throw new ArgumentException("mclBnG2_setStr:" + s);
                }
            }
            public bool IsValid()
            {
                return mclBnG2_isValid(this) == 1;
            }
            public bool Equals(in G2 rhs)
            {
                return mclBnG2_isEqual(this, rhs) == 1;
            }
            public bool IsZero()
            {
                return mclBnG2_isZero(this) == 1;
            }
            public void HashAndMapTo(String s)
            {
                if (mclBnG2_hashAndMapTo(ref this, s, s.Length) != 0) {
                    throw new ArgumentException("mclBnG2_hashAndMapTo:" + s);
                }
            }
            public void SetHashOf(String s)
            {
                HashAndMapTo(s);
            }
            public string GetStr(int ioMode)
            {
                StringBuilder sb = new StringBuilder(1024);
                long size = mclBnG2_getStr(sb, sb.Capacity, this, ioMode);
                if (size == 0) {
                    throw new InvalidOperationException("mclBnG2_getStr:");
                }
                return sb.ToString();
            }
            public byte[] Serialize()
            {
                byte[] buf = new byte[mclBn_getFpByteSize() * 2];
                ulong n = mclBnG2_serialize(buf, (ulong)buf.Length, this);
                if (n != (ulong)buf.Length) {
                    throw new ArithmeticException("mclBnG2_serialize");
                }
                return buf;
            }
            public void Deserialize(byte[] buf)
            {
                ulong n = mclBnG2_deserialize(ref this, buf, (ulong)buf.Length);
                if (n == 0) {
                    throw new ArithmeticException("mclBnG2_deserialize");
                }
            }
            public void Neg(in G2 x)
            {
                MCL.Neg(ref this, x);
            }
            public void Dbl(in G2 x)
            {
                MCL.Dbl(ref this, x);
            }
            public void Normalize(in G2 x)
            {
                MCL.Normalize(ref this, x);
            }
            public void Add(in G2 x, in G2 y)
            {
                MCL.Add(ref this, x, y);
            }
            public void Sub(in G2 x, in G2 y)
            {
                MCL.Sub(ref this, x, y);
            }
            public void Mul(in G2 x, in Fr y)
            {
                MCL.Mul(ref this, x, y);
            }
            public static G2 operator -(in G2 x)
            {
                var result = new G2();
                result.Neg(x);
                return result;
            }
            public static G2 operator +(in G2 x, in G2 y)
            {
                var z = new G2();
                z.Add(x, y);
                return z;
            }
            public static G2 operator -(in G2 x, in G2 y)
            {
                var z = new G2();
                z.Sub(x, y);
                return z;
            }
            public static G2 operator *(in G2 x, in Fr y)
            {
                var z = new G2();
                z.Mul(x, y);
                return z;
            }
        }
        [StructLayout(LayoutKind.Sequential)]
        public struct GT {
            public Fp v00, v01, v02, v03, v04, v05, v06, v07, v08, v09, v10, v11;
            public void Clear()
            {
                mclBnGT_clear(ref this);
            }
            public void SetStr(String s, int ioMode)
            {
                if (mclBnGT_setStr(ref this, s, s.Length, ioMode) != 0) {
                    throw new ArgumentException("mclBnGT_setStr:" + s);
                }
            }
            public bool Equals(in GT rhs)
            {
                return mclBnGT_isEqual(this, rhs) == 1;
            }
            public bool IsZero()
            {
                return mclBnGT_isZero(this) == 1;
            }
            public bool IsOne()
            {
                return mclBnGT_isOne(this) == 1;
            }
            public string GetStr(int ioMode)
            {
                StringBuilder sb = new StringBuilder(2048);
                long size = mclBnGT_getStr(sb, sb.Capacity, this, ioMode);
                if (size == 0) {
                    throw new InvalidOperationException("mclBnGT_getStr:");
                }
                return sb.ToString();
            }
            public void Neg(in GT x)
            {
                MCL.Neg(ref this, x);
            }
            public void Inv(in GT x)
            {
                MCL.Inv(ref this, x);
            }
            public void Sqr(in GT x)
            {
                MCL.Sqr(ref this, x);
            }
            public void Add(in GT x, in GT y)
            {
                MCL.Add(ref this, x, y);
            }
            public void Sub(in GT x, in GT y)
            {
                MCL.Sub(ref this, x, y);
            }
            public void Mul(in GT x, in GT y)
            {
                MCL.Mul(ref this, x, y);
            }
            public void Div(in GT x, in GT y)
            {
                MCL.Div(ref this, x, y);
            }
            public static GT operator -(in GT x)
            {
                GT y = new GT();
                y.Neg(x);
                return y;
            }
            public static GT operator +(in GT x, in GT y)
            {
                GT z = new GT();
                z.Add(x, y);
                return z;
            }
            public static GT operator -(in GT x, in GT y)
            {
                GT z = new GT();
                z.Sub(x, y);
                return z;
            }
            public static GT operator *(in GT x, in GT y)
            {
                GT z = new GT();
                z.Mul(x, y);
                return z;
            }
            public static GT operator /(in GT x, in GT y)
            {
                GT z = new GT();
                z.Div(x, y);
                return z;
            }
            public void Pow(in GT x, in Fr y)
            {
                MCL.Pow(ref this, x, y);
            }
            public void Pairing(in G1 x, in G2 y)
            {
                MCL.Pairing(ref this, x, y);
            }
            public void FinalExp(in GT x)
            {
                MCL.FinalExp(ref this, x);
            }
            public void MillerLoop(in G1 x, in G2 y)
            {
                MCL.MillerLoop(ref this, x, y);
            }
        }
    }
}
