using System;

namespace mcl {
	using static BN256;
	class BN256Test {
		static int err = 0;
		static void assert(string msg, bool b)
		{
			if (b) return;
			Console.WriteLine("ERR {0}", msg);
			err++;
		}
		static void Main(string[] args)
		{
			try {
				assert("64bit system", System.Environment.Is64BitProcess);
				init();
				TestFr();
				TestG1();
				TestG2();
				TestPairing();
				if (err == 0) {
					Console.WriteLine("all tests succeed");
				} else {
					Console.WriteLine("err={0}", err);
				}
			} catch (Exception e) {
				Console.WriteLine("ERR={0}", e);
			}
		}
		static void TestFr()
		{
			Console.WriteLine("TestFr");
			Fr x = new Fr();
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
			try {
				x.SetStr("1234567891234x", 10);
				Console.WriteLine("x = {0}", x);
			} catch (Exception e) {
				Console.WriteLine("exception test OK\n'{0}'", e);
			}
			x.SetStr("1234567891234", 10);
			assert("1234567891234", x.GetStr(10) == "1234567891234");
		}
		static void TestG1()
		{
			Console.WriteLine("TestG1");
			G1 P = new G1();
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
		}
		static void TestG2()
		{
			Console.WriteLine("TestG2");
			G2 P = new G2();
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
	}
}
