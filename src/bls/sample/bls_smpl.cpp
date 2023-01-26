#include <bls/bls384_256.h>
#include <bls/bls.hpp>
#include <cybozu/option.hpp>
#include <cybozu/itoa.hpp>
#include <fstream>

const std::string pubFile = "sample/publickey";
const std::string secFile = "sample/secretkey";
const std::string signFile = "sample/sign";

std::string makeName(const std::string& name, const bls::Id& id)
{
	const std::string suf = ".txt";
	if (id.isZero()) return name + suf;
	std::ostringstream os;
	os << name << '.' << id << suf;
	return os.str();
}

template<class T>
void save(const std::string& file, const T& t, const bls::Id& id = 0)
{
	const std::string name = makeName(file, id);
	std::ofstream ofs(name.c_str(), std::ios::binary);
	if (!(ofs << t)) {
		throw cybozu::Exception("can't save") << name;
	}
}

template<class T>
void load(T& t, const std::string& file, const bls::Id& id = 0)
{
	const std::string name = makeName(file, id);
	std::ifstream ifs(name.c_str(), std::ios::binary);
	if (!(ifs >> t)) {
		throw cybozu::Exception("can't load") << name;
	}
}

int init()
{
	printf("make %s and %s files\n", secFile.c_str(), pubFile.c_str());
	bls::SecretKey sec;
	sec.init();
	save(secFile, sec);
	bls::PublicKey pub;
	sec.getPublicKey(pub);
	save(pubFile, pub);
	return 0;
}

int sign(const std::string& m, int id)
{
	printf("sign message `%s` by id=%d\n", m.c_str(), id);
	bls::SecretKey sec;
	load(sec, secFile, id);
	bls::Signature s;
	sec.sign(s, m);
	save(signFile, s, id);
	return 0;
}

int verify(const std::string& m, int id)
{
	printf("verify message `%s` by id=%d\n", m.c_str(), id);
	bls::PublicKey pub;
	load(pub, pubFile, id);
	bls::Signature s;
	load(s, signFile, id);
	if (s.verify(pub, m)) {
		puts("verify ok");
		return 0;
	} else {
		puts("verify err");
		return 1;
	}
}

int share(size_t n, size_t k)
{
	printf("%d-out-of-%d threshold sharing\n", (int)k, (int)n);
	bls::SecretKey sec;
	load(sec, secFile);
	bls::SecretKeyVec msk;
	sec.getMasterSecretKey(msk, k);
	bls::SecretKeyVec secVec(n);
	bls::IdVec ids(n);
	for (size_t i = 0; i < n; i++) {
		int id = i + 1;
		ids[i] = id;
		secVec[i].set(msk, id);
	}
	for (size_t i = 0; i < n; i++) {
		save(secFile, secVec[i], ids[i]);
		bls::PublicKey pub;
		secVec[i].getPublicKey(pub);
		save(pubFile, pub, ids[i]);
	}
	return 0;
}

int recover(const bls::IdVec& ids)
{
	printf("recover from");
	for (size_t i = 0; i < ids.size(); i++) {
		std::cout << ' ' << ids[i];
	}
	printf("\n");
	bls::SignatureVec sigVec(ids.size());
	for (size_t i = 0; i < sigVec.size(); i++) {
		load(sigVec[i], signFile, ids[i]);
	}
	bls::Signature s;
	s.recover(sigVec, ids);
	save(signFile, s);
	return 0;
}

int main(int argc, char *argv[])
	try
{
	bls::init(); // use BN254

	std::string mode;
	std::string m;
	size_t n;
	size_t k;
	int id;
	bls::IdVec ids;

	cybozu::Option opt;
	opt.appendParam(&mode, "init|sign|verify|share|recover");
	opt.appendOpt(&n, 10, "n", ": k-out-of-n threshold");
	opt.appendOpt(&k, 3, "k", ": k-out-of-n threshold");
	opt.appendOpt(&m, "", "m", ": message to be signed");
	opt.appendOpt(&id, 0, "id", ": id of secretKey");
	opt.appendVec(&ids, "ids", ": select k id in [0, n). this option should be last");
	opt.appendHelp("h");
	if (!opt.parse(argc, argv)) {
		goto ERR_EXIT;
	}

	if (mode == "init") {
		return init();
	} else if (mode == "sign") {
		if (m.empty()) goto ERR_EXIT;
		return sign(m, id);
	} else if (mode == "verify") {
		if (m.empty()) goto ERR_EXIT;
		return verify(m, id);
	} else if (mode == "share") {
		return share(n, k);
	} else if (mode == "recover") {
		if (ids.empty()) {
			fprintf(stderr, "use -ids option. ex. share -ids 1 3 5\n");
			goto ERR_EXIT;
		}
		return recover(ids);
	} else {
		fprintf(stderr, "bad mode %s\n", mode.c_str());
	}
ERR_EXIT:
	opt.usage();
	return 1;
} catch (std::exception& e) {
	fprintf(stderr, "ERR %s\n", e.what());
	return 1;
}
