import java.io.*;
import com.herumi.bls.*;

/*
	BlsTest
*/
public class BlsTest {
	static {
		String lib = "blsjava";
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
	public static String byteToHexStr(byte[] buf) {
		StringBuilder sb = new StringBuilder();
		for (byte b : buf) {
			sb.append(String.format("%02x", b));
		}
		return sb.toString();
	}
	public static byte[] hexStrToByte(String hex) {
		int n = hex.length();
		if ((n % 2) != 0) throw new IllegalArgumentException("hexStrToByte odd length");
		n /= 2;
		byte[] buf = new byte[n];
		for (int i = 0; i < n; i++) {
			int H = Character.digit(hex.charAt(i * 2 + 0), 16);
			int L = Character.digit(hex.charAt(i * 2 + 1), 16);
			buf[i] = (byte)(H * 16 + L);
		}
		return buf;
	}
	public static void printHex(String msg, byte[] buf) {
		System.out.print(msg + " " + byteToHexStr(buf));
	}
	public static void testSecretKey() {
		SecretKey x = new SecretKey(255);
		SecretKey y = new SecretKey();
		assertEquals("x.dec", x.toString(), "255");
		assertEquals("x.hex", x.toString(16), "ff");
		assertBool("x.!isZero", !x.isZero());
		x.clear();
		assertBool("x.isZero", x.isZero());
		x.setByCSPRNG();
		System.out.println("x.setByCSPRNG()=" + x.toString(16));
		byte[] b = x.serialize();
		{
			y.deserialize(b);
			assertBool("x.serialize", x.equals(y));
		}
		x.setInt(5);
		y.setInt(10);
		x.add(y);
		assertEquals("x.add", x.toString(), "15");
		x.setInt(13);
		y.setInt(7);
		x.sub(y);
		assertEquals("x.sub", x.toString(), "6");
		x.setInt(-9);
		x.neg();
		y.setInt(7);
		x.add(y);
		assertEquals("x.neg", x.toString(), "16");
		x.setInt(9);
		y.setInt(7);
		x.mul(y);
		assertEquals("x.mul", x.toString(), "63");
		x.setHashOf(new byte[]{1, 2, 3});
		System.out.println("hashOf=" + x.toString(16));
	}
	public static void testPublicKey() {
		PublicKey x = new PublicKey();
		PublicKey y = new PublicKey();
	}
	public static void testSign() {
		SecretKey sec = new SecretKey();
		sec.setByCSPRNG();
		PublicKey pub = sec.getPublicKey();
		byte[] m = new byte[]{1, 2, 3, 4, 5};
		byte[] m2 = new byte[]{1, 2, 3, 4, 5, 6};
		Signature sig = sec.sign(m);
		printHex("sec", sec.serialize());
		printHex("pub", pub.serialize());
		printHex("sig", sig.serialize());
		assertBool("verify", sig.verify(pub, m));
		assertBool("!verify", !sig.verify(pub, m2));
	}
	public static void testShare() {
		int k = 3; // fix
		int n = 5;
		byte[] msg = new byte[]{3, 2, 4, 2, 5, 3, 4};
		SecretKeyVec msk = new SecretKeyVec();
		PublicKeyVec mpk = new PublicKeyVec();

		// setup msk (master secret key) and mpk (master public key)
		for (int i = 0; i < k; i++) {
			SecretKey sec = new SecretKey();
			sec.setByCSPRNG();
			msk.add(sec);
			PublicKey pub = sec.getPublicKey();
			mpk.add(pub);
		}
		// orgSig is signed by secret key
		Signature orgSig = msk.get(0).sign(msg);
		assertBool("verify", orgSig.verify(mpk.get(0), msg));
		// share
		SecretKeyVec ids = new SecretKeyVec();
		SecretKeyVec secVec = new SecretKeyVec();
		PublicKeyVec pubVec = new PublicKeyVec();
		SignatureVec sigVec = new SignatureVec();
		secVec.reserve(n);
		pubVec.reserve(n);
		sigVec.reserve(n);
		for (int i = 0; i < n; i++) {
			SecretKey id = new SecretKey();
			id.setByCSPRNG();
			ids.add(id);
			SecretKey sec = Bls.share(msk, ids.get(i));
			secVec.add(sec);
			PublicKey pub = Bls.share(mpk, ids.get(i));
			pubVec.add(pub);
			Signature sig = sec.sign(msg);
			sigVec.add(sig);
		}
		// recover
		SecretKeyVec idVec2 = new SecretKeyVec(k, new SecretKey());
		PublicKeyVec pubVec2 = new PublicKeyVec(k, new PublicKey());
		SignatureVec sigVec2 = new SignatureVec(k, new Signature());
		for (int i0 = 0; i0 < n; i0++) {
			for (int i1 = i0 + 1; i1 < n; i1++) {
				for (int i2 = i1 + 1; i2 < n; i2++) {
					idVec2.set(0, ids.get(i0));
					idVec2.set(1, ids.get(i1));
					idVec2.set(2, ids.get(i2));
					pubVec2.set(0, pubVec.get(i0));
					pubVec2.set(1, pubVec.get(i1));
					pubVec2.set(2, pubVec.get(i2));
					sigVec2.set(0, sigVec.get(i0));
					sigVec2.set(1, sigVec.get(i1));
					sigVec2.set(2, sigVec.get(i2));
					PublicKey pub = Bls.recover(pubVec2, idVec2);
					Signature sig = Bls.recover(sigVec2, idVec2);
					assertBool("recover pub", pub.equals(mpk.get(0)));
					assertBool("recover sig", sig.equals(orgSig));
				}
			}
		}
	}
	public static void testAggregateSignature() {
		int n = 10;
		PublicKey aggPub = new PublicKey();
		PublicKeyVec pubVec = new PublicKeyVec();
		SignatureVec sigVec = new SignatureVec();
		byte[] msg = new byte[]{1, 2, 3, 5, 9};
		aggPub.clear();
		for (int i = 0; i < n; i++) {
			SecretKey sec = new SecretKey();
			sec.setByCSPRNG();
			PublicKey pub = sec.getPublicKey();
			Signature sig = sec.sign(msg);
			aggPub.add(pub);
			pubVec.add(pub);
			sigVec.add(sig);
		}
		Signature aggSig = Bls.aggregate(sigVec);
		assertBool("aggSig.verify", aggSig.verify(aggPub, msg));
		assertBool("fastAggregateVerify", aggSig.fastAggregateVerify(pubVec, msg));
	}
	public static void addVec(ByteArrayOutputStream os, PublicKeyVec pubVec, SignatureVec sigVec, int n, boolean isDiff) {
		for (int i = 0; i < n; i++) {
			byte[] msg = new byte[Bls.MSG_SIZE];
			if (isDiff) {
				msg[0] = (byte)i;
				msg[1] = (byte)(i + 2);
			} else {
				msg[0] = (byte)(i % 4);
			}
			try {
				os.write(msg);
			} catch (IOException e) {
				assertBool("os.write", false);
				return;
			}
			SecretKey sec = new SecretKey();
			sec.setByCSPRNG();
			pubVec.add(sec.getPublicKey());
			sigVec.add(sec.sign(msg));
		}
	}
	public static void testHex() {
		System.out.println("testHex");
		byte[] b = new byte[]{1, 2, 3, 4, 0x12, (byte)0xff };
		String s = byteToHexStr(b);
		assertEquals("byteToHexStr", s, "0102030412ff");
		byte[] b2 = hexStrToByte(s);
		assertBool("hexStrToByte", java.util.Arrays.equals(b2, b));
	}
	public static void testAggregateVerify() {
		System.out.println("testAggregateVerify");
		final int n = 10;
		for (int i = 0; i < 2; i++) {
			ByteArrayOutputStream os = new ByteArrayOutputStream();
			boolean isDiff = i == 0;
			PublicKeyVec pubVec = new PublicKeyVec();
			SignatureVec sigVec = new SignatureVec();
			addVec(os, pubVec, sigVec, n, isDiff);
			byte[] msgVec = os.toByteArray();
			Signature aggSig = Bls.aggregate(sigVec);
			assertBool("aggregateVerifyNoCheck", aggSig.aggregateVerifyNoCheck(pubVec, msgVec));
			if (isDiff) {
				assertBool("aggregateVerify", aggSig.aggregateVerify(pubVec, msgVec));
			} else {
				assertBool("!aggregateVerify", !aggSig.aggregateVerify(pubVec, msgVec));
			}
		}
	}
	public static void testHarmonyONE() {
		assertBool("requirements", Bls.isDefinedBLS_ETH());
		System.out.println("testHarmonyONE");
		// setting for Harmony.ONE (make library with BLS_ETH=1)
		Bls.init(Bls.BLS12_381);
		Bls.setETHserialization(false);
		Bls.setMapToMode(Bls.MAP_TO_MODE_ORIGINAL);
		PublicKey gen = new PublicKey();
		// the old generator
		gen.setStr("1 4f58f3d9ee829f9a853f80b0e32c2981be883a537f0c21ad4af17be22e6e9959915ec21b7f9d8cc4c7315f31f3600e5 1212110eb10dbc575bccc44dcd77400f38282c4728b5efac69c0b4c9011bd27b8ed608acd81f027039216a291ac636a8");
		Bls.setGeneratorOfPublicKey(gen);

		SecretKey sec = new SecretKey();
		byte[] bytes = new byte[] {
			-71, 10, 0, -19, -60, -25, -21, -3, 95, -35, 36, -32, 109, -4, 10, -34, 14, -59, 82, 107, -36, 29, -48, 123, -87, -66, 98, -75, -16, -58, 35, 98
		};
		sec.deserialize(bytes);
		Signature sig = new Signature();
		sec.signHash(sig, bytes);
		assertBool("check sig", byteToHexStr(sig.serialize()).equals("2f4ff940216b2f13d75a231b988cd16ef22b45a4709df3461d9baeebfeaafeb54fad86ea7465212f35ceb0af6fe86b1828cf6de9099cefe233d97e0523ba6c0f5eecf4db71f7b1ae08cd098547946abbd0329fdac14d27102f2a1891e9188a19"));

		sec.deserialize(hexStrToByte("d243f3f029c188a5b1c4c098f5719cbc967184ef962b5c5d6c72693c92c1f725"));
		PublicKey pub = sec.getPublicKey();
		assertBool("check pub", byteToHexStr(pub.serialize()).equals("15ad529698be1f6164fd50416d1991a04b977d9014b43ae9d014ca50ae634829182632d54c7188b3a53e0b77ae4c9e87"));
		String[] dataHex = new String[]{
"1100000000000000000000000000000000000000000000000000000000000031",
"2f2071370054ded61081a069dc657d7111ae909fc08a38ae1f3a967c4e4c1b97b7d2ee8adc512ded7213dc0ab2c9db158451eca54a4db26ec013e108d561d8e0eede883b73074d49143ea8e9291903aabce2a2b4f153f99822e4512dac86ae18",
"807f6fb074cfdd0318501ef894b127a3f71754745dcff2163dd128091efd065c4ac419e3d7d5428a0b94ada19de657176fb4ccfdb3105f869d6351e503ba20e9d6ef55f179510941db4d131c3678e9d9316090c99bd7d62107d83f6fe5f6f115",
"2100000000000000000000000000000000000000000000000000000000000041",
"d9039da9fc699ba0d4ec0d339e85cfb503bb180b021aa81f038afc5db5f108df0bf265142d0dfd4731ef441731753e0eace7acd190582a27117b140d4928d2dc47bb6603fcc1fcd1e9bdb92f72621ed21f09a9896d67d8493e614cefc8a6b418",
"8b22e141cf2886a64f665a7b6e2bb17028a297eaeba7bb4523e2fa176f015e68bab4a58941a19990cf0ae7412073f219825d80b7b48b10b992eaf8eba48e1d1c81b5a00d3a5c9123e464b6ec84b8b1b182ff710786ec449f12bd4e031858a486",
"3100000000000000000000000000000000000000000000000000000000000051",
"5d862a1bc0283f747d5e883def841fdcaeef9f115bae69037eebfa356c7b2aa68664412ff8f18cf7c3099a71422bae0edfb53f77cf8d7484fd5aeda3d88ae7c7224151710aa137f9ad3e83aae8d02004178effe6a026bc987e513891c6d09419",
"5af837d3ebc317c449910fc614cda04a10e7e45c4ff551ee006af188a4d21f6fb9526f025fe529112ea1d2abaff3dc0207cfd3fa36980dcdf99c5279f37746bb27c4bce7c3ed446cae763764be794a45b3dab053395143bfb5da3d19158fdf90",
"4100000000000000000000000000000000000000000000000000000000000061",
"6a4940b62aa2c931251cb428658ee7e857dd608258131a743759e366a7945521accc1dec3dff8b2b782338d33446b015fb8e2883234d983e918c19c1efc21c81789c3219cf6324390c472d1070a2f0e764e20adb3e11172a8d7380cbef363102",
"ac1c3e7bda94bfbed0db51d765aa3f933fe7fc8194f8f61f8ffe56505d1b93158d2f03b25297f88cd84f4b632334a8124d881235766f7ca2b06cba349c75abb9e194d1823d7453cb47d2714881ef7b22993affaf9cb1821eeaf13c331793cc93",
"5100000000000000000000000000000000000000000000000000000000000071",
"5e01b80bd8bb9be237114fa59e18c8f6d833628aa706279cf1e5c52b8ba3404087c488e6b9b0f754531ee273aedb9f0eb9f2eb83168b4369e9f88b02d60cf06ad4031333d5dc768bbe735716930a1b098d318e2d34828b1d08c6bb70ba1bb708",
"d8204ea2171172adfd8db306b4120e9c1dc635c6fa1660d2c30fb8de4173769edc18a6382e80503232ce2e2aec45e418b4446a25344561675a0ff609b107b995d3cec76ca7f51c9a9ea47a57123cad265b83a1f728bf56af80661c78beddcf16",
"6100000000000000000000000000000000000000000000000000000000000081",
"01516454923f8e318851934a3be19d5e642cf49d3628f97ee924e426f3658e1471aee5361d2ebef876d47d65917bbd153db4266f41058efc21b87e1f391df28b44425222cf09309e1adcbbbc154eea7c4fa6cb1c2bfd6f1e29df02d8ecbfd78a",
"6a576f41860bb2eb49d29b5aa76a2e0277ef2c55cccbf7cea1a6e40b3d558de5ef89824930eeaac4d906b6d37f9fa918efe009844b20dfe8626966ba3eb45f56e7485e3dce65495616075610d468df3a4c7514b39b6025d38b6119184053008f",
"7100000000000000000000000000000000000000000000000000000000000091",
"dc51fd434b1738fa74621a773849d92ecd2e137cb5b8bba292a850ff347e23815366a7696269bad26cc35a9cb885a30767804c6e160b627397fc0b05b76abfac85edb16b901061a68b704287345d49e4470328e5084b439481fe466971c8a191",
"267f20683979c7ad658cccdff45cc747748584c4a6d27b21a4f175b0c50b2e844b4d9ad98a3b7a489ca529f42e46950fa7bb2f233ba03f839ba5d3ff4aeabe8d233faf6d9d9879a94b6adf4dff89d53191635ded9867fe64c0b725af51122a8f",
"81000000000000000000000000000000000000000000000000000000000000a1",
"808eba98e02654924a8ef241daa0e509e2967dd8107244684d43ea3c15f1a365d0c8a303d8e7947036fd6784fc001b0a8ea8c0d06855ef3005d9f7677b509c9f9630714daee2b9012cabdc212cb4c8ed7aed23162987f2d419bdbdfc06daf483",
"89af6dcb5767748c00be13feebc7fc4c19e3e37a5f3fd2ce0acdb5a4be5556a8c558522d3b154ccac9c22cb935d78e132d39268bda15d4e4b59b3ed3563e5cb14531247595de5a7643710501be8e9dffb7f87ddbc819706a4e1c6a302328d787",
"91000000000000000000000000000000000000000000000000000000000000b1",
"9ab282ebe3c5226c3c78650a865b47390730e35042898caeddd45aba7d9068298c221fac1fafd6a81ebecd042683121369fb5cbb996d66159c27f309f7e7c299a37f2b36925096fda1183c97b610b1ca9ed8e3a473b3a32c683dd8fc1be89815",
"ca590d2a56b606e4defa8d400313ff57aba33b5330bbc402137b2090abfb1f877550d64a5441aeb75030cabc4c56db14f0ae475b127ac9796ebcf0a762622d8c3a9a7a4aaa2fc7249dae4f99ca08b2a4681bd24f9d8083d7c6ba18ab9f5a7589",
"a1000000000000000000000000000000000000000000000000000000000000c1",
"1e5f1b5fc478ef245a8e0306c752e282a94cf698be44de4823f44a85ae5c252a68f98e3e07e2dec7f0a553543e4d8b0d56e1b5cd863ff9aa9cbde2b19c25b5f86289bdcab4c83ce3be4cc6280015afff6e28517978138a794579becba2dde697",
"91078b1e5f57bac1957026e67525e3cca709a6cc051ecb990f034457b747fc0abb396e8b011488dc4d43a3d408b173001d35b20f620da6bb9cea8e958e7f1a8fc862f8c828e49d4cfef652a083d51f7d48af679ecd2bcc4acb900f8dd9928694",
"b1000000000000000000000000000000000000000000000000000000000000d1",
"f2dd65c95f55260f1b875fb739c06a7ad0b56e326274dcf2e336407689109f4761b926b9111ae3fbcb8130dcbb331012d988ea44224dcb5dad083d8757f91a48f8a55ddafcaabf145426c759b8ba0bd13790864c6bd501b1ecbe2cff4a622a0b",
"5bf7e9a1a5fc1796e736d4ab2eb6ac7e1f01651a3843a2d5139e3ce0288af1b94d2a33d9e9a4fb9e5e5641abc6a7f00ed235130ef116c172e049fe2933cfea5a0f6c90bd572b1f6e528c8d7e052052e8d591f2a4642c27bf34babf785f79f196",
"c1000000000000000000000000000000000000000000000000000000000000e1",
"53a598c29f8375aa719e0d8450f278ffbebd4e244f443571fa87480c8befbc552f5c940108a54c4fb5ddb07700573517426d6eae951331acadac389d49621b3f94988cce8f5f43a28bc9400bab5d679b1534fd75679423ab4c42d1d2a5dda40e",
"77753387f0dfe22734ec8120b627d1ad247274e0ae8a3a71fcfc94ca0af54a1cf518f767b4d888941f2b8dc1ba063c0f611b374f338784238a7524704c9a28a71828d38562e069951383aef19b8dcc996e4b45f8ffb1f15deb109226d0630404",
"d1000000000000000000000000000000000000000000000000000000000000f1",
"ad7f6fab8f77997d25d4071ea06fce8e2fb84ab812bbbcf1bd05534af163db4fec71d9cd8bf5226927fee5d3efbbe207e64a5373a95277876ca6c59a5d8a938c281bd5ced57cd1c14cd89bfffc367423e55fbeb9c853540235ac68868a85e015",
"2692e76e900af740d7ac08fcd5b127e254ff9e3a41e228a5eb2ac19062557ebcb3721752a38c0473e8e7d0a5ea3e0c0f205a94e2d1db9488c70565453dc3ed04c7efd5b9af5fbf3cd13247ca28d64ad116c949d6173abac3f410061259378b0c",
"e100000000000000000000000000000000000000000000000000000000000001",
"4787ab4016f3ff0802c998963bc480124291fc1632b83128805381cd644b57b588eb09b176a1d3f93fdd15889e1b8105fea5f78912e158cdd0660194771afbc095d25a75791c52eef21d1286313d37cade4e3b44456040b7b967d6770b7a5712",
"caad624f8b9f955dbb8d03e3bbfe7b64c12fd6b6c55de61dba564a0b52dedc6f2a913e5d4fefab2487d0d51370f4591841ebb420d31f3738973b6fe824e9b3ba3f34e5e4417d21ff10f0758b8008ac202a64c7f527ec7ebde1007b6699880a19",
"f100000000000000000000000000000000000000000000000000000000000011",
"d447003c4051ba8ef7b2ce7f9febc7026594e8daca96a951f8ecebd1f7d18a4f5212997227ecba7655ad06aee32483040b0e7d6bf665ade589a35a27fa8f3dadee7a9c09ef8c5fd8c5a6fac5ca231397aa83578f3d29fc6a03b0418eb53a9d8f",
"2157bbc5f016989e2cd7f11abdd5d0060e00922c54dffb429d7770ddf4da05770c51db3b9db61ae726704438f2778c0e0b7f9617d0df77433817c31909d1c0dd69d012c67ba6370dd06762304dc99ac849304bc3101840f8c921ccbeae8fb389",
"0100000000000000000000000000000000000000000000000000000000000021",
"7deb35923ebc93587dd332ce02b2df77ab46bf8833e7b865f80a2fd883d78e4fe9ff4bd548f471de958ae8de25336e0ee6b72d41392e99fd10b0b24523e0ef326df9df801bae5f52725af5b0a174522ee4d303d9224249e0f0089e6bf80acd94",
"9d7f306dcbc3740db5c842d7d077467fb7d7947228ba6711ad45ba79c71394d1ca4f6dcef78f1032f1af62e9b6bf8d0edee31d64562a1af3cb072b2009628f74a9fe6e477f530881ba90c167704ca4b880bc975469438ddb17345e0abf70ce97",
		};
		for (int i = 0; i < dataHex.length; i += 3) {
			byte[] msg = hexStrToByte(dataHex[i]);
			sig = sec.sign(msg);
			assertBool("sec.sign", byteToHexStr(sig.serialize()).equals(dataHex[i + 1]));
			assertBool("sig.verify", sig.verify(pub, msg));
			sig = sec.signHash(msg);
			assertBool("sec.signHash", byteToHexStr(sig.serialize()).equals(dataHex[i + 2]));
			assertBool("sig.verifyHash", sig.verifyHash(pub, msg));
		}
	}
	public static void testCurve(int curveType, String name) {
		try {
			System.out.println("curve=" + name);
			Bls.init(curveType);
			testHex();
			testSecretKey();
			testPublicKey();
			testSign();
			testShare();
			testAggregateSignature();
			if (Bls.isDefinedBLS_ETH() && curveType == Bls.BLS12_381) {
				System.out.println("BLS ETH mode");
				testAggregateVerify();
				testHarmonyONE();
			}
			if (errN == 0) {
				System.out.println("all test passed");
			} else {
				System.out.println("ERR=" + errN);
			}
		} catch (RuntimeException e) {
			System.out.println("unknown exception :" + e);
		}
	}
	public static void main(String argv[]) {
		testCurve(Bls.BN254, "BN254");
		testCurve(Bls.BLS12_381, "BLS12_381");
	}
}
