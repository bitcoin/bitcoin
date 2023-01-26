/*
	vote sample tool
	Copyright (c) 2014, National Institute of Advanced Industrial
	Science and Technology All rights reserved.
	This source file is subject to BSD 3-Clause license.

	modifyed for mcl by herumi
*/
#include <iostream>
#include <fstream>
#include <cybozu/random_generator.hpp>
#include <cybozu/option.hpp>
#include <cybozu/itoa.hpp>
#include <mcl/fp.hpp>
#include <mcl/ec.hpp>
#include <mcl/elgamal.hpp>
#include <mcl/ecparam.hpp>

typedef mcl::FpT<mcl::FpTag> Fp;
typedef mcl::FpT<mcl::ZnTag> Zn;
typedef mcl::EcT<Fp> Ec;
typedef mcl::ElgamalT<Ec, Zn> Elgamal;

cybozu::RandomGenerator rg;

const std::string pubFile = "vote_pub.txt";
const std::string prvFile = "vote_prv.txt";
const std::string resultFile = "vote_ret.txt";

std::string GetSheetName(size_t n)
{
	return std::string("vote_") + cybozu::itoa(n) + ".txt";
}

struct Param {
	std::string mode;
	std::string voteList;
	Param(int argc, const char *const argv[])
	{
		cybozu::Option opt;
		opt.appendOpt(&voteList, "11001100", "l", ": list of voters for vote mode(eg. 11001100)");
		opt.appendHelp("h", ": put this message");
		opt.appendParam(&mode, "mode", ": init/vote/count/open");
		if (!opt.parse(argc, argv)) {
			opt.usage();
			exit(1);
		}
		printf("mode=%s\n", mode.c_str());
		if (mode == "vote") {
			printf("voters=%s\n", voteList.c_str());
			size_t pos = voteList.find_first_not_of("01");
			if (pos != std::string::npos) {
				printf("bad char %c\n", voteList[pos]);
				exit(1);
			}
		}
	}
};

void SysInit()
{
	mcl::initCurve<Ec, Zn>(MCL_SECP192K1);
}

template<class T>
bool Load(T& t, const std::string& name, bool doThrow = true)
{
	std::ifstream ifs(name.c_str(), std::ios::binary);
	if (!ifs) {
		if (doThrow) throw cybozu::Exception("Load:can't read") << name;
		return false;
	}
	if (ifs >> t) return true;
	if (doThrow) throw cybozu::Exception("Load:bad data") << name;
	return false;
}

template<class T>
void Save(const std::string& name, const T& t)
{
	std::ofstream ofs(name.c_str(), std::ios::binary);
	ofs << t;
}

void Init()
{
	const mcl::EcParam& para = mcl::ecparam::secp192k1;
	const Fp x0(para.gx);
	const Fp y0(para.gy);
	const Ec P(x0, y0);
	const size_t bitSize = para.bitSize;

	Elgamal::PrivateKey prv;
	prv.init(P, bitSize, rg);
	const Elgamal::PublicKey& pub = prv.getPublicKey();
	printf("make privateKey=%s, publicKey=%s\n", prvFile.c_str(), pubFile.c_str());
	Save(prvFile, prv);
	Save(pubFile, pub);
}

struct CipherWithZkp {
	Elgamal::CipherText c;
	Elgamal::Zkp zkp;
	bool verify(const Elgamal::PublicKey& pub) const
	{
		return pub.verify(c, zkp);
	}
};

inline std::ostream& operator<<(std::ostream& os, const CipherWithZkp& self)
{
	return os << self.c << std::endl << self.zkp;
}
inline std::istream& operator>>(std::istream& is, CipherWithZkp& self)
{
	return is >> self.c >> self.zkp;
}

void Vote(const std::string& voteList)
{
	Elgamal::PublicKey pub;
	Load(pub, pubFile);
	puts("shuffle");
	std::vector<size_t> idxTbl(voteList.size());
	for (size_t i = 0; i < idxTbl.size(); i++) {
		idxTbl[i] = i;
	}
	cybozu::shuffle(idxTbl, rg);
	puts("each voter votes");
	for (size_t i = 0; i < voteList.size(); i++) {
		CipherWithZkp c;
		pub.encWithZkp(c.c, c.zkp, voteList[i] - '0', rg);
		const std::string sheetName = GetSheetName(idxTbl[i]);
		printf("make %s\n", sheetName.c_str());
		Save(sheetName, c);
	}
}

void Count()
{
	Elgamal::PublicKey pub;
	Load(pub, pubFile);
	Elgamal::CipherText result;
	puts("aggregate votes");
	for (size_t i = 0; ; i++) {
		const std::string sheetName = GetSheetName(i);
		CipherWithZkp c;
		if (!Load(c, sheetName, false)) break;
		if (!c.verify(pub)) throw cybozu::Exception("bad cipher text") << i;
		printf("add %s\n", sheetName.c_str());
		result.add(c.c);
	}
	printf("create result file : %s\n", resultFile.c_str());
	Save(resultFile, result);
}

void Open()
{
	Elgamal::PrivateKey prv;
	Load(prv, prvFile);
	Elgamal::CipherText c;
	Load(c, resultFile);
	Zn n;
	prv.dec(n, c);
	std::cout << "result of vote count " << n << std::endl;
#if 0
	puts("open real value");
	for (size_t i = 0; ; i++) {
		Elgamal::CipherText c;
		const std::string sheetName = GetSheetName(i);
		if (!Load(c, sheetName, false)) break;
		Zn n;
		prv.dec(n, c);
		std::cout << sheetName << " " << n << std::endl;
	}
#endif
}

int main(int argc, char *argv[])
	try
{
	const Param p(argc, argv);
	SysInit();
	if (p.mode == "init") {
		Init();
	} else
	if (p.mode == "vote") {
		Vote(p.voteList);
	} else
	if (p.mode == "count") {
		Count();
	} else
	if (p.mode == "open") {
		Open();
	} else
	{
		printf("bad mode=%s\n", p.mode.c_str());
		return 1;
	}
} catch (std::exception& e) {
	printf("ERR %s\n", e.what());
}

