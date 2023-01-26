/*
	make she DLP table
*/
#include <mcl/she.hpp>
#include <cybozu/option.hpp>
#include <fstream>

using namespace mcl::she;

struct Param {
	int curveType;
	int hashBitSize;
	int group;
	std::string path;
};

template<class HashTable, class G>
void makeTable(const Param& param, const char *groupStr, HashTable& hashTbl, const G& P)
{
	char baseName[32];
	CYBOZU_SNPRINTF(baseName, sizeof(baseName), "she-dlp-%d-%d-%s.bin", param.curveType, param.hashBitSize, groupStr);
	const std::string fileName = param.path + baseName;
	printf("file=%s\n", fileName.c_str());
	std::ofstream ofs(fileName.c_str(), std::ios::binary);

	const size_t hashSize = 1u << param.hashBitSize;
	hashTbl.init(P, hashSize);
	hashTbl.save(ofs);
}

void run(const Param& param)
{
	SHE::init(*mcl::getCurveParam(param.curveType));

	switch (param.group) {
	case 1:
		makeTable(param, "g1", getHashTableG1(), SHE::P_);
		break;
	case 2:
		makeTable(param, "g2", getHashTableG2(), SHE::Q_);
		break;
	case 3:
		makeTable(param, "gt", getHashTableGT(), SHE::ePQ_);
		break;
	default:
		throw cybozu::Exception("bad group") << param.group;
	}
}

int main(int argc, char *argv[])
	try
{
	cybozu::Option opt;
	Param param;
	opt.appendOpt(&param.curveType, 0, "ct", ": curveType(0:BN254, 1:BN381_1, 5:BLS12_381)");
	opt.appendOpt(&param.hashBitSize, 20, "hb", ": hash bit size");
	opt.appendOpt(&param.group, 3, "g", ": group(1:G1, 2:G2, 3:GT");
	opt.appendOpt(&param.path, "./", "path", ": path to table");
	opt.appendHelp("h");
	if (opt.parse(argc, argv)) {
		run(param);
	} else {
		opt.usage();
		return 1;
	}
} catch (std::exception& e) {
	printf("err %s\n", e.what());
	return 1;
}
