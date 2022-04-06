#pragma once
/**
	@file
	@brief map to G2 on BLS12-381 (must be included from mcl/bn.hpp)
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
	ref. https://eprint.iacr.org/2019/403 , https://github.com/algorand/bls_sigs_ref
*/
namespace mcl {

namespace local {

// y^2 = x^3 + 4(1 + i)
template<class F>
struct PointT {
	typedef F Fp;
	F x, y, z;
	static F a_;
	static F b_;
	static int specialA_;
	bool isZero() const
	{
		return z.isZero();
	}
	void clear()
	{
		x.clear();
		y.clear();
		z.clear();
	}
	bool isEqual(const PointT<F>& rhs) const
	{
		return ec::isEqualJacobi(*this, rhs);
	}
};

template<class F> F PointT<F>::a_;
template<class F> F PointT<F>::b_;
template<class F> int PointT<F>::specialA_ = ec::GenericA;

} // mcl::local

template<class Fp, class G1, class Fp2, class G2>
struct MapTo_WB19 {
	typedef local::PointT<Fp> E1;
	typedef local::PointT<Fp2> E2;
	struct Dst {
		static const size_t maxDstLen = 64;
		char dst[maxDstLen + 1];
		size_t len;
		bool set(const char *dst_, size_t len_)
		{
			if (len_ > maxDstLen) return false;
			len = len_;
			memcpy(dst, dst_, len_);
			dst[len_] = '\0';
			return true;
		}
	};
	Dst dstG1;
	Dst dstG2;
	mpz_class sqrtConst; // (p^2 - 9) / 16
	Fp2 root4[4];
	Fp2 etas[4];
	Fp2 xnum[4];
	Fp2 xden[3];
	Fp2 ynum[4];
	Fp2 yden[4];
	Fp g1c1, g1c2;
	Fp g1xnum[12];
	Fp g1xden[11];
	Fp g1ynum[16];
	Fp g1yden[16];
	mpz_class g1cofactor;
	int g1Z;
	void init()
	{
		bool b;
		E2::a_.a = 0;
		E2::a_.b = 240;
		E2::b_.a = 1012;
		E2::b_.b = 1012;
		sqrtConst = Fp::getOp().mp;
		sqrtConst *= sqrtConst;
		sqrtConst -= 9;
		sqrtConst /= 16;
		const char *rv1Str = "0x6af0e0437ff400b6831e36d6bd17ffe48395dabc2d3435e77f76e17009241c5ee67992f72ec05f4c81084fbede3cc09";
		root4[0].a = 1;
		root4[0].b.clear();
		root4[1].a.clear();
		root4[1].b = 1;
		root4[2].a.setStr(&b, rv1Str);
		assert(b); (void)b;
		root4[2].b = root4[2].a;
		root4[3].a = root4[2].a;
		Fp::neg(root4[3].b, root4[3].a);
		const char *ev1Str = "0x699be3b8c6870965e5bf892ad5d2cc7b0e85a117402dfd83b7f4a947e02d978498255a2aaec0ac627b5afbdf1bf1c90";
		const char *ev2Str = "0x8157cd83046453f5dd0972b6e3949e4288020b5b8a9cc99ca07e27089a2ce2436d965026adad3ef7baba37f2183e9b5";
		const char *ev3Str = "0xab1c2ffdd6c253ca155231eb3e71ba044fd562f6f72bc5bad5ec46a0b7a3b0247cf08ce6c6317f40edbc653a72dee17";
		const char *ev4Str = "0xaa404866706722864480885d68ad0ccac1967c7544b447873cc37e0181271e006df72162a3d3e0287bf597fbf7f8fc1";
		Fp& ev1 = etas[0].a;
		Fp& ev2 = etas[0].b;
		Fp& ev3 = etas[2].a;
		Fp& ev4 = etas[2].b;
		ev1.setStr(&b, ev1Str);
		assert(b); (void)b;
		ev2.setStr(&b, ev2Str);
		assert(b); (void)b;
		Fp::neg(etas[1].a, ev2);
		etas[1].b = ev1;
		ev3.setStr(&b, ev3Str);
		assert(b); (void)b;
		ev4.setStr(&b, ev4Str);
		assert(b); (void)b;
		Fp::neg(etas[3].a, ev4);
		etas[3].b = ev3;
		init_iso3();
		{
			const char *A = "0x144698a3b8e9433d693a02c96d4982b0ea985383ee66a8d8e8981aefd881ac98936f8da0e0f97f5cf428082d584c1d";
			const char *B = "0x12e2908d11688030018b12e8753eee3b2016c1f0f24f4070a0b9c14fcef35ef55a23215a316ceaa5d1cc48e98e172be0";
			const char *c1 = "0x680447a8e5ff9a692c6e9ed90d2eb35d91dd2e13ce144afd9cc34a83dac3d8907aaffffac54ffffee7fbfffffffeaaa";
			const char *c2 = "0x3d689d1e0e762cef9f2bec6130316806b4c80eda6fc10ce77ae83eab1ea8b8b8a407c9c6db195e06f2dbeabc2baeff5";
			E1::a_.setStr(&b, A);
			assert(b); (void)b;
			E1::b_.setStr(&b, B);
			assert(b); (void)b;
			g1c1.setStr(&b, c1);
			assert(b); (void)b;
			g1c2.setStr(&b, c2);
			assert(b); (void)b;
			g1Z = 11;
			gmp::setStr(&b, g1cofactor, "d201000000010001", 16);
			assert(b); (void)b;
		}
		init_iso11();
		const char *dst = "BLS_SIG_BLS12381G1_XMD:SHA-256_SSWU_RO_POP_";
		bool ret = dstG1.set(dst, strlen(dst));
		assert(ret); (void)ret;
		dst = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";
		ret = dstG2.set(dst, strlen(dst));
		assert(ret); (void)ret;
	}
	void initArray(Fp *dst, const char **s, size_t n) const
	{
		bool b;
		for (size_t i = 0; i < n; i++) {
			dst[i].setStr(&b, s[i]);
			assert(b);
			(void)b;
		}
	}
	void init_iso3()
	{
		const char *tbl[] = {
			"0x5c759507e8e333ebb5b7a9a47d7ed8532c52d39fd3a042a88b58423c50ae15d5c2638e343d9c71c6238aaaaaaaa97d6",
			"0x11560bf17baa99bc32126fced787c88f984f87adf7ae0c7f9a208c6b4f20a4181472aaa9cb8d555526a9ffffffffc71a",
			"0x11560bf17baa99bc32126fced787c88f984f87adf7ae0c7f9a208c6b4f20a4181472aaa9cb8d555526a9ffffffffc71e",
			"0x8ab05f8bdd54cde190937e76bc3e447cc27c3d6fbd7063fcd104635a790520c0a395554e5c6aaaa9354ffffffffe38d",
			"0x171d6541fa38ccfaed6dea691f5fb614cb14b4e7f4e810aa22d6108f142b85757098e38d0f671c7188e2aaaaaaaa5ed1",
			"0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaa63",
			"0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaa9f",
			"0x1530477c7ab4113b59a4c18b076d11930f7da5d4a07f649bf54439d87d27e500fc8c25ebf8c92f6812cfc71c71c6d706",
			"0x5c759507e8e333ebb5b7a9a47d7ed8532c52d39fd3a042a88b58423c50ae15d5c2638e343d9c71c6238aaaaaaaa97be",
			"0x11560bf17baa99bc32126fced787c88f984f87adf7ae0c7f9a208c6b4f20a4181472aaa9cb8d555526a9ffffffffc71c",
			"0x8ab05f8bdd54cde190937e76bc3e447cc27c3d6fbd7063fcd104635a790520c0a395554e5c6aaaa9354ffffffffe38f",
			"0x124c9ad43b6cf79bfbf7043de3811ad0761b0f37a1e26286b0e977c69aa274524e79097a56dc4bd9e1b371c71c718b10",
			"0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffa8fb",
			"0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffa9d3",
			"0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaa99",
		};
		bool b;
		xnum[0].a.setStr(&b, tbl[0]); assert(b); (void)b;
		xnum[0].b = xnum[0].a;
		xnum[1].a.clear();
		xnum[1].b.setStr(&b, tbl[1]); assert(b); (void)b;
		xnum[2].a.setStr(&b, tbl[2]); assert(b); (void)b;
		xnum[2].b.setStr(&b, tbl[3]); assert(b); (void)b;
		xnum[3].a.setStr(&b, tbl[4]); assert(b); (void)b;
		xnum[3].b.clear();
		xden[0].a.clear();
		xden[0].b.setStr(&b, tbl[5]); assert(b); (void)b;
		xden[1].a = 0xc;
		xden[1].b.setStr(&b, tbl[6]); assert(b); (void)b;
		xden[2].a = 1;
		xden[2].b = 0;
		ynum[0].a.setStr(&b, tbl[7]); assert(b); (void)b;
		ynum[0].b = ynum[0].a;
		ynum[1].a.clear();
		ynum[1].b.setStr(&b, tbl[8]); assert(b); (void)b;
		ynum[2].a.setStr(&b, tbl[9]); assert(b); (void)b;
		ynum[2].b.setStr(&b, tbl[10]); assert(b); (void)b;
		ynum[3].a.setStr(&b, tbl[11]); assert(b); (void)b;
		ynum[3].b.clear();
		yden[0].a.setStr(&b, tbl[12]); assert(b); (void)b;
		yden[0].b = yden[0].a;
		yden[1].a.clear();
		yden[1].b.setStr(&b, tbl[13]); assert(b); (void)b;
		yden[2].a = 0x12;
		yden[2].b.setStr(&b, tbl[14]); assert(b); (void)b;
		yden[3].a = 1;
		yden[3].b.clear();
	}
	void init_iso11()
	{
		const char *xnumStr[] = {
			"0x11a05f2b1e833340b809101dd99815856b303e88a2d7005ff2627b56cdb4e2c85610c2d5f2e62d6eaeac1662734649b7",
			"0x17294ed3e943ab2f0588bab22147a81c7c17e75b2f6a8417f565e33c70d1e86b4838f2a6f318c356e834eef1b3cb83bb",
			"0xd54005db97678ec1d1048c5d10a9a1bce032473295983e56878e501ec68e25c958c3e3d2a09729fe0179f9dac9edcb0",
			"0x1778e7166fcc6db74e0609d307e55412d7f5e4656a8dbf25f1b33289f1b330835336e25ce3107193c5b388641d9b6861",
			"0xe99726a3199f4436642b4b3e4118e5499db995a1257fb3f086eeb65982fac18985a286f301e77c451154ce9ac8895d9",
			"0x1630c3250d7313ff01d1201bf7a74ab5db3cb17dd952799b9ed3ab9097e68f90a0870d2dcae73d19cd13c1c66f652983",
			"0xd6ed6553fe44d296a3726c38ae652bfb11586264f0f8ce19008e218f9c86b2a8da25128c1052ecaddd7f225a139ed84",
			"0x17b81e7701abdbe2e8743884d1117e53356de5ab275b4db1a682c62ef0f2753339b7c8f8c8f475af9ccb5618e3f0c88e",
			"0x80d3cf1f9a78fc47b90b33563be990dc43b756ce79f5574a2c596c928c5d1de4fa295f296b74e956d71986a8497e317",
			"0x169b1f8e1bcfa7c42e0c37515d138f22dd2ecb803a0c5c99676314baf4bb1b7fa3190b2edc0327797f241067be390c9e",
			"0x10321da079ce07e272d8ec09d2565b0dfa7dccdde6787f96d50af36003b14866f69b771f8c285decca67df3f1605fb7b",
			"0x6e08c248e260e70bd1e962381edee3d31d79d7e22c837bc23c0bf1bc24c6b68c24b1b80b64d391fa9c8ba2e8ba2d229",
		};
		const char *xdenStr[] = {
			"0x8ca8d548cff19ae18b2e62f4bd3fa6f01d5ef4ba35b48ba9c9588617fc8ac62b558d681be343df8993cf9fa40d21b1c",
			"0x12561a5deb559c4348b4711298e536367041e8ca0cf0800c0126c2588c48bf5713daa8846cb026e9e5c8276ec82b3bff",
			"0xb2962fe57a3225e8137e629bff2991f6f89416f5a718cd1fca64e00b11aceacd6a3d0967c94fedcfcc239ba5cb83e19",
			"0x3425581a58ae2fec83aafef7c40eb545b08243f16b1655154cca8abc28d6fd04976d5243eecf5c4130de8938dc62cd8",
			"0x13a8e162022914a80a6f1d5f43e7a07dffdfc759a12062bb8d6b44e833b306da9bd29ba81f35781d539d395b3532a21e",
			"0xe7355f8e4e667b955390f7f0506c6e9395735e9ce9cad4d0a43bcef24b8982f7400d24bc4228f11c02df9a29f6304a5",
			"0x772caacf16936190f3e0c63e0596721570f5799af53a1894e2e073062aede9cea73b3538f0de06cec2574496ee84a3a",
			"0x14a7ac2a9d64a8b230b3f5b074cf01996e7f63c21bca68a81996e1cdf9822c580fa5b9489d11e2d311f7d99bbdcc5a5e",
			"0xa10ecf6ada54f825e920b3dafc7a3cce07f8d1d7161366b74100da67f39883503826692abba43704776ec3a79a1d641",
			"0x95fc13ab9e92ad4476d6e3eb3a56680f682b4ee96f7d03776df533978f31c1593174e4b4b7865002d6384d168ecdd0a",
			"0x1",
		};
		const char *ynumStr[] = {
			"0x90d97c81ba24ee0259d1f094980dcfa11ad138e48a869522b52af6c956543d3cd0c7aee9b3ba3c2be9845719707bb33",
			"0x134996a104ee5811d51036d776fb46831223e96c254f383d0f906343eb67ad34d6c56711962fa8bfe097e75a2e41c696",
			"0xcc786baa966e66f4a384c86a3b49942552e2d658a31ce2c344be4b91400da7d26d521628b00523b8dfe240c72de1f6",
			"0x1f86376e8981c217898751ad8746757d42aa7b90eeb791c09e4a3ec03251cf9de405aba9ec61deca6355c77b0e5f4cb",
			"0x8cc03fdefe0ff135caf4fe2a21529c4195536fbe3ce50b879833fd221351adc2ee7f8dc099040a841b6daecf2e8fedb",
			"0x16603fca40634b6a2211e11db8f0a6a074a7d0d4afadb7bd76505c3d3ad5544e203f6326c95a807299b23ab13633a5f0",
			"0x4ab0b9bcfac1bbcb2c977d027796b3ce75bb8ca2be184cb5231413c4d634f3747a87ac2460f415ec961f8855fe9d6f2",
			"0x987c8d5333ab86fde9926bd2ca6c674170a05bfe3bdd81ffd038da6c26c842642f64550fedfe935a15e4ca31870fb29",
			"0x9fc4018bd96684be88c9e221e4da1bb8f3abd16679dc26c1e8b6e6a1f20cabe69d65201c78607a360370e577bdba587",
			"0xe1bba7a1186bdb5223abde7ada14a23c42a0ca7915af6fe06985e7ed1e4d43b9b3f7055dd4eba6f2bafaaebca731c30",
			"0x19713e47937cd1be0dfd0b8f1d43fb93cd2fcbcb6caf493fd1183e416389e61031bf3a5cce3fbafce813711ad011c132",
			"0x18b46a908f36f6deb918c143fed2edcc523559b8aaf0c2462e6bfe7f911f643249d9cdf41b44d606ce07c8a4d0074d8e",
			"0xb182cac101b9399d155096004f53f447aa7b12a3426b08ec02710e807b4633f06c851c1919211f20d4c04f00b971ef8",
			"0x245a394ad1eca9b72fc00ae7be315dc757b3b080d4c158013e6632d3c40659cc6cf90ad1c232a6442d9d3f5db980133",
			"0x5c129645e44cf1102a159f748c4a3fc5e673d81d7e86568d9ab0f5d396a7ce46ba1049b6579afb7866b1e715475224b",
			"0x15e6be4e990f03ce4ea50b3b42df2eb5cb181d8f84965a3957add4fa95af01b2b665027efec01c7704b456be69c8b604",
		};
		const char *ydenStr[] = {
			"0x16112c4c3a9c98b252181140fad0eae9601a6de578980be6eec3232b5be72e7a07f3688ef60c206d01479253b03663c1",
			"0x1962d75c2381201e1a0cbd6c43c348b885c84ff731c4d59ca4a10356f453e01f78a4260763529e3532f6102c2e49a03d",
			"0x58df3306640da276faaae7d6e8eb15778c4855551ae7f310c35a5dd279cd2eca6757cd636f96f891e2538b53dbf67f2",
			"0x16b7d288798e5395f20d23bf89edb4d1d115c5dbddbcd30e123da489e726af41727364f2c28297ada8d26d98445f5416",
			"0xbe0e079545f43e4b00cc912f8228ddcc6d19c9f0f69bbb0542eda0fc9dec916a20b15dc0fd2ededda39142311a5001d",
			"0x8d9e5297186db2d9fb266eaac783182b70152c65550d881c5ecd87b6f0f5a6449f38db9dfa9cce202c6477faaf9b7ac",
			"0x166007c08a99db2fc3ba8734ace9824b5eecfdfa8d0cf8ef5dd365bc400a0051d5fa9c01a58b1fb93d1a1399126a775c",
			"0x16a3ef08be3ea7ea03bcddfabba6ff6ee5a4375efa1f4fd7feb34fd206357132b920f5b00801dee460ee415a15812ed9",
			"0x1866c8ed336c61231a1be54fd1d74cc4f9fb0ce4c6af5920abc5750c4bf39b4852cfe2f7bb9248836b233d9d55535d4a",
			"0x167a55cda70a6e1cea820597d94a84903216f763e13d87bb5308592e7ea7d4fbc7385ea3d529b35e346ef48bb8913f55",
			"0x4d2f259eea405bd48f010a01ad2911d9c6dd039bb61a6290e591b36e636a5c871a5c29f4f83060400f8b49cba8f6aa8",
			"0xaccbb67481d033ff5852c1e48c50c477f94ff8aefce42d28c0f9a88cea7913516f968986f7ebbea9684b529e2561092",
			"0xad6b9514c767fe3c3613144b45f1496543346d98adf02267d5ceef9a00d9b8693000763e3b90ac11e99b138573345cc",
			"0x2660400eb2e4f3b628bdd0d53cd76f2bf565b94e72927c1cb748df27942480e420517bd8714cc80d1fadc1326ed06f7",
			"0xe0fa1d816ddc03e6b24255e0d7819c171c40f65e273b853324efcd6356caa205ca2f570f13497804415473a1d634b8f",
			"0x1",
		};
		initArray(g1xnum, xnumStr, CYBOZU_NUM_OF_ARRAY(xnumStr));
		initArray(g1xden, xdenStr, CYBOZU_NUM_OF_ARRAY(xdenStr));
		initArray(g1ynum, ynumStr, CYBOZU_NUM_OF_ARRAY(ynumStr));
		initArray(g1yden, ydenStr, CYBOZU_NUM_OF_ARRAY(ydenStr));
	}
	template<class F, size_t N>
	void evalPoly(F& y, const F& x, const F *zpows, const F (&cof)[N]) const
	{
		y = cof[N - 1]; // always zpows[0] = 1
		for (size_t i = 1; i < N; i++) {
			y *= x;
			F t;
			F::mul(t, zpows[i - 1], cof[N - 1 - i]);
			y += t;
		}
	}
	// refer (xnum, xden, ynum, yden)
	void iso3(G2& Q, const E2& P) const
	{
		Fp2 zpows[3];
		Fp2::sqr(zpows[0], P.z);
		Fp2::sqr(zpows[1], zpows[0]);
		Fp2::mul(zpows[2], zpows[1], zpows[0]);
		Fp2 mapvals[4];
		evalPoly(mapvals[0], P.x, zpows, xnum);
		evalPoly(mapvals[1], P.x, zpows, xden);
		evalPoly(mapvals[2], P.x, zpows, ynum);
		evalPoly(mapvals[3], P.x, zpows, yden);
		mapvals[1] *= zpows[0];
		mapvals[2] *= P.y;
		mapvals[3] *= zpows[0];
		mapvals[3] *= P.z;
		Fp2::mul(Q.z, mapvals[1], mapvals[3]);
		Fp2::mul(Q.x, mapvals[0], mapvals[3]);
		Q.x *= Q.z;
		Fp2 t;
		Fp2::sqr(t, Q.z);
		Fp2::mul(Q.y, mapvals[2], mapvals[1]);
		Q.y *= t;
	}
	template<class X, class C, size_t N>
	X evalPoly2(const X& x, const C (&c)[N]) const
	{
		X ret = c[N - 1];
		for (size_t i = 1; i < N; i++) {
			ret *= x;
			ret += c[N - 1 - i];
		}
		return ret;
	}
	// refer (g1xnum, g1xden, g1ynum, g1yden)
	void iso11(G1& Q, E1& P) const
	{
		ec::normalizeJacobi(P);
		Fp xn, xd, yn, yd;
		xn = evalPoly2(P.x, g1xnum);
		xd = evalPoly2(P.x, g1xden);
		yn = evalPoly2(P.x, g1ynum);
		yd = evalPoly2(P.x, g1yden);
		/*
			[xn/xd:y * yn/yd:1] = [xn xd yd^2:y yn xd^3 yd^2:xd yd]
			=[xn yd z:y yn xd z^2:z] where z = xd yd
		*/
		Fp::mul(Q.z, xd, yd);
		Fp::mul(Q.x, xn, yd);
		Q.x *= Q.z;
		Fp::mul(Q.y, P.y, yn);
		Q.y *= xd;
		Fp::sqr(xd, Q.z);
		Q.y *= xd;
	}
	/*
		xi = -2-i
		(a+bi)*(-2-i) = (b-2a)-(a+2b)i
	*/
	void mul_xi(Fp2& y, const Fp2& x) const
	{
		Fp t;
		Fp::sub(t, x.b, x.a);
		t -= x.a;
		Fp::add(y.b, x.b, x.b);
		y.b += x.a;
		Fp::neg(y.b, y.b);
		y.a = t;
	}
	bool isNegSign(const Fp& x) const
	{
		return x.isOdd();
	}
	bool isNegSign(const Fp2& x) const
	{
		bool sign0 = isNegSign(x.a);
		bool zero0 = x.a.isZero();
		bool sign1 = isNegSign(x.b);
		return sign0 || (zero0 & sign1);
	}
	// https://tools.ietf.org/html/draft-irtf-cfrg-hash-to-curve-07#appendix-D.3.5
	void sswuG1(Fp& xn, Fp& xd, Fp& y, const Fp& u) const
	{
		const Fp& A = E1::a_;
		const Fp& B = E1::b_;
		const Fp& c1 = g1c1;
		const Fp& c2 = g1c2;
		const int Z = g1Z;
		Fp u2, u2Z, t, t2, t3;

		Fp::sqr(u2, u);
		Fp::mulUnit(u2Z, u2, Z);
		Fp::sqr(t, u2Z);
		Fp::add(xd, t, u2Z);
		if (xd.isZero()) {
			Fp::mulUnit(xd, A, Z);
			xn = B;
		} else {
			Fp::add(xn, xd, Fp::one());
			xn *= B;
			xd *= A;
			Fp::neg(xd, xd);
		}
		Fp::sqr(t, xd);
		Fp::mul(t2, t, xd);
		t *= A;
		Fp::sqr(t3, xn);
		t3 += t;
		t3 *= xn;
		Fp::mul(t, t2, B);
		t3 += t;
		Fp::sqr(y, t2);
		Fp::mul(t, t3, t2);
		y *= t;
		Fp::pow(y, y, c1);
		y *= t;
		Fp::sqr(t, y);
		t *= t2;
		if (t != t3) {
			xn *= u2Z;
			y *= c2;
			y *= u2;
			y *= u;
		}
		if (isNegSign(u) != isNegSign(y)) {
			Fp::neg(y, y);
		}
	}
	void sswuG1(E1& pt, const Fp& u) const
	{
		Fp xn, y;
		Fp& xd = pt.z;
		sswuG1(xn, xd, y, u);
		Fp::mul(pt.x, xn, xd);
		Fp::sqr(pt.y, xd);
		pt.y *= xd;
		pt.y *= y;
	}
	// https://github.com/algorand/bls_sigs_ref
	void sswuG2(E2& P, const Fp2& t) const
	{
		Fp2 t2, t2xi;
		Fp2::sqr(t2, t);
		Fp2 den, den2;
		mul_xi(t2xi, t2);
		den = t2xi;
		Fp2::sqr(den2, den);
		// (t^2 * xi)^2 + (t^2 * xi)
		den += den2;
		Fp2 x0_num, x0_den;
		Fp2::add(x0_num, den, 1);
		x0_num *= E2::b_;
		if (den.isZero()) {
			mul_xi(x0_den, E2::a_);
		} else {
			Fp2::mul(x0_den, -E2::a_, den);
		}
		Fp2 x0_den2, x0_den3, gx0_den, gx0_num;
		Fp2::sqr(x0_den2, x0_den);
		Fp2::mul(x0_den3, x0_den2, x0_den);
		gx0_den = x0_den3;

		Fp2::mul(gx0_num, E2::b_, gx0_den);
		Fp2 tmp, tmp1, tmp2;
		Fp2::mul(tmp, E2::a_, x0_num);
		tmp *= x0_den2;
		gx0_num += tmp;
		Fp2::sqr(tmp, x0_num);
		tmp *= x0_num;
		gx0_num += tmp;

		Fp2::sqr(tmp1, gx0_den); // x^2
		Fp2::sqr(tmp2, tmp1); // x^4
		tmp1 *= tmp2;
		tmp1 *= gx0_den; // x^7
		Fp2::mul(tmp2, gx0_num, tmp1);
		tmp1 *= tmp2;
		tmp1 *= gx0_den;
		Fp2 candi;
		Fp2::pow(candi, tmp1, sqrtConst);
		candi *= tmp2;
		bool isNegT = isNegSign(t);
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(root4); i++) {
			Fp2::mul(P.y, candi, root4[i]);
			Fp2::sqr(tmp, P.y);
			tmp *= gx0_den;
			if (tmp == gx0_num) {
				if (isNegSign(P.y) != isNegT) {
					Fp2::neg(P.y, P.y);
				}
				Fp2::mul(P.x, x0_num, x0_den);
				P.y *= x0_den3;
				P.z = x0_den;
				return;
			}
		}
		Fp2 x1_num, x1_den, gx1_num, gx1_den;
		Fp2::mul(x1_num, t2xi, x0_num);
		x1_den = x0_den;
		Fp2::mul(gx1_num, den2, t2xi);
		gx1_num *= gx0_num;
		gx1_den = gx0_den;
		candi *= t2;
		candi *= t;
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(etas); i++) {
			Fp2::mul(P.y, candi, etas[i]);
			Fp2::sqr(tmp, P.y);
			tmp *= gx1_den;
			if (tmp == gx1_num) {
				if (isNegSign(P.y) != isNegT) {
					Fp2::neg(P.y, P.y);
				}
				Fp2::mul(P.x, x1_num, x1_den);
				Fp2::sqr(tmp, x1_den);
				P.y *= tmp;
				P.y *= x1_den;
				P.z = x1_den;
				return;
			}
		}
		assert(0);
	}
	template<class T>
	void put(const T& P) const
	{
		const int base = 10;
		printf("x=%s\n", P.x.getStr(base).c_str());
		printf("y=%s\n", P.y.getStr(base).c_str());
		printf("z=%s\n", P.z.getStr(base).c_str());
	}
	void Fp2ToG2(G2& P, const Fp2& t, const Fp2 *t2 = 0) const
	{
		E2 Pp;
		sswuG2(Pp, t);
		if (t2) {
			E2 P2;
			sswuG2(P2, *t2);
			ec::addJacobi(Pp, Pp, P2);
		}
		iso3(P, Pp);
		mcl::local::mulByCofactorBLS12fast(P, P);
	}
	void hashToFp2(Fp2 out[2], const void *msg, size_t msgSize, const void *dst, size_t dstSize) const
	{
		uint8_t md[256];
		mcl::fp::expand_message_xmd(md, sizeof(md), msg, msgSize, dst, dstSize);
		Fp *x = out[0].getFp0();
		for (size_t i = 0; i < 4; i++) {
			bool b;
			x[i].setBigEndianMod(&b, &md[64 * i], 64);
			assert(b); (void)b;
		}
	}
	void msgToG2(G2& out, const void *msg, size_t msgSize, const void *dst, size_t dstSize) const
	{
		Fp2 t[2];
		hashToFp2(t, msg, msgSize, dst, dstSize);
		Fp2ToG2(out, t[0], &t[1]);
	}
	void msgToG2(G2& out, const void *msg, size_t msgSize) const
	{
		msgToG2(out, msg, msgSize, dstG2.dst, dstG1.len);
	}
	void FpToG1(G1& out, const Fp& u0, const Fp *u1 = 0) const
	{
		E1 P1;
		sswuG1(P1, u0);
		if (u1) {
			E1 P2;
			sswuG1(P2, *u1);
			ec::addJacobi(P1, P1, P2);
		}
		iso11(out, P1);
		G1::mulGeneric(out, out, g1cofactor);
	}
	void msgToG1(G1& out, const void *msg, size_t msgSize, const char *dst, size_t dstSize) const
	{
		uint8_t md[128];
		mcl::fp::expand_message_xmd(md, sizeof(md), msg, msgSize, dst, dstSize);
		Fp u[2];
		for (size_t i = 0; i < 2; i++) {
			bool b;
			u[i].setBigEndianMod(&b, &md[64 * i], 64);
			assert(b); (void)b;
		}
		FpToG1(out, u[0], &u[1]);
	}

	void msgToG1(G1& out, const void *msg, size_t msgSize) const
	{
		msgToG1(out, msg, msgSize, dstG1.dst, dstG1.len);
	}
};

} // mcl

