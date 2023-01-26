/*
	tripartie Diffie-Hellman
*/
#include <iostream>
#include <fstream>
#include <cybozu/random_generator.hpp>
#include <mcl/bn256.hpp>
#include <cybozu/option.hpp>

static cybozu::RandomGenerator rg;

const std::string skSuf = ".sk.txt";
const std::string pkSuf = ".pk.txt";

using namespace mcl::bn256;

void keygen(const std::string& user)
{
	if (user.empty()) {
		throw cybozu::Exception("keygen:user is empty");
	}
	const char *aa = "12723517038133731887338407189719511622662176727675373276651903807414909099441";
	const char *ab = "4168783608814932154536427934509895782246573715297911553964171371032945126671";
	const char *ba = "13891744915211034074451795021214165905772212241412891944830863846330766296736";
	const char *bb = "7937318970632701341203597196594272556916396164729705624521405069090520231616";


	initPairing();
	G2 Q(Fp2(aa, ab), Fp2(ba, bb));
	G1 P(-1, 1);

	Fr s;
	s.setRand(rg);
	G1::mul(P, P, s);
	G2::mul(Q, Q, s);
	{
		std::string name = user + skSuf;
		std::ofstream ofs(name.c_str(), std::ios::binary);
		ofs << s << std::endl;
	}
	{
		std::string name = user + pkSuf;
		std::ofstream ofs(name.c_str(), std::ios::binary);
		ofs << P << std::endl;
		ofs << Q << std::endl;
	}
}

void load(G1& P, G2& Q, const std::string& fileName)
{
	std::ifstream ifs(fileName.c_str(), std::ios::binary);
	ifs >> P >> Q;
}

void share(const std::string& skFile, const std::string& pk1File, const std::string& pk2File)
{
	initPairing();
	Fr s;
	G1 P1, P2;
	G2 Q1, Q2;
	{
		std::ifstream ifs(skFile.c_str(), std::ios::binary);
		ifs >> s;
	}
	load(P1, Q1, pk1File);
	load(P2, Q2, pk2File);
	Fp12 e;
	pairing(e, P1, Q2);
	{
		// verify(not necessary)
		Fp12 e2;
		pairing(e2, P2, Q1);
		if (e != e2) {
			throw cybozu::Exception("share:bad public key file") << e << e2;
		}
	}
	Fp12::pow(e, e, s);
	std::cout << "share key:\n" << e << std::endl;
}

int main(int argc, char *argv[])
	try
{
	if (argc == 3 && strcmp(argv[1], "keygen") == 0) {
		keygen(argv[2]);
	} else if (argc == 5 && strcmp(argv[1], "share") == 0) {
		share(argv[2], argv[3], argv[4]);
	} else {
		fprintf(stderr, "tri-dh.exe keygen <user name>\n");
		fprintf(stderr, "tri-dh.exe share <secret key file> <public key1 file> <public key2 file>\n");
		return 1;
	}
} catch (std::exception& e) {
	printf("ERR %s\n", e.what());
	return 1;
}

