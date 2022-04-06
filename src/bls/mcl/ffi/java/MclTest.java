import java.io.*;
import com.herumi.mcl.*;

/*
	MclTest
*/
public class MclTest {
	static {
		String lib = "mcljava";
		String libName = System.mapLibraryName(lib);
		System.out.println("libName : " + libName);
		System.loadLibrary(lib);
	}
	public static int errN = 0;
	public static void assertEquals(String msg, String x, String y) {
		if (x.equals(y)) {
			System.out.println("OK : " + msg);
		} else {
			System.out.println("NG : " + msg + ", x = " + x + ", y = " + y);
			errN++;
		}
	}
	public static void assertBool(String msg, boolean b) {
		if (b) {
			System.out.println("OK : " + msg);
		} else {
			System.out.println("NG : " + msg);
			errN++;
		}
	}
	public static void testCurve(int curveType, String name) {
		try {
			System.out.println("curve=" + name);
			Mcl.SystemInit(curveType);
			Fr x = new Fr(5);
			Fr y = new Fr(-2);
			Fr z = new Fr(5);
			assertBool("x != y", !x.equals(y));
			assertBool("x == z", x.equals(z));
			assertEquals("x == 5", x.toString(), "5");
			Mcl.add(x, x, y);
			assertEquals("x == 3", x.toString(), "3");
			Mcl.mul(x, x, x);
			assertEquals("x == 9", x.toString(), "9");
			assertEquals("x == 12", (new Fr("12")).toString(), "12");
			assertEquals("x == 18", (new Fr("12", 16)).toString(), "18");
			assertEquals("x == ff", (new Fr("255")).toString(16), "ff");
			Mcl.inv(y, x);
			Mcl.mul(x, y, x);
			assertBool("x == 1", x.isOne());

			{
				byte[] b = x.serialize();
				Fr t = new Fr();
				t.deserialize(b);
				assertBool("serialize", x.equals(t));
				t.setLittleEndianMod(b);
				assertBool("setLittleEndianMod", x.equals(t));
				t.setHashOf(b);
				assertBool("setHashOf", !x.equals(t));
				Fr u = new Fr();
				u.setHashOf(new byte[]{1,2,3});
				assertBool("setHashOf - different", !u.equals(t));
			}
			G1 P = new G1();
			System.out.println("P=" + P);
			Mcl.hashAndMapToG1(P, "test".getBytes());
			System.out.println("P=" + P);
			byte[] buf = { 1, 2, 3, 4 };
			Mcl.hashAndMapToG1(P, buf);
			System.out.println("P=" + P);
			Mcl.neg(P, P);
			System.out.println("P=" + P);
			{
				byte[] b = P.serialize();
				G1 t = new G1();
				t.deserialize(b);
				assertBool("serialize", P.equals(t));
			}

			G2 Q = new G2();
			Mcl.hashAndMapToG2(Q, "abc".getBytes());
			System.out.println("Q=" + Q);

			Mcl.hashAndMapToG1(P, "This is a pen".getBytes());
			{
				String s = P.toString();
				G1 P1 = new G1();
				P1.setStr(s);
				assertBool("P == P1", P1.equals(P));
			}
			{
				byte[] b = Q.serialize();
				G2 t = new G2();
				t.deserialize(b);
				assertBool("serialize", Q.equals(t));
			}

			GT e = new GT();
			Mcl.pairing(e, P, Q);
			GT e1 = new GT();
			GT e2 = new GT();
			Fr c = new Fr("1234567890123234928348230428394234");
			System.out.println("c=" + c);
			G2 cQ = new G2(Q);
			Mcl.mul(cQ, Q, c); // cQ = Q * c
			Mcl.pairing(e1, P, cQ);
			Mcl.pow(e2, e, c); // e2 = e^c
			assertBool("e1 == e2", e1.equals(e2));
			{
				byte[] b = e1.serialize();
				GT t = new GT();
				t.deserialize(b);
				assertBool("serialize", e1.equals(t));
			}
			G1 cP = new G1(P);
			Mcl.mul(cP, P, c); // cP = P * c
			Mcl.pairing(e1, cP, Q);
			assertBool("e1 == e2", e1.equals(e2));
			Mcl.inv(e1, e1);
			Mcl.mul(e1, e1, e2);
			e2.setStr("1 0 0 0 0 0 0 0 0 0 0 0");
			assertBool("e1 == 1", e1.equals(e2));
			assertBool("e1 == 1", e1.isOne());

			BLSsignature(Q);
			if (errN == 0) {
				System.out.println("all test passed");
			} else {
				System.out.println("ERR=" + errN);
			}
		} catch (RuntimeException e) {
			System.out.println("unknown exception :" + e);
		}
	}
	public static void BLSsignature(G2 Q)
	{
		Fr s = new Fr();
		s.setByCSPRNG(); // secret key
		System.out.println("secret key " + s);
		G2 pub = new G2();
		Mcl.mul(pub, Q, s); // public key = sQ

		byte[] m = "signature test".getBytes();
		G1 H = new G1();
		Mcl.hashAndMapToG1(H, m); // H = Hash(m)
		G1 sign = new G1();
		Mcl.mul(sign, H, s); // signature of m = s H

		GT e1 = new GT();
		GT e2 = new GT();
		Mcl.pairing(e1, H, pub); // e1 = e(H, s Q)
		Mcl.pairing(e2, sign, Q); // e2 = e(s H, Q);
		assertBool("verify signature", e1.equals(e2));
	}
	public static void testG1Curve(int curveType, String name) {
		try {
			System.out.println("curve=" + name);
			Mcl.SystemInit(curveType);
			Fp x = new Fp(123);
			G1 P = new G1();
			P.tryAndIncMapTo(x);
			P.normalize();
			System.out.println("P.x=" + P.getX());
			System.out.println("P.y=" + P.getY());
		} catch (RuntimeException e) {
			System.out.println("unknown exception :" + e);
		}
	}
	public static void main(String argv[]) {
		testCurve(Mcl.BN254, "BN254");
		testCurve(Mcl.BLS12_381, "BLS12_381");
		testG1Curve(Mcl.SECP256K1, "SECP256K1");
	}
}
