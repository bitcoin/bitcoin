#define PUT(x) std::cout << #x "=" << (x) << std::endl;
#include <cybozu/test.hpp>
#include <cybozu/sha2.hpp>
#include <mcl/bls12_381.hpp>
#include <iostream>
#include <fstream>
#include <cybozu/atoi.hpp>
#include <cybozu/file.hpp>
#include <cybozu/benchmark.hpp>

using namespace mcl;
using namespace mcl::bn;

typedef mcl::MapTo_WB19<Fp, G1, Fp2, G2> MapTo;
typedef MapTo::E2 E2;

void dump(const void *msg, size_t msgSize)
{
	const uint8_t *p = (const uint8_t *)msg;
	for (size_t i = 0; i < msgSize; i++) {
		printf("%02x", p[i]);
	}
	printf("\n");
}

void dump(const std::string& s)
{
	dump(s.c_str(), s.size());
}

std::string toHexStr(const void *_buf, size_t n)
{
	const uint8_t *buf = (const uint8_t*)_buf;
	std::string out;
	out.resize(n * 2);
	for (size_t i = 0; i < n; i++) {
		cybozu::itohex(&out[i * 2], 2, buf[i], false);
	}
	return out;
}

std::string toHexStr(const std::string& s)
{
	return toHexStr(s.c_str(), s.size());
}

typedef std::vector<uint8_t> Uint8Vec;

Uint8Vec fromHexStr(const std::string& s)
{
	Uint8Vec ret(s.size() / 2);
	for (size_t i = 0; i < s.size(); i += 2) {
		ret[i / 2] = cybozu::hextoi(&s[i], 2);
	}
	return ret;
}

struct Fp2Str {
	const char *a;
	const char *b;
};

struct PointStr {
	Fp2Str x;
	Fp2Str y;
	Fp2Str z;
};

void set(Fp2& x, const Fp2Str& s)
{
	x.a.setStr(s.a);
	x.b.setStr(s.b);
}

template<class E2>
void set(E2& P, const PointStr& s)
{
	set(P.x, s.x);
	set(P.y, s.y);
	set(P.z, s.z);
}

std::string toHexStr(const Fp2& x)
{
	uint8_t buf1[96];
	uint8_t buf2[96];
	size_t n1 = x.a.serialize(buf1, sizeof(buf1));
	size_t n2 = x.b.serialize(buf2, sizeof(buf2));
	return toHexStr(buf1, n1) + " " + toHexStr(buf2, n2);
}

std::string toHexStr(const G2& P)
{
	uint8_t xy[96];
	size_t n = P.serialize(xy, 96);
	CYBOZU_TEST_EQUAL(n, 96);
	return toHexStr(xy, 96);
}

/*
	in : Proj [X:Y:Z]
	out : Jacobi [A:B:C]
	[X:Y:Z] as Proj
	= (X/Z, Y/Z) as Affine
	= [X/Z:Y/Z:1] as Jacobi
	= [XZ:YZ^2:Z] as Jacobi
*/
void toJacobi(G2& out, const G2& in)
{
	Fp2 z2;
	Fp2::sqr(z2, in.z);
	Fp2::mul(out.x, in.x, in.z);
	Fp2::mul(out.y, in.y, z2);
	out.z = in.z;
}

void testHMAC()
{
	const char *key = "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b";
	const char *msg = "Hi There";
	uint8_t hmac[32];
	const char *expect = "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7";
	cybozu::hmac256(hmac, key, strlen(key), msg, strlen(msg));
	std::string out = toHexStr(hmac, 32);
	CYBOZU_TEST_EQUAL(out, expect);
}

void addTest()
{
	const struct Tbl {
		PointStr P;
		PointStr Q;
		PointStr R;
	} tbl[] = {
		{
			{
				{
					"0x111fe4d895d4a8eb21b87f8717727a638cb3f79b91217ac2b47ea599513a5e9bff14cd85f91e5bef822160e0ad4f6726",
					"0x29180cfc2d6a6c717ad4b93725475117c959496d3163974cc08068c0319cb47ba7c8d49c0ebb1ed1a4659b91acab3f",
				},
				{
					"0x192e14063ab46786058c355387e4141921a2b0fd1bcecd6bbf6e3e25f972b2b88fe23b1fd6b14f8070c7ada0bbcfb8d7",
					"0x153bc38ad032b044e55f649b9b1e6384cfe0936b3be350e16a8cf847790bf718e9099b102fbdab5ad8f0acca6b0ac65a",
				},
				{
					"0x119f8d49f20b7a3ef00527779ef9326250a835a742770e9599b3be1939d5e00f8b329781bea38e725e1b0de76354b2ea",
					"0xd95d36844c2ef0678e3614c0d9698daf7d54cb41322fb6acf90a4fd61122c36213e6f811c81c573385110d98e49136",
				},
			},
			{
				{
					"0x738abc340e315a70a95d22c68e4beb8f8ce8cb17ec4d8104285b5770a63b2e9fdceaffb88df1fde2104d807bd0fb5df",
					"0x19edac9569a018b7a17ddd9554430318500e83e38c798d6f8e0a22e9e54ef2b0ec0cf4866013e3a43237eaf949c4548b",
				},
				{
					"0x12234a4947cf5c0a0fc04edadefa7c3766489d927ad3d7d7236af997b0e0fd7deaaf4ab78aad390c6a8f0088f21256af",
					"0x4a1cddb800e9fc6fb9f12e036bd0dae9a75c276f8007407cb9be46177e4338ac43d00f3dc413cab629d6305327ffbc",
				},
				{
					"0x187212ac7f7d68aa32dafe6c1c52dc0411ea11cffa4c6a10e0ba407c94b8663376f1642379451a09a4c7ce6e691a557f",
					"0x1381999b5cc68ae42d64d71ac99a20fb5874f3883a222a9e15c8211610481642b32b85da288872269480383b62696e5a",
				},
			},
			{
				{
					"0x1027d652690099dd3bea0c8ec2f8686c8db37444b08067a40780a264f2edd995d3a39941a302289ac8025007e7f08e35",
					"0xe4c1e12005a577f2a7487bd0bca91253bfff829258e7120716d70133dfc1c8f4aa80d2b4c076f267f3483ec1ca66cdc",
				},
				{
					"0x16bd53f43f8acfb29d3a451a274445ca87d43f0e1a6550c6107654516fda0b4cd1a346369ef0d44d4ee78904ce1b3e4b",
					"0xf0f67bbce56d7791c676b7af20f0d91382973c6c7b971a920525dbd58b13364ec226651308c8bc56e636d0458d46f50",
				},
				{
					"0x8027cefbfd3e7e7fdc88735eddd7e669520197227bd2a7014078f56489267256fdfb27d080515412d69f86770f3ce",
					"0x2470e1d8896cfe74ab01b68071b97d121333ebcec7a41cddd4581d736a25ba154ac94321a119906e3f41beec971d082",
				},
			},
		},
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		E2 P, Q, R;
		set(P, tbl[i].P);
		set(Q, tbl[i].Q);
		set(R, tbl[i].R);
		E2 E;
		ec::addJacobi(E, P, Q);
		CYBOZU_TEST_ASSERT(R.isEqual(E));
	}
}

template<class T>
void iso3Test(const T& mapto)
{
	const PointStr Ps = {
		{
			"0xf0d9554fa5b04dbc6b106727e987bd68fb8c0cc97226a3845b59cc9d09972f24ea5a0d93cd0eedd18318c0024bf3df0",
			"0x656650d143a2cf4a913821fa6a90ab6baa0bb063d1207b15108ea919258bfa4bdd1ba7247e8e65300d526801e43dca6",
		},
		{
			"0x13a4b7c833b2702dc6ac4f5ee6ee74923a24c28e5a9b8e3b5626f700489ea47f9b1c3aa8cc0f4b525ae56e1e89aba868",
			"0x16c0b9a89dcbe4e375f1e4d064013adff8e6e09866d38769c08ce355fbac9c823d52df971286b091b46d2cd49625c09",
		},
		{
			"0x176ce067d52f676d4f6778eda26f2e2e75f9f39712583e60e2b3f345e2b2a84df1ae9ffa241ce89b1a377e4286c85ccf",
			"0x822bc033cf0eec8bea9037ede74db0a73d932dc9b43f855e1862b747b0e53312dde5ed301e32551a11a5ef2dfe2dbf4",
		}
	};
	const PointStr Qs = {
		{
			"0x8d5483693b4cf3fd5c7a62dad4179503094a66a52f2498dcedb5c97a33697ba4110e2da42ddef98beeeab04619ec0fe",
			"0xd45728bb18737fb6abf8cc94ad37957f95855da867ca718708503fd072d3707ca6059fefb5c52b2745210cdd7991d10",
		},
		{
			"0x17027ae16e10908f87e79c70f96ba44b1b11fa40fb5ac5456162133860f14896ca363b58d81ef8cb068bdaca2e576ed7",
			"0xfb2d1655b00027d5580bbff8afa6eec6e6caacf5df4020c5255eafb51d50710193a8e39eac760745c45cc6ec556a820",
		},
		{
			"0x376b86a7d664dc080485c29a57618eee792396f154806f75c78599ee223103e77bee223037bb99354114201619ea06",
			"0xf0c64e52dbb8e2dca3c790993c8f101012c516b2884db16de4d857ae6bfb85e9101ab15906870b3e5a18268a57bfc99",
		}
	};
	const PointStr clearPs = {
		{
			"0x6f3d4cbd80011d9cbf0f0772502d1e6571d00bc24efc892659339fc8ae049e757c57d22368c33cfc6c64bc2df59b3da",
			"0x71e02679953af97ed57d9301d126c3243de7faa3bbebd40b46af880ba3ba608b8c09c0a876401545ce6f901950f192",
		},
		{
			"0x174d1e92bd85b0cf1dd2808bd96a25ed48ba1e8d15c1af5557f62719e9f425bd8df58c900cf036e57bce1b1c78efb859",
			"0x1cfc358b91d57bf6aa9fa6c688b0ef516fdac0c9bfd9ef310ea11e44aaf778cca99430594a8f5eb37d31c1b1f72c2f6",
		},
		{
			"0x17614e52aacf8804ed2e7509db5b72395e586e2edc92dba02da24e6f73d059226a6deb6e396bd39567cec952f3849a6c",
			"0xb7b36b9b1bbcf801d21ca5164aa9a0e71df2b4710c67dc0cd275b786800935fc29defbdf9c7e23dc84e26af13ba761d",
		}
	};
	typename T::E2 P;
	G2 Q1, Q2;
	set(P, Ps);
	set(Q1, Qs);
	mapto.iso3(Q2, P);
	CYBOZU_TEST_EQUAL(Q1, Q2);
	set(Q1, clearPs);
	mcl::local::mulByCofactorBLS12fast(Q2, Q2);
	CYBOZU_TEST_EQUAL(Q1, Q2);
}

template<class T>
void testHashToFp2v7(const T& mapto)
{
	{
		const char *msg = "asdf";
		PointStr s = {
			{
				"2525875563870715639912451285996878827057943937903727288399283574780255586622124951113038778168766058972461529282986",
				"3132482115871619853374334004070359337604487429071253737901486558733107203612153024147084489564256619439711974285977",
			},
			{
				"2106640002084734620850657217129389007976098691731730501862206029008913488613958311385644530040820978748080676977912",
				"2882649322619140307052211460282445786973517746532934590265600680988689024512167659295505342688129634612479405019290",
			},
			{
				"1",
				"0",
			}
		};
		G2 P1, P2;
		mapto.msgToG2(P1, msg, strlen(msg));
		set(P2, s);
		CYBOZU_TEST_EQUAL(P1, P2);
	}
	{
		struct Tbl {
			const char *msg;
			const char *dst;
			const char *expect;
			size_t mdSize;
		} tbl[] = {
			{
				/*
					https://github.com:cfrg/draft-irtf-cfrg-hash-to-curve
					tag: draft-irtf-cfrg-hash-to-curve-07
					the return value of expand_message_xmd in hash_to_field.py
				*/
				"asdf",
				"BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_",
				"ca53fcd6f140590d19138f38819eb13330c014a1670e40f0f8e991de7b35e21a1fca52a14486c8e8acc9d865718cd41fe3638c2fb50fdc75b95690dc58f86494005fb37fc330366a7fef5f6e26bb631f4a5462affab2b9a9630c3b1c63621875baf782dd435500fda05ba7a9e86a766eeffe259128dc6e43c1852c58034856c4c4e2158c3414a881c17b727be5400432bf5c0cd02066a3b763e25e3ca32f19ca69a807bbc14c7c8c7988915fb1df523c536f744aa8b9bd0bbcea9800a236355690a4765491cd8969ca2f8cac8b021d97306e6ce6a2126b2868cf57f59f5fc416385bc1c2ae396c62608adc6b9174bbdb981a4601c3bd81bbe086e385d9a909aa",
				256,
			},
			{
				"asdf",
				"QUUX-V01-CS02-with-BLS12381G1_XMD:SHA-256_SSWU_RO_",
				"ecc25edef8f6b277e27a88cf5ca0cdd4c4a49e8ba273d6069a4f0c9db05d37b78e700a875f4bb5972bfce49a867172ec1cb8c5524b1853994bb8af52a8ad2338d2cf688cf788b732372c10013445cd2c16a08a462028ae8ffff3082c8e47e8437dee5a58801e03ee8320980ae7c071ab022473231789d543d56defe9ff53bdba",
				128,
			},
			// Test coming from https://tools.ietf.org/html/draft-irtf-cfrg-hash-to-curve-09#appendix-I.1
			{
				"",
				"QUUX-V01-CS02-with-expander",
				"f659819a6473c1835b25ea59e3d38914c98b374f0970b7e4c92181df928fca88",
				32,
			},
			{
				"abc",
				"QUUX-V01-CS02-with-expander",
				"1c38f7c211ef233367b2420d04798fa4698080a8901021a795a1151775fe4da7",
				32,
			},
			{
				"abcdef0123456789",
				"QUUX-V01-CS02-with-expander",
				"8f7e7b66791f0da0dbb5ec7c22ec637f79758c0a48170bfb7c4611bd304ece89",
				32,
			},
			{
				"q128_qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq",
				"QUUX-V01-CS02-with-expander",
				"72d5aa5ec810370d1f0013c0df2f1d65699494ee2a39f72e1716b1b964e1c642",
				32,
			},
			{
				"",
				"QUUX-V01-CS02-with-expander",
				"8bcffd1a3cae24cf9cd7ab85628fd111bb17e3739d3b53f89580d217aa79526f1708354a76a402d3569d6a9d19ef3de4d0b991e4f54b9f20dcde9b95a66824cbdf6c1a963a1913d43fd7ac443a02fc5d9d8d77e2071b86ab114a9f34150954a7531da568a1ea8c760861c0cde2005afc2c114042ee7b5848f5303f0611cf297f",
				128,
			},
			{
				"abc",
				"QUUX-V01-CS02-with-expander",
				"fe994ec51bdaa821598047b3121c149b364b178606d5e72bfbb713933acc29c186f316baecf7ea22212f2496ef3f785a27e84a40d8b299cec56032763eceeff4c61bd1fe65ed81decafff4a31d0198619c0aa0c6c51fca15520789925e813dcfd318b542f8799441271f4db9ee3b8092a7a2e8d5b75b73e28fb1ab6b4573c192",
				128,
			},
			{
				"abcdef0123456789",
				"QUUX-V01-CS02-with-expander",
				"c9ec7941811b1e19ce98e21db28d22259354d4d0643e301175e2f474e030d32694e9dd5520dde93f3600d8edad94e5c364903088a7228cc9eff685d7eaac50d5a5a8229d083b51de4ccc3733917f4b9535a819b445814890b7029b5de805bf62b33a4dc7e24acdf2c924e9fe50d55a6b832c8c84c7f82474b34e48c6d43867be",
				128,
			},
			{
				"q128_qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq",
				"QUUX-V01-CS02-with-expander",
				"48e256ddba722053ba462b2b93351fc966026e6d6db493189798181c5f3feea377b5a6f1d8368d7453faef715f9aecb078cd402cbd548c0e179c4ed1e4c7e5b048e0a39d31817b5b24f50db58bb3720fe96ba53db947842120a068816ac05c159bb5266c63658b4f000cbf87b1209a225def8ef1dca917bcda79a1e42acd8069",
				128,
			},
			{
				"a512_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
				"QUUX-V01-CS02-with-expander",
				"396962db47f749ec3b5042ce2452b619607f27fd3939ece2746a7614fb83a1d097f554df3927b084e55de92c7871430d6b95c2a13896d8a33bc48587b1f66d21b128a1a8240d5b0c26dfe795a1a842a0807bb148b77c2ef82ed4b6c9f7fcb732e7f94466c8b51e52bf378fba044a31f5cb44583a892f5969dcd73b3fa128816e",
				128,
			},
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const char *msg = tbl[i].msg;
			const char *dst = tbl[i].dst;
			const char *expect = tbl[i].expect;
			size_t mdSize = tbl[i].mdSize;
			uint8_t md[256];
			size_t msgSize = strlen(msg);
			size_t dstSize = strlen(dst);
			mcl::fp::expand_message_xmd(md, mdSize, msg, msgSize, dst, dstSize);
			CYBOZU_TEST_EQUAL(toHexStr(md, mdSize), expect);
		}
	}
	{
		const uint8_t largeMsg[] = {
			0xda, 0x56, 0x6f, 0xc9, 0xaf, 0x58, 0x60, 0x1b, 0xf6, 0xe0, 0x98, 0x25, 0x1c, 0x14, 0x03, 0x8e,
			0x7f, 0x1b, 0x7d, 0xfe, 0x8f, 0x1c, 0xf4, 0x18, 0xa5, 0x3b, 0x4a, 0x86, 0x35, 0x9f, 0x67, 0xeb,
			0x99, 0x6e, 0x30, 0xea, 0x3e, 0xcb, 0x8b, 0x32, 0x8a, 0xe2, 0x61, 0x5e, 0xde, 0xd5, 0x90, 0x20,
			0xa7, 0xcc, 0x08, 0x14, 0x45, 0xe7, 0xa7, 0xe6, 0x1e, 0x53, 0x57, 0x34, 0x9f, 0x37, 0xfe, 0xb0,
			0x25, 0xb1, 0x80, 0xf7, 0x1d, 0xee, 0xc3, 0xb0, 0xe8, 0x0f, 0xae, 0x85, 0xef, 0x42, 0x2a, 0x19,
			0x98, 0xa4, 0x18, 0x15, 0x45, 0x5d, 0x63, 0x19, 0x64, 0x97, 0xeb, 0xd3, 0x54, 0x75, 0x9d, 0xe0,
			0x7c, 0x20, 0x58, 0xef, 0x43, 0xb7, 0x06, 0x9e, 0x13, 0x69, 0x8b, 0x99, 0x49, 0x57, 0xd1, 0x82,
			0x54, 0xa8, 0xb8, 0xff, 0x93, 0x81, 0x2b, 0xbb, 0x73, 0x05, 0x0d, 0x57, 0x51, 0x63, 0x48, 0x7d,
			0x9c, 0xb8, 0xba, 0xd1, 0xb5, 0x31, 0x51, 0xf5, 0x06, 0xee, 0xf4, 0x96, 0xed, 0x18, 0x82, 0x53,
			0xd8, 0xd4, 0xe0, 0xda, 0x2e, 0x4d, 0xfc, 0xc7, 0x4b, 0xa0, 0xba, 0xca, 0xa0, 0xf8, 0xfa, 0x86,
			0x8a, 0x7e, 0xac, 0x9b, 0x76, 0x52, 0xab, 0xb8, 0xc5, 0x9d, 0xe3, 0x7e, 0xe1, 0x83, 0x3a, 0x92,
			0x2c, 0x30, 0x96, 0x9a, 0x0e, 0xc3, 0xdc, 0x3f, 0xf2, 0x63, 0xf0, 0x27, 0x33, 0x3b, 0xbc, 0xf7,
			0x40, 0x6d, 0x26, 0x54, 0x7e, 0x1e, 0x0f, 0xe6, 0x50, 0x75, 0x61, 0x4d, 0x1b, 0x9a, 0x02, 0x37,
			0x49, 0xb3, 0xd4, 0x49, 0x3e, 0xe4, 0xc4, 0x23, 0x61, 0x53, 0xb4, 0x70, 0x17, 0x25, 0x88, 0xd3,
			0xc4, 0x88, 0x2b, 0xf6, 0xd0, 0x96, 0x7d, 0x7e, 0xa3, 0x7c, 0x68, 0x0c, 0xa3, 0x5d, 0xd1, 0x4a,
			0x31, 0x62, 0xa0, 0xe2, 0xb6, 0xb0, 0xb9, 0x70, 0x9b, 0x70, 0x00, 0xa5, 0x42, 0xbe, 0x5f, 0x1c,
			0x12, 0xcd, 0xb9, 0x84, 0x6f, 0xb6, 0xf5, 0x80, 0xc4, 0xa9, 0xfa, 0xb6, 0x75, 0xca, 0xad, 0xc9,
			0xe3, 0x3d, 0xf6, 0x63, 0x7a, 0x2a, 0xb7, 0x2a, 0xa0, 0xb4, 0xd9, 0xc2, 0xb9, 0x01, 0x3f, 0xd3,
			0x2a, 0x3a, 0xd4, 0xfe, 0x58, 0x84, 0x7a, 0xee, 0xb0, 0x07, 0x18, 0x4a, 0x90, 0xe4, 0x96, 0xb3,
			0x63, 0x43, 0xd5, 0xd4, 0x89, 0x4a, 0xc1, 0x4d, 0x6f, 0x25, 0x3d, 0xcc, 0x7b, 0xef, 0x2d, 0xef,
			0x0a, 0xd7, 0x7a, 0x60, 0x8e, 0xfe, 0x0c, 0xc8, 0x66, 0x8c, 0xc3, 0xc7, 0xf7, 0xa9, 0x8a, 0x05,
			0xab, 0x74, 0x43, 0x2d, 0xe6, 0x1a, 0xd6, 0xdb, 0x0c, 0xc0, 0x2a, 0xc0, 0x88, 0x88, 0x24, 0x7a,
			0xbc, 0x9f, 0xae, 0xaf, 0x0c, 0x23, 0xa6, 0x0b, 0xe6, 0x3e, 0xf7, 0x32, 0xa8, 0x17, 0x87, 0xc4,
			0xbc, 0xd1, 0x38, 0x6f, 0x8a, 0x94, 0xf7, 0xd4, 0x73, 0x85, 0xa5, 0x9e, 0xde, 0xce, 0x28, 0x6c,
			0x32, 0x8e, 0x68, 0xeb, 0xda, 0xf1, 0x49, 0xbb, 0x34, 0x18, 0xb3, 0x84, 0xa8, 0x2e
		};
		const uint8_t largeDst[] = {
			0xf0, 0x9d, 0x58, 0xbb, 0x9f, 0x78, 0xb7, 0x22, 0x3d, 0xa2, 0x77, 0xa8, 0x67, 0x83, 0xbb, 0x37,
			0x52, 0x35, 0x1d, 0x08, 0x89, 0x43, 0x1d, 0x93, 0xf4, 0x00, 0xb3, 0x5c, 0x08, 0xdd, 0x7e, 0x68,
			0x53, 0x90, 0x70, 0x9d, 0x79, 0x80, 0xb2, 0xe2, 0x21, 0x17, 0x6b, 0x20, 0xf0, 0xb2, 0xba, 0x39,
			0x8c, 0xc1, 0xce, 0x12, 0x9c, 0xa0, 0xc6, 0x96, 0xc9, 0xc0, 0xce, 0xa4, 0xb2, 0x7b, 0x07, 0x29,
			0x01, 0x41, 0x97, 0x09, 0x60, 0x76, 0xa9, 0x29, 0x6c, 0x7b, 0x5c, 0x6d, 0xd0, 0xb8, 0xdb, 0xc2,
			0x32, 0x99, 0x49, 0x05, 0x27, 0x96, 0x73, 0x43, 0x27, 0x41, 0x0a, 0xb4, 0x90, 0x62, 0xd8, 0xcd,
			0xc1, 0x20, 0xc7, 0x3a, 0x20, 0xa0, 0xb7, 0x99, 0x1d, 0x0a, 0x90, 0x93, 0x10, 0x84, 0x3f, 0xd2,
			0x6c, 0x2d, 0x9e, 0xf8, 0x8f, 0x37, 0x9f, 0xab, 0xe1, 0xbf, 0x1f, 0x91, 0xc8, 0xee, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbf, 0xce, 0x9d, 0x8f, 0x67, 0xa6,
			0xda, 0x19, 0x4e, 0xa8, 0xe6, 0xc1, 0x5a, 0x7e, 0x25, 0xb0, 0x6c, 0xd0, 0x01, 0x0d, 0x6a, 0x44,
			0xdc, 0xfd, 0x3d, 0x7a, 0x21, 0xa3, 0xce, 0x1a, 0x32, 0x2c, 0xfa, 0xce, 0x75, 0xb5, 0xaf, 0x3d,
			0x55, 0x6d, 0xd0, 0x06, 0x32, 0x72, 0x43, 0xd2, 0x72, 0xf0, 0xed, 0x00, 0x00, 0x00
		};

		const struct {
			const char *msg;
			const char *dst;
			Fp2Str x;
			Fp2Str y;
		} tbl[] = {
			// https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-hash-to-curve-07#appendix-G.10.1
			{
				"", // msg
				"BLS12381G2_XMD:SHA-256_SSWU_RO_TESTGEN",
				{ // P.x
					"0x0a650bd36ae7455cb3fe5d8bb1310594551456f5c6593aec9ee0c03d2f6cb693bd2c5e99d4e23cbaec767609314f51d3",
					"0x0fbdae26f9f9586a46d4b0b70390d09064ef2afe5c99348438a3c7d9756471e015cb534204c1b6824617a85024c772dc",
				},
				{ // P.y
					"0x0d8d49e7737d8f9fc5cef7c4b8817633103faf2613016cb86a1f3fc29968fe2413e232d9208d2d74a89bf7a48ac36f83",
					"0x02e5cf8f9b7348428cc9e66b9a9b36fe45ba0b0a146290c3a68d92895b1af0e1f2d9f889fb412670ae8478d8abd4c5aa",
				}
			},
			{
				"abc",
				"BLS12381G2_XMD:SHA-256_SSWU_RO_TESTGEN",
				{
					"0x1953ce6d4267939c7360756d9cca8eb34aac4633ef35369a7dc249445069888e7d1b3f9d2e75fbd468fbcbba7110ea02",
					"0x03578447618463deb106b60e609c6f7cc446dc6035f84a72801ba17c94cd800583b493b948eff0033f09086fdd7f6175",
				},
				{
					"0x0882ab045b8fe4d7d557ebb59a63a35ac9f3d312581b509af0f8eaa2960cbc5e1e36bb969b6e22980b5cbdd0787fcf4e",
					"0x0184d26779ae9d4670aca9b267dbd4d3b30443ad05b8546d36a195686e1ccc3a59194aea05ed5bce7c3144a29ec047c4",
				},
			},
			{
				"abcdef0123456789",
				"BLS12381G2_XMD:SHA-256_SSWU_RO_TESTGEN",
				{
					"0x17b461fc3b96a30c2408958cbfa5f5927b6063a8ad199d5ebf2d7cdeffa9c20c85487204804fab53f950b2f87db365aa",
					"0x195fad48982e186ce3c5c82133aefc9b26d55979b6f530992a8849d4263ec5d57f7a181553c8799bcc83da44847bdc8d",
				},
				{
					"0x174a3473a3af2d0302b9065e895ca4adba4ece6ce0b41148ba597001abb152f852dd9a96fb45c9de0a43d944746f833e",
					"0x005cdf3d984e3391e7e969276fb4bc02323c5924a4449af167030d855acc2600cf3d4fab025432c6d868c79571a95bef",
				},
			},
			{
				"a512_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
				"BLS12381G2_XMD:SHA-256_SSWU_RO_TESTGEN",
				{
					"0x0a162306f3b0f2bb326f0c4fb0e1fea020019c3af796dcd1d7264f50ddae94cacf3cade74603834d44b9ab3d5d0a6c98",
					"0x123b6bd9feeba26dd4ad00f8bfda2718c9700dc093ea5287d7711844644eb981848316d3f3f57d5d3a652c6cdc816aca",
				},
				{
					"0x15c1d4f1a685bb63ee67ca1fd96155e3d091e852a684b78d085fd34f6091e5249ddddbdcf2e7ec82ce6c04c63647eeb7",
					"0x05483f3b96d9252dd4fc0868344dfaf3c9d145e3387db23fa8e449304fab6a7b6ec9c15f05c0a1ea66ff0efcc03e001a",
				},
			},
			// https://www.ietf.org/id/draft-irtf-cfrg-hash-to-curve-08.html#name-bls12381g2_xmdsha-256_sswu_
			{
				"", // msg
				"QUUX-V01-CS02-with-BLS12381G2_XMD:SHA-256_SSWU_RO_",
				{ // P.x
					"0x0141ebfbdca40eb85b87142e130ab689c673cf60f1a3e98d69335266f30d9b8d4ac44c1038e9dcdd5393faf5c41fb78a",
					"0x05cb8437535e20ecffaef7752baddf98034139c38452458baeefab379ba13dff5bf5dd71b72418717047f5b0f37da03d",
				},
				{ // P.y
					"0x0503921d7f6a12805e72940b963c0cf3471c7b2a524950ca195d11062ee75ec076daf2d4bc358c4b190c0c98064fdd92",
					"0x12424ac32561493f3fe3c260708a12b7c620e7be00099a974e259ddc7d1f6395c3c811cdd19f1e8dbf3e9ecfdcbab8d6",
				}
			},
			// https://www.ietf.org/id/draft-irtf-cfrg-hash-to-curve-09.html#name-bls12381g2_xmdsha-256_sswu_
			{
				"abc", // msg
				"QUUX-V01-CS02-with-BLS12381G2_XMD:SHA-256_SSWU_RO_",
				{ // P.x
					"0x02c2d18e033b960562aae3cab37a27ce00d80ccd5ba4b7fe0e7a210245129dbec7780ccc7954725f4168aff2787776e6",
					"0x139cddbccdc5e91b9623efd38c49f81a6f83f175e80b06fc374de9eb4b41dfe4ca3a230ed250fbe3a2acf73a41177fd8",
				},
				{ // P.y
					"0x1787327b68159716a37440985269cf584bcb1e621d3a7202be6ea05c4cfe244aeb197642555a0645fb87bf7466b2ba48",
					"0x00aa65dae3c8d732d10ecd2c50f8a1baf3001578f71c694e03866e9f3d49ac1e1ce70dd94a733534f106d4cec0eddd16",
				}
			},
			{
				"abcdef0123456789", // msg
				"QUUX-V01-CS02-with-BLS12381G2_XMD:SHA-256_SSWU_RO_",
				{ // P.x
					"0x121982811d2491fde9ba7ed31ef9ca474f0e1501297f68c298e9f4c0028add35aea8bb83d53c08cfc007c1e005723cd0",
					"0x190d119345b94fbd15497bcba94ecf7db2cbfd1e1fe7da034d26cbba169fb3968288b3fafb265f9ebd380512a71c3f2c",
				},
				{ // P.y
					"0x05571a0f8d3c08d094576981f4a3b8eda0a8e771fcdcc8ecceaf1356a6acf17574518acb506e435b639353c2e14827c8",
					"0x0bb5e7572275c567462d91807de765611490205a941a5a6af3b1691bfe596c31225d3aabdf15faff860cb4ef17c7c3be",
				}
			},
			{
				"q128_qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq", // msg
				"QUUX-V01-CS02-with-BLS12381G2_XMD:SHA-256_SSWU_RO_",
				{ // P.x
					"0x19a84dd7248a1066f737cc34502ee5555bd3c19f2ecdb3c7d9e24dc65d4e25e50d83f0f77105e955d78f4762d33c17da",
					"0x0934aba516a52d8ae479939a91998299c76d39cc0c035cd18813bec433f587e2d7a4fef038260eef0cef4d02aae3eb91",
				},
				{ // P.y
					"0x14f81cd421617428bc3b9fe25afbb751d934a00493524bc4e065635b0555084dd54679df1536101b2c979c0152d09192",
					"0x09bcccfa036b4847c9950780733633f13619994394c23ff0b32fa6b795844f4a0673e20282d07bc69641cee04f5e5662",
				}
			},
			{
				"a512_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", // msg
				"QUUX-V01-CS02-with-BLS12381G2_XMD:SHA-256_SSWU_RO_",
				{ // P.x
					"0x01a6ba2f9a11fa5598b2d8ace0fbe0a0eacb65deceb476fbbcb64fd24557c2f4b18ecfc5663e54ae16a84f5ab7f62534",
					"0x11fca2ff525572795a801eed17eb12785887c7b63fb77a42be46ce4a34131d71f7a73e95fee3f812aea3de78b4d01569",
				},
				{ // P.y
					"0x0b6798718c8aed24bc19cb27f866f1c9effcdbf92397ad6448b5c9db90d2b9da6cbabf48adc1adf59a1a28344e79d57e",
					"0x03a47f8e6d1763ba0cad63d6114c0accbef65707825a511b251a660a9b3994249ae4e63fac38b23da0c398689ee2ab52",
				}
			},
			{
				(const char*)largeMsg,
				(const char*)largeDst,
				{
					"1621198997308164855924679271466068736858072552785749054645341480120465596875824494080137207843099766374306872449248",
					"1083281626011882732241684404246060395112782337887047566845349523874158040278646833293564936724890128537465803996399",
				},
				{
					"3796472889358084161418545238718592151842923587110768242624300389546030860679313089401746511331063864568256792368175",
					"3428585241079663876306492378538920664201248193908204300130152438040549594213769385563811847347660927609220812378737",
				}
			},
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			const char *msg = tbl[i].msg;
			size_t msgSize = strlen(msg);
			const char *dst = tbl[i].dst;
			size_t dstSize = strlen(dst);
			if (msg == (const char*)largeMsg) {
				msgSize = sizeof(largeMsg);
				dstSize = sizeof(largeDst);
			}
			G2 P1, P2;
			set(P1.x, tbl[i].x);
			set(P1.y, tbl[i].y);
			P1.z = 1;
			mapto.msgToG2(P2, msg, msgSize, dst, dstSize);
			CYBOZU_TEST_EQUAL(P1, P2);
		}
		{
			G2 P;
			mcl::bn::hashAndMapToG2(P, "asdf", 4);
			CYBOZU_BENCH_C("draft07 hashAndMapToG2", 1000, mcl::bn::hashAndMapToG2, P, "asdf", 4);
			P.normalize();
			printf("P=%s %s\n", P.x.getStr(10).c_str(), P.y.getStr(10).c_str());
		}
	}
}

void testEth2phase0()
{
	const struct {
		const char *sec;
		const char *msg;
		const char *out;
	} tbl[] = {
		{
			"328388aff0d4a5b7dc9205abd374e7e98f3cd9f3418edb4eafda5fb16473d216",
			"abababababababababababababababababababababababababababababababab",
			"ae82747ddeefe4fd64cf9cedb9b04ae3e8a43420cd255e3c7cd06a8d88b7c7f8638543719981c5d16fa3527c468c25f0026704a6951bde891360c7e8d12ddee0559004ccdbe6046b55bae1b257ee97f7cdb955773d7cf29adf3ccbb9975e4eb9",
		},
		{
			"47b8192d77bf871b62e87859d653922725724a5c031afeabc60bcef5ff665138",
			"abababababababababababababababababababababababababababababababab",
			"9674e2228034527f4c083206032b020310face156d4a4685e2fcaec2f6f3665aa635d90347b6ce124eb879266b1e801d185de36a0a289b85e9039662634f2eea1e02e670bc7ab849d006a70b2f93b84597558a05b879c8d445f387a5d5b653df",
		},
		{
			"328388aff0d4a5b7dc9205abd374e7e98f3cd9f3418edb4eafda5fb16473d216",
			"5656565656565656565656565656565656565656565656565656565656565656",
			"a4efa926610b8bd1c8330c918b7a5e9bf374e53435ef8b7ec186abf62e1b1f65aeaaeb365677ac1d1172a1f5b44b4e6d022c252c58486c0a759fbdc7de15a756acc4d343064035667a594b4c2a6f0b0b421975977f297dba63ee2f63ffe47bb6",
		},
		{
			"47b8192d77bf871b62e87859d653922725724a5c031afeabc60bcef5ff665138",
			"5656565656565656565656565656565656565656565656565656565656565656",
			"af1390c3c47acdb37131a51216da683c509fce0e954328a59f93aebda7e4ff974ba208d9a4a2a2389f892a9d418d618418dd7f7a6bc7aa0da999a9d3a5b815bc085e14fd001f6a1948768a3f4afefc8b8240dda329f984cb345c6363272ba4fe",
		},
		{
			"263dbd792f5b1be47ed85f8938c0f29586af0d3ac7b977f21c278fe1462040e3",
			"0000000000000000000000000000000000000000000000000000000000000000",
			"b6ed936746e01f8ecf281f020953fbf1f01debd5657c4a383940b020b26507f6076334f91e2366c96e9ab279fb5158090352ea1c5b0c9274504f4f0e7053af24802e51e4568d164fe986834f41e55c8e850ce1f98458c0cfc9ab380b55285a55",
		},
		{
			"47b8192d77bf871b62e87859d653922725724a5c031afeabc60bcef5ff665138",
			"0000000000000000000000000000000000000000000000000000000000000000",
			"b23c46be3a001c63ca711f87a005c200cc550b9429d5f4eb38d74322144f1b63926da3388979e5321012fb1a0526bcd100b5ef5fe72628ce4cd5e904aeaa3279527843fae5ca9ca675f4f51ed8f83bbf7155da9ecc9663100a885d5dc6df96d9",
		},
		{
			"328388aff0d4a5b7dc9205abd374e7e98f3cd9f3418edb4eafda5fb16473d216",
			"0000000000000000000000000000000000000000000000000000000000000000",
			"948a7cb99f76d616c2c564ce9bf4a519f1bea6b0a624a02276443c245854219fabb8d4ce061d255af5330b078d5380681751aa7053da2c98bae898edc218c75f07e24d8802a17cd1f6833b71e58f5eb5b94208b4d0bb3848cecb075ea21be115",
		},
		{
			"263dbd792f5b1be47ed85f8938c0f29586af0d3ac7b977f21c278fe1462040e3",
			"abababababababababababababababababababababababababababababababab",
			"91347bccf740d859038fcdcaf233eeceb2a436bcaaee9b2aa3bfb70efe29dfb2677562ccbea1c8e061fb9971b0753c240622fab78489ce96768259fc01360346da5b9f579e5da0d941e4c6ba18a0e64906082375394f337fa1af2b7127b0d121",
		},
		{
			"263dbd792f5b1be47ed85f8938c0f29586af0d3ac7b977f21c278fe1462040e3",
			"5656565656565656565656565656565656565656565656565656565656565656",
			"882730e5d03f6b42c3abc26d3372625034e1d871b65a8a6b900a56dae22da98abbe1b68f85e49fe7652a55ec3d0591c20767677e33e5cbb1207315c41a9ac03be39c2e7668edc043d6cb1d9fd93033caa8a1c5b0e84bedaeb6c64972503a43eb",
		},
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const Uint8Vec msg = fromHexStr(tbl[i].msg);
		const Uint8Vec out = fromHexStr(tbl[i].out);
		Fr r;
		r.setStr(tbl[i].sec, 16);
		G2 P;
		mcl::bn::hashAndMapToG2(P, msg.data(), msg.size());
		P *= r;
		P.normalize();
		uint8_t buf[256];
		size_t n = P.serialize(buf, sizeof(buf));
		CYBOZU_TEST_EQUAL_ARRAY(out.data(), buf, n);
	}
}

template<class T>
void testSswuG1(const T& mapto)
{
	const struct {
		const char *u;
		const char *xn;
		const char *xd;
		const char *y;
	} tbl[] = {
		{
			"0",
			"2906670324641927570491258158026293881577086121416628140204402091718288198173574630967936031029026176254968826637280",
			"134093699507829814821517650980559345626771735832728306571853989028117161444712301203928819168120125800913069360447",
			"883926319761702754759909536142450234040420493353017578303105057331414514426056372828799438842649753623273850162620",
		},
		{
			"1",
			"1899737305729263819017890260937734483867440857300594896394519620134021106669873067956151260450660652775675911846846",
			"2393285161127709615559578013969192009035621989946268206469810267786625713154290249995541799111574154426937440234423",
			"930707443353688021592152842018127582116075842630002779852379799673382026358889394936840703051493045692645732041175",
		},
		{
			"2445954111132780748727614926881625117054159133000189976501123519233969822355358926084559381412726536178576396564099",
			"1380948948858039589493865757655255282539355225819860723137103295095584615993188368169864518071716731687572756871254",
			"3943815976847699234459109633672806041428347164453405394564656059649800794974863796342327007702642595444543195342842",
			"2822129059347872230939996033946474192520362213555773694753196763199812747558444338256205967106315253391997542043187",
		},
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp u;
		u.setStr(tbl[i].u);
		Fp xn, xd, y;
		mapto.sswuG1(xn, xd, y, u);
		CYBOZU_TEST_EQUAL(xn.getStr(), tbl[i].xn);
		CYBOZU_TEST_EQUAL(xd.getStr(), tbl[i].xd);
		CYBOZU_TEST_EQUAL(y.getStr(), tbl[i].y);
	}
}

template<class T>
void testMsgToG1(const T& mapto)
{
	const struct {
		const char *msg;
		const char *dst;
		const char *x;
		const char *y;
	} tbl[] = {
		{
			// generated by draft-irtf-cfrg-hash-to-curve/poc/suite_bls12381g1.sage
			"asdf",
			"BLS_SIG_BLS12381G1_XMD:SHA-256_SSWU_RO_POP_",
			"a72df17570d0eb81260042edbea415ad49bdb94a1bc1ce9d1bf147d0d48268170764bb513a3b994d662e1faba137106",
			"122b77eca1ed58795b7cd456576362f4f7bd7a572a29334b4817898a42414d31e9c0267f2dc481a4daf8bcf4a460322",
		},
		// https://www.ietf.org/id/draft-irtf-cfrg-hash-to-curve-09.txt
		// H.9.1.  BLS12381G1_XMD:SHA-256_SSWU_RO_
		{
			"",
			"QUUX-V01-CS02-with-BLS12381G1_XMD:SHA-256_SSWU_RO_",
			"052926add2207b76ca4fa57a8734416c8dc95e24501772c814278700eed6d1e4e8cf62d9c09db0fac349612b759e79a1",
			"08ba738453bfed09cb546dbb0783dbb3a5f1f566ed67bb6be0e8c67e2e81a4cc68ee29813bb7994998f3eae0c9c6a265",
		},
		{
			"abc",
			"QUUX-V01-CS02-with-BLS12381G1_XMD:SHA-256_SSWU_RO_",
			"03567bc5ef9c690c2ab2ecdf6a96ef1c139cc0b2f284dca0a9a7943388a49a3aee664ba5379a7655d3c68900be2f6903",
			"0b9c15f3fe6e5cf4211f346271d7b01c8f3b28be689c8429c85b67af215533311f0b8dfaaa154fa6b88176c229f2885d",
		},
		{
			"abcdef0123456789",
			"QUUX-V01-CS02-with-BLS12381G1_XMD:SHA-256_SSWU_RO_",
			"11e0b079dea29a68f0383ee94fed1b940995272407e3bb916bbf268c263ddd57a6a27200a784cbc248e84f357ce82d98",
			"03a87ae2caf14e8ee52e51fa2ed8eefe80f02457004ba4d486d6aa1f517c0889501dc7413753f9599b099ebcbbd2d709",
		},
		{
			"q128_qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq",
			"QUUX-V01-CS02-with-BLS12381G1_XMD:SHA-256_SSWU_RO_",
			"15f68eaa693b95ccb85215dc65fa81038d69629f70aeee0d0f677cf22285e7bf58d7cb86eefe8f2e9bc3f8cb84fac488",
			"1807a1d50c29f430b8cafc4f8638dfeeadf51211e1602a5f184443076715f91bb90a48ba1e370edce6ae1062f5e6dd38",
		},
		{
			"a512_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
			"QUUX-V01-CS02-with-BLS12381G1_XMD:SHA-256_SSWU_RO_",
			"082aabae8b7dedb0e78aeb619ad3bfd9277a2f77ba7fad20ef6aabdc6c31d19ba5a6d12283553294c1825c4b3ca2dcfe",
			"05b84ae5a942248eea39e1d91030458c40153f3b654ab7872d779ad1e942856a20c438e8d99bc8abfbf74729ce1f7ac8",
		},
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		const char *msg = tbl[i].msg;
		const size_t msgSize = strlen(msg);
		const char *dst = tbl[i].dst;
		const size_t dstSize = strlen(dst);
		G1 P, Q;
		mapto.msgToG1(P, msg, msgSize, dst, dstSize);
		Q.x.setStr(tbl[i].x, 16);
		Q.y.setStr(tbl[i].y, 16);
		Q.z = 1;
		CYBOZU_TEST_EQUAL(P, Q);
CYBOZU_BENCH_C("msgToG1", 1000, mapto.msgToG1, P, msg, msgSize, dst, dstSize);
		if (i == 0) { // correct dst
			P.clear();
			bn::hashAndMapToG1(P, msg, msgSize);
			CYBOZU_TEST_EQUAL(P, Q);
		}
	}
}

std::string appendZeroToRight(const std::string& s, size_t n)
{
	if (s.size() >= n) return s;
	return std::string(n - s.size(), '0') + s;
}

template<class T>
void testFpToG1(const T& mapto)
{
	const struct {
		const char *in;
		const char *out;
	} tbl[] = {
		// https://github.com/matter-labs/eip1962/blob/master/src/test/test_vectors/eip2537/fp_to_g1.csv
		{
			"0000000000000000000000000000000014406e5bfb9209256a3820879a29ac2f62d6aca82324bf3ae2aa7d3c54792043bd8c791fccdb080c1a52dc68b8b69350",
			"000000000000000000000000000000000d7721bcdb7ce1047557776eb2659a444166dc6dd55c7ca6e240e21ae9aa18f529f04ac31d861b54faf3307692545db700000000000000000000000000000000108286acbdf4384f67659a8abe89e712a504cb3ce1cba07a716869025d60d499a00d1da8cdc92958918c222ea93d87f0",
		},
		{
			"000000000000000000000000000000000e885bb33996e12f07da69073e2c0cc880bc8eff26d2a724299eb12d54f4bcf26f4748bb020e80a7e3794a7b0e47a641",
			"00000000000000000000000000000000191ba6e4c4dafa22c03d41b050fe8782629337641be21e0397dc2553eb8588318a21d30647182782dee7f62a22fd020c000000000000000000000000000000000a721510a67277eabed3f153bd91df0074e1cbd37ef65b85226b1ce4fb5346d943cf21c388f0c5edbc753888254c760a",
		},
		{
			"000000000000000000000000000000000ba1b6d79150bdc368a14157ebfe8b5f691cf657a6bbe30e79b6654691136577d2ef1b36bfb232e3336e7e4c9352a8ed",
			"000000000000000000000000000000001658c31c0db44b5f029dba56786776358f184341458577b94d3a53c877af84ffbb1a13cc47d228a76abb4b67912991850000000000000000000000000000000018cf1f27eab0a1a66f28a227bd624b7d1286af8f85562c3f03950879dd3b8b4b72e74b034223c6fd93de7cd1ade367cb",
		},
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Uint8Vec v = fromHexStr(tbl[i].in);
		Fp x;
		/*
			serialize() accepts only values in [0, p).
			The test values exceed p, so use setBigEndianMod to set (v mod p).
		*/
		x.setBigEndianMod(v.data(), v.size());
		G1 P;
		mapto.FpToG1(P, x);
		/*
			The test value is the form such as "000...<x>000...<y>".
			So normalize P and pad zeros to compare it.
		*/
		P.normalize();
		const size_t L = 128;
		std::string s = appendZeroToRight(P.x.getStr(16), L) + appendZeroToRight(P.y.getStr(16), L);
		CYBOZU_TEST_EQUAL(s, tbl[i].out);
		{
			G1 Q;
			mcl::bn::mapToG1(Q, x);
			CYBOZU_TEST_EQUAL(P, Q);
		}
	}
}

template<class T>
void testSameUV(const T& mapto)
{
	// u is equal to v
	const struct {
		const char *in;
		const char *x;
		const char *y;
		const char *x2;
		const char *y2;
	} tbl[] = {
		{
			"4a6975dab86d4d3320186347d0861fcccb083a6a08462e64f05d90a30599de14",
			"11e1a490dbbe5c0c55a174ef60a96afadc3399f9a5b19ade1c261bad04cbc5d587084693187e25496d442995e2de6c7b",
			"175048bd7270177570c013d5ea0789da1d7715c0bbcf1613d6f3c2738efe28dabef3b626f226b3c87d0f6421459dbf13",
			"11af691e69c9eee42642af1ab9dc571c00fdfd3ce490d3ebfc6051033f56e95d818e6d457bfac95f0f4ed922480511d5 14ed0779f31dc441593f541a73f71c21167ad62d0634cbea09a6973b2e94a68baa9d1fed9cd69f21a2c1fc60c82a2822",
			"14c196deb0bdd1127821190aa7a555c7f2544d4dcd45db52d1b9d1f27e0566aafa5b65f5be3bd44ee4cdc94b6fb733ca 13f4bb86ec914c82f0a9dfa0a028777b7fcade21e1982830b99e054b5a5467616c623cefd8da39fa3692ce2ce8aa2a3c",
		},
		{
			"0",
			"19b6652bc7e44b6ca66a7803d1dff1b2d0fd02a32fa1b09f43716e21fec0b508e688e87b2d7a03618c066409ad53665c",
			"10549370803d643dee27b367d4381b08e1655cc8887914917419eed52ad0472115c9fac1a14974ddea16ada22eb37ba7",
			"19da1b4d47efeeb154f8968b43da2125376e0999ba722141419b03fd857490562fa42a5d0973956d1932dd20c1e0a284 18426da25dadd359adfda64fbaddac4414da2a841cb467935289877db450fac424361efb2e7fb141b7b98e6b2f888aef",
			"c2f8d431770d9be9b087c36fc5b66bb83ce6372669f48294193ef646105e0f21d17b134e7d1ad9c18f54b81f6a3707b 3257c3be77016e69b75905a97871008a6dfd2e324a6748c48d3304380156987bd0905991824936fcfe34ab25c3b6caa",
		},
		{
			"1",
			"1eb4296187e4ff28b54dfb40be85e1725d99e78b57225d76fd9673f6cf444616ddb5bc5d1e849a7cb58aa0bf8db9120",
			"7c1de2aaf8adec05ae75cf10fd27cde7477987936b8455ba344c625873a5381a64b7754135d8ec61bced3107abd9dce",
			"b296300065ea264a6d4e0bd647b4c03d607ce7fc8897f618b7642c81e16b5f5f2c2858adc69104eed8161f1e9682829 176bee4273421cd9e13467f5c39493708a0468dc6e54f4b7c20394ebeab4d983d5ebf1c27158ce0ca7179a01dc8b10d6",
			"b9b6c01f849b11a4f96db02a32b3146085871da1807b5cee7e075e582768d979b667650208b92dd359480ab3761af4a 45166b53c4fb92f85a63f23517f7581b7fa1a5b06060932fcf9a8a0ddd873f812fcbb5fe9a527fdacc668b2df12b2c9",
		},
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		Fp x;
		x.setStr(tbl[i].in, 16);
		G1 P;
		mapto.FpToG1(P, x, &x);
		P.normalize();
		CYBOZU_TEST_EQUAL(P.x.getStr(16), tbl[i].x);
		CYBOZU_TEST_EQUAL(P.y.getStr(16), tbl[i].y);
		Fp2 x2;
		x2.a = x;
		x2.b = x;
		G2 Q;
		mapto.Fp2ToG2(Q, x2, &x2);
		Q.normalize();
		CYBOZU_TEST_EQUAL(Q.x.getStr(16), tbl[i].x2);
		CYBOZU_TEST_EQUAL(Q.y.getStr(16), tbl[i].y2);
	}
}

template<class T>
void testSetDst(const T& mapto)
{
	const char *dst = "abc";
	bool ret;
	ret = setDstG1(dst, strlen(dst));
	CYBOZU_TEST_ASSERT(ret);
	CYBOZU_TEST_EQUAL(mapto.dstG1.dst, dst);
	ret = setDstG1("def", 1000);
	CYBOZU_TEST_ASSERT(!ret);
	CYBOZU_TEST_EQUAL(mapto.dstG1.dst, dst);

	ret = setDstG2(dst, strlen(dst));
	CYBOZU_TEST_ASSERT(ret);
	CYBOZU_TEST_EQUAL(mapto.dstG2.dst, dst);
	ret = setDstG2("def", 1000);
	CYBOZU_TEST_ASSERT(!ret);
	CYBOZU_TEST_EQUAL(mapto.dstG2.dst, dst);
}

CYBOZU_TEST_AUTO(test)
{
	initPairing(mcl::BLS12_381);
	Fp::setETHserialization(true);
	bn::setMapToMode(MCL_MAP_TO_MODE_HASH_TO_CURVE_07);
	const MapTo& mapto = BN::param.mapTo.mapTo_WB19_;
	addTest();
	iso3Test(mapto);
	testHMAC();
	testHashToFp2v7(mapto);
	testEth2phase0();
	testSswuG1(mapto);
	testMsgToG1(mapto);
	testFpToG1(mapto);
	testSameUV(mapto);
	// this test should be last
	testSetDst(mapto);
}
