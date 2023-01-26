using System;

namespace mcl {
    using static MCL;
    class MCLTest {
        static int err = 0;
        static void assert(string msg, bool b)
        {
            if (b) return;
            Console.WriteLine("ERR {0}", msg);
            err++;
        }
        static void Main()
        {
            err = 0;
            try {
                Console.WriteLine("BN254");
                TestCurve(BN254);
                Console.WriteLine("BN_SNARK");
                TestCurve(BN_SNARK);
                Console.WriteLine("BLS12_381");
                TestCurve(BLS12_381);
                Console.WriteLine("BLS12_381 eth");
                ETHmode();
                TestETH();
                if (err == 0) {
                    Console.WriteLine("all tests succeed");
                } else {
                    Console.WriteLine("err={0}", err);
                }
            } catch (Exception e) {
                Console.WriteLine("ERR={0}", e);
            }
        }

        static void TestCurve(int curveType)
        {
            Init(curveType);
            TestFr();
            TestFp();
            TestG1();
            TestG2();
            TestPairing();
            TestSS();
        }
        static void TestFr()
        {
            Console.WriteLine("TestFr");
            Fr x = new Fr();
            assert("x.isZero", x.IsZero());
            x.Clear();
            assert("0", x.GetStr(10) == "0");
            assert("0.IzZero", x.IsZero());
            assert("!0.IzOne", !x.IsOne());
            x.SetInt(1);
            assert("1", x.GetStr(10) == "1");
            assert("!1.IzZero", !x.IsZero());
            assert("1.IzOne", x.IsOne());
            x.SetInt(3);
            assert("3", x.GetStr(10) == "3");
            assert("!3.IzZero", !x.IsZero());
            assert("!3.IzOne", !x.IsOne());
            x.SetInt(-5);
            x = -x;
            assert("5", x.GetStr(10) == "5");
            x.SetInt(4);
            x = x * x;
            assert("16", x.GetStr(10) == "16");
            assert("10", x.GetStr(16) == "10");
            Fr y;
            y = x;
            assert("x == y", x.Equals(y));
            x.SetInt(123);
            assert("123", x.GetStr(10) == "123");
            assert("7b", x.GetStr(16) == "7b");
            assert("y != x", !x.Equals(y));
            Console.WriteLine("exception test");
            try {
                x.SetStr("1234567891234x", 10);
                Console.WriteLine("x = {0}", x);
            } catch (Exception e) {
                Console.WriteLine("OK ; expected exception: {0}", e);
            }
            x.SetStr("1234567891234", 10);
            assert("1234567891234", x.GetStr(10) == "1234567891234");
            {
                byte[] buf = x.Serialize();
                y.Deserialize(buf);
                assert("x == y", x.Equals(y));
            }
        }
        static void TestFp()
        {
            Console.WriteLine("TestFp");
            Fp x = new Fp();
            assert("x.isZero", x.IsZero());
            x.Clear();
            assert("0", x.GetStr(10) == "0");
            assert("0.IzZero", x.IsZero());
            assert("!0.IzOne", !x.IsOne());
            x.SetInt(1);
            assert("1", x.GetStr(10) == "1");
            assert("!1.IzZero", !x.IsZero());
            assert("1.IzOne", x.IsOne());
            x.SetInt(3);
            assert("3", x.GetStr(10) == "3");
            assert("!3.IzZero", !x.IsZero());
            assert("!3.IzOne", !x.IsOne());
            x.SetInt(-5);
            x = -x;
            assert("5", x.GetStr(10) == "5");
            x.SetInt(4);
            x = x * x;
            assert("16", x.GetStr(10) == "16");
            assert("10", x.GetStr(16) == "10");
            Fp y;
            y = x;
            assert("x == y", x.Equals(y));
            x.SetInt(123);
            assert("123", x.GetStr(10) == "123");
            assert("7b", x.GetStr(16) == "7b");
            assert("y != x", !x.Equals(y));
            Console.WriteLine("exception test");
            try {
                x.SetStr("1234567891234x", 10);
                Console.WriteLine("x = {0}", x);
            } catch (Exception e) {
                Console.WriteLine("OK ; expected exception: {0}", e);
            }
            x.SetStr("1234567891234", 10);
            assert("1234567891234", x.GetStr(10) == "1234567891234");
            {
                byte[] buf = x.Serialize();
                y.Deserialize(buf);
                assert("x == y", x.Equals(y));
            }
        }
        static void TestG1()
        {
            Console.WriteLine("TestG1");
            G1 P = new G1();
            assert("P.isZero", P.IsZero());
            P.Clear();
            assert("P.IsValid", P.IsValid());
            assert("P.IsZero", P.IsZero());
            P.HashAndMapTo("abc");
            assert("P.IsValid", P.IsValid());
            assert("!P.IsZero", !P.IsZero());
            G1 Q = new G1();
            Q = P;
            assert("P == Q", Q.Equals(P));
            Q.Neg(P);
            Q.Add(Q, P);
            assert("P = Q", Q.IsZero());
            Q.Dbl(P);
            G1 R = new G1();
            R.Add(P, P);
            assert("Q == R", Q.Equals(R));
            Fr x = new Fr();
            x.SetInt(3);
            R.Add(R, P);
            Q.Mul(P, x);
            assert("Q == R", Q.Equals(R));
            {
                byte[] buf = P.Serialize();
                Q.Clear();
                Q.Deserialize(buf);
                assert("P == Q", P.Equals(Q));
            }
            {
                const int n = 5;
                G1[] xVec = new G1[n];
                Fr[] yVec = new Fr[n];
                P.Clear();
                for (int i = 0; i < n; i++) {
                    xVec[i].HashAndMapTo(i.ToString());
                    yVec[i].SetByCSPRNG();
                    Q.Mul(xVec[i], yVec[i]);
                    P.Add(P, Q);
                }
                MulVec(ref Q, xVec, yVec);
                assert("mulVecG1", P.Equals(Q));
            }
            G1 W = G1.Zero();
            assert("W.IsZero", W.IsZero());
        }
        static void TestG2()
        {
            Console.WriteLine("TestG2");
            G2 P = new G2();
            assert("P.isZero", P.IsZero());
            P.Clear();
            assert("P is valid", P.IsValid());
            assert("P is zero", P.IsZero());
            P.HashAndMapTo("abc");
            assert("P is valid", P.IsValid());
            assert("P is not zero", !P.IsZero());
            G2 Q = new G2();
            Q = P;
            assert("P == Q", Q.Equals(P));
            Q.Neg(P);
            Q.Add(Q, P);
            assert("Q is zero", Q.IsZero());
            Q.Dbl(P);
            G2 R = new G2();
            R.Add(P, P);
            assert("Q == R", Q.Equals(R));
            Fr x = new Fr();
            x.SetInt(3);
            R.Add(R, P);
            Q.Mul(P, x);
            assert("Q == R", Q.Equals(R));
            {
                byte[] buf = P.Serialize();
                Q.Clear();
                Q.Deserialize(buf);
                assert("P == Q", P.Equals(Q));
            }
            {
                const int n = 5;
                G2[] xVec = new G2[n];
                Fr[] yVec = new Fr[n];
                P.Clear();
                for (int i = 0; i < n; i++) {
                    xVec[i].HashAndMapTo(i.ToString());
                    yVec[i].SetByCSPRNG();
                    Q.Mul(xVec[i], yVec[i]);
                    P.Add(P, Q);
                }
                MulVec(ref Q, xVec, yVec);
                assert("mulVecG2", P.Equals(Q));
            }
            G2 W = G2.Zero();
            assert("W.IsZero", W.IsZero());
        }
        static void TestPairing()
        {
            Console.WriteLine("TestG2");
            G1 P = new G1();
            P.HashAndMapTo("123");
            G2 Q = new G2();
            Q.HashAndMapTo("1");
            Fr a = new Fr();
            Fr b = new Fr();
            a.SetStr("12345678912345673453", 10);
            b.SetStr("230498230982394243424", 10);
            G1 aP = new G1();
            G2 bQ = new G2();
            aP.Mul(P, a);
            bQ.Mul(Q, b);
            GT e1 = new GT();
            GT e2 = new GT();
            GT e3 = new GT();
            e1.Pairing(P, Q);
            e2.Pairing(aP, Q);
            e3.Pow(e1, a);
            assert("e2.Equals(e3)", e2.Equals(e3));
            e2.Pairing(P, bQ);
            e3.Pow(e1, b);
            assert("e2.Equals(e3)", e2.Equals(e3));
        }
        static void TestETH_mapToG1()
        {
            var tbl = new[] {
                new {
                    msg = "asdf",
                    x = "a72df17570d0eb81260042edbea415ad49bdb94a1bc1ce9d1bf147d0d48268170764bb513a3b994d662e1faba137106",
                    y = "122b77eca1ed58795b7cd456576362f4f7bd7a572a29334b4817898a42414d31e9c0267f2dc481a4daf8bcf4a460322",
                },
           };
            G1 P = new G1();
            Fp x = new Fp();
            Fp y = new Fp();
            foreach (var v in tbl) {
                P.HashAndMapTo(v.msg);
                x.SetStr(v.x, 16);
                y.SetStr(v.y, 16);
                Normalize(ref P, P);
                Console.WriteLine("x={0}", P.x.GetStr(16));
                Console.WriteLine("y={0}", P.y.GetStr(16));
                assert("P.x", P.x.Equals(x));
                assert("P.y", P.y.Equals(y));
            }
        }
        static void TestETH()
        {
            TestETH_mapToG1();
        }
        static void TestSS_Fr()
        {
            const int n = 5;
            const int k = 3; // can't change because the following loop
            Fr[] cVec = new Fr[k];
            // init polynomial coefficient
            for (int i = 0; i < k; i++) {
                cVec[i].SetByCSPRNG();
            }

            Fr[] xVec = new Fr[n];
            Fr[] yVec = new Fr[n];
            // share cVec[0] with yVec[0], ..., yVec[n-1]
            for (int i = 0; i < n; i++) {
                xVec[i].SetHashOf(i.ToString());
                MCL.Share(ref yVec[i], cVec, xVec[i]);
            }
            // recover cVec[0] from xVecSubset and yVecSubset
            Fr[] xVecSubset = new Fr[k];
            Fr[] yVecSubset = new Fr[k];
            for (int i0 = 0; i0 < n; i0++) {
                xVecSubset[0] = xVec[i0];
                yVecSubset[0] = yVec[i0];
                for (int i1 = i0 + 1; i1 < n; i1++) {
                    xVecSubset[1] = xVec[i1];
                    yVecSubset[1] = yVec[i1];
                    for (int i2 = i1 + 1; i2 < n; i2++) {
                        xVecSubset[2] = xVec[i2];
                        yVecSubset[2] = yVec[i2];
                        Fr s = new Fr();
                        MCL.Recover(ref s, xVecSubset, yVecSubset);
                        assert("Recover", s.Equals(cVec[0]));
                    }
                }
            }
        }
        static void TestSS_G1()
        {
            const int n = 5;
            const int k = 3; // can't change because the following loop
            G1[] cVec = new G1[k];
            // init polynomial coefficient
            for (int i = 0; i < k; i++) {
                Fr x = new Fr();
                x.SetByCSPRNG();
                cVec[i].SetHashOf(x.GetStr(16));
            }

            Fr[] xVec = new Fr[n];
            G1[] yVec = new G1[n];
            // share cVec[0] with yVec[0], ..., yVec[n-1]
            for (int i = 0; i < n; i++) {
                xVec[i].SetHashOf(i.ToString());
                MCL.Share(ref yVec[i], cVec, xVec[i]);
            }
            // recover cVec[0] from xVecSubset and yVecSubset
            Fr[] xVecSubset = new Fr[k];
            G1[] yVecSubset = new G1[k];
            for (int i0 = 0; i0 < n; i0++) {
                xVecSubset[0] = xVec[i0];
                yVecSubset[0] = yVec[i0];
                for (int i1 = i0 + 1; i1 < n; i1++) {
                    xVecSubset[1] = xVec[i1];
                    yVecSubset[1] = yVec[i1];
                    for (int i2 = i1 + 1; i2 < n; i2++) {
                        xVecSubset[2] = xVec[i2];
                        yVecSubset[2] = yVec[i2];
                        G1 s = new G1();
                        MCL.Recover(ref s, xVecSubset, yVecSubset);
                        assert("Recover", s.Equals(cVec[0]));
                    }
                }
            }
        }
        static void TestSS_G2()
        {
            const int n = 5;
            const int k = 3; // can't change because the following loop
            G2[] cVec = new G2[k];
            // init polynomial coefficient
            for (int i = 0; i < k; i++) {
                Fr x = new Fr();
                x.SetByCSPRNG();
                cVec[i].SetHashOf(x.GetStr(16));
            }

            Fr[] xVec = new Fr[n];
            G2[] yVec = new G2[n];
            // share cVec[0] with yVec[0], ..., yVec[n-1]
            for (int i = 0; i < n; i++) {
                xVec[i].SetHashOf(i.ToString());
                MCL.Share(ref yVec[i], cVec, xVec[i]);
            }
            // recover cVec[0] from xVecSubset and yVecSubset
            Fr[] xVecSubset = new Fr[k];
            G2[] yVecSubset = new G2[k];
            for (int i0 = 0; i0 < n; i0++) {
                xVecSubset[0] = xVec[i0];
                yVecSubset[0] = yVec[i0];
                for (int i1 = i0 + 1; i1 < n; i1++) {
                    xVecSubset[1] = xVec[i1];
                    yVecSubset[1] = yVec[i1];
                    for (int i2 = i1 + 1; i2 < n; i2++) {
                        xVecSubset[2] = xVec[i2];
                        yVecSubset[2] = yVec[i2];
                        G2 s = new G2();
                        MCL.Recover(ref s, xVecSubset, yVecSubset);
                        assert("Recover", s.Equals(cVec[0]));
                    }
                }
            }
        }
        static void TestSS()
        {
            TestSS_Fr();
            TestSS_G1();
            TestSS_G2();
        }
    }
}
