#include <mcl/fp.hpp>
#include <mcl/gmp_util.hpp>
#include <mcl/ecparam.hpp>
#include <cybozu/random_generator.hpp>
#include <map>
#include <mcl/fp.hpp>
typedef mcl::FpT<> Fp;

typedef std::map<std::string, int> Map;

int main(int argc, char *argv[])
{
	cybozu::RandomGenerator rg;
	const char *p = mcl::ecparam::secp192k1.p;
	if (argc == 2) {
		p = argv[1];
	}
	Fp::init(p);
	Fp x;
	printf("p=%s\n", p);
	Map m;
	for (int i = 0; i < 10000; i++) {
		x.setRand(rg);
		m[x.getStr(16)]++;
	}
	for (Map::const_iterator i = m.begin(), ie = m.end(); i != ie; ++i) {
		printf("%s %d\n", i->first.c_str(), i->second);
	}
}
