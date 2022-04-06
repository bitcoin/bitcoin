#pragma once
/**
	@file
	@brief command line parser

	@author MITSUNARI Shigeo(@herumi)
*/
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cybozu/exception.hpp>
#include <cybozu/atoi.hpp>

/*
	Option parser

	progName (opt1-name|opt2-name|...) param1 param2 ...
	  param1:param1-help
	  param2:param2-help
	  -op1-name:opt1-help
	  ...

	How to setup
	int num;
	-n num ; (optional) option => appendOpt(&x, <defaultValue>, "num", "num-help");
	-n num ; must option       => appendMust(&x, "num", "num-help");

	std::vector<int> v;
	-v s1 s2 s3 ...            => appendVec(&v, "v");

	Remark1: terminate parsing of v if argv begins with '-[^0-9]'
	Remark2: the begining character of opt-name is not a number ('0'...'9')
	         because avoid conflict with minus number

	std::string file1;
	file1 is param             => appendParam(&file1, "input-file");
	file2 is optional param    => appendParamOpt(&file2, "output-file");

	How to use
	opt.parse(argc, argv);

	see sample/option_smpl.cpp
*/

namespace cybozu {

struct OptionError : public cybozu::Exception {
	enum Type {
		NoError = 0,
		BAD_OPT = 1,
		BAD_VALUE,
		NO_VALUE,
		OPT_IS_NECESSARY,
		PARAM_IS_NECESSARY,
		REDUNDANT_VAL,
		BAD_ARGC
	};
	Type type;
	int argPos;
	OptionError()
		: cybozu::Exception("OptionError", false)
		, type(NoError)
		, argPos(0)
	{
	}
	cybozu::Exception& set(Type _type, int _argPos = 0)
	{
		this->type = _type;
		this->argPos = _argPos;
		switch (_type) {
		case BAD_OPT:
			(*this) << "bad opt";
			break;
		case BAD_VALUE:
			(*this) << "bad value";
			break;
		case NO_VALUE:
			(*this) << "no value";
			break;
		case OPT_IS_NECESSARY:
			(*this) << "opt is necessary";
			break;
		case PARAM_IS_NECESSARY:
			(*this) << "param is necessary";
			break;
		case REDUNDANT_VAL:
			(*this) << "redundant argVal";
			break;
		case BAD_ARGC:
			(*this) << "bad argc";
		default:
			break;
		}
		return *this;
	}
};

namespace option_local {

template<class T>
bool convert(T* x, const char *str)
{
	std::istringstream is(str);
	is >> *x;
	return !!is;
}

template<>
inline bool convert(std::string* x, const char *str)
{
	*x = str;
	return true;
}

template<class T>
bool convertInt(T* x, const char *str)
{
	if (str[0] == '0' && str[1] == 'x') {
		bool b;
		*x = cybozu::hextoi(&b, str + 2);
		return b;
	}
	size_t len = strlen(str);
	int factor = 1;
	if (len > 1) {
		switch (str[len - 1]) {
		case 'k': factor = 1000; len--; break;
		case 'm': factor = 1000 * 1000; len--; break;
		case 'g': factor = 1000 * 1000 * 1000; len--; break;
		case 'K': factor = 1024; len--; break;
		case 'M': factor = 1024 * 1024; len--; break;
		case 'G': factor = 1024 * 1024 * 1024; len--; break;
		default: break;
		}
	}
	bool b;
	T y = cybozu::atoi(&b, str, len);
	if (!b) return false;
	if (factor > 1) {
		if ((std::numeric_limits<T>::min)() / factor <= y
			&& y <= (std::numeric_limits<T>::max)() / factor) {
			*x = static_cast<T>(y * factor);
		} else {
			return false;
		}
	} else {
		*x = y;
	}
	return true;
}

template<class T>
void convertToStr(std::ostream& os, const T* p)
{
	os << *p;
}
template<>inline void convertToStr(std::ostream& os, const int8_t* p)
{
	os << static_cast<int>(*p);
}
template<>inline void convertToStr(std::ostream& os, const uint8_t* p)
{
	os << static_cast<int>(*p);
}

#define CYBOZU_OPTION_DEFINE_CONVERT_INT(type) \
template<>inline bool convert(type* x, const char *str) { return convertInt(x, str); }

CYBOZU_OPTION_DEFINE_CONVERT_INT(int8_t)
CYBOZU_OPTION_DEFINE_CONVERT_INT(uint8_t)

CYBOZU_OPTION_DEFINE_CONVERT_INT(int16_t)
CYBOZU_OPTION_DEFINE_CONVERT_INT(uint16_t)

CYBOZU_OPTION_DEFINE_CONVERT_INT(int)
CYBOZU_OPTION_DEFINE_CONVERT_INT(long)
CYBOZU_OPTION_DEFINE_CONVERT_INT(long long)

CYBOZU_OPTION_DEFINE_CONVERT_INT(unsigned int)
CYBOZU_OPTION_DEFINE_CONVERT_INT(unsigned long)
CYBOZU_OPTION_DEFINE_CONVERT_INT(unsigned long long)

#undef CYBOZU_OPTION_DEFINE_CONVERT_INT

struct HolderBase {
	virtual ~HolderBase(){}
	virtual bool set(const char*) = 0;
	virtual HolderBase *clone() const = 0;
	virtual std::string toStr() const = 0;
	virtual const void *get() const = 0;
};

template<class T>
struct Holder : public HolderBase {
	T *p_;
	Holder(T *p) : p_(p) {}
	HolderBase *clone() const { return new Holder(p_); }
	bool set(const char *str) { return option_local::convert(p_, str); }
	std::string toStr() const
	{
		std::ostringstream os;
		convertToStr(os, p_);
		return os.str();
	}
	const void *get() const { return (void*)p_; }
};

/*
	for gcc 7 with -fnew-ttp-matching
	this specialization is not necessary under -fno-new-ttp-matching
*/
template struct Holder<std::string>;

template<class T, class Alloc, template<class T_, class Alloc_>class Container>
struct Holder<Container<T, Alloc> > : public HolderBase {
	typedef Container<T, Alloc> Vec;
	Vec *p_;
	Holder(Vec *p) : p_(p) {}
	HolderBase *clone() const { return new Holder<Vec>(p_); }
	bool set(const char *str)
	{
		T t;
		bool b = option_local::convert(&t, str);
		if (b) p_->push_back(t);
		return b;
	}
	std::string toStr() const
	{
		std::ostringstream os;
		bool isFirst = true;
		for (typename Vec::const_iterator i = p_->begin(), ie = p_->end(); i != ie; ++i) {
			if (isFirst) {
				isFirst = false;
			} else {
				os << ' ';
			}
			os << *i;
		}
		return os.str();
	}
	const void *get() const { return (void*)p_; }
};

class Var {
	HolderBase *p_;
	bool isSet_;
public:
	Var() : p_(0), isSet_(false) { }
	Var(const Var& rhs) : p_(rhs.p_->clone()), isSet_(false) { }
	template<class T>
	explicit Var(T *x) : p_(new Holder<T>(x)), isSet_(false) { }

	~Var() { delete p_; }

	void swap(Var& rhs) CYBOZU_NOEXCEPT
	{
		std::swap(p_, rhs.p_);
		std::swap(isSet_, rhs.isSet_);
	}
	void operator=(const Var& rhs)
	{
		Var v(rhs);
		swap(v);
	}
	bool set(const char *str)
	{
		isSet_ = true;
		return p_->set(str);
	}
	std::string toStr() const { return p_ ? p_->toStr() : ""; }
	bool isSet() const { return isSet_; }
	const void *get() const { return p_ ? p_->get() : 0; }
};

} // option_local

class Option {
	enum Mode { // for opt
		N_is0 = 0, // for bool by appendBoolOpt()
		N_is1 = 1,
		N_any = 2
	};
	enum ParamMode {
		P_exact = 0, // one
		P_optional = 1, // zero or one
		P_variable = 2 // zero or greater
	};
	struct Info {
		option_local::Var var;
		Mode mode; // 0 or 1 or any ; for opt, not used for Param
		bool isMust; // this option is must
		std::string opt; // option param name without '-'
		std::string help; // description of option

		Info() : mode(N_is0), isMust(false) {}
		template<class T>
		Info(T* pvar, Mode mode, bool isMust, const char *opt, const std::string& help)
			: var(pvar)
			, mode(mode)
			, isMust(isMust)
			, opt(opt)
			, help(help)
		{
		}
		friend inline std::ostream& operator<<(std::ostream& os, const Info& self)
		{
			os << self.opt << '=' << self.var.toStr();
			if (self.var.isSet()) {
				os << " (set)";
			} else {
				os << " (default)";
			}
			return os;
		}
		void put() const
		{
			std::cout << *this;
		}
		void usage() const
		{
			printf("  -%s %s%s\n", opt.c_str(), help.c_str(), isMust ? " (must)" : "");
		}
		void shortUsage() const
		{
			printf(" -%s %s", opt.c_str(), mode == N_is0 ? "" : mode == N_is1 ? "para" : "para...");
		}
		bool isSet() const { return var.isSet(); }
		const void *get() const { return var.get(); }
	};
	typedef std::vector<Info> InfoVec;
	typedef std::vector<std::string> StrVec;
	typedef std::map<std::string, size_t> OptMap;
	InfoVec infoVec_;
	InfoVec paramVec_;
	Info remains_;
	OptMap optMap_;
	bool showOptUsage_;
	ParamMode paramMode_;
	std::string progName_;
	std::string desc_;
	std::string helpOpt_;
	std::string help_;
	std::string usage_;
	StrVec delimiters_;
	StrVec *remainsAfterDelimiter_;
	int nextDelimiter_;
	template<class T>
	void appendSub(T *pvar, Mode mode, bool isMust, const char *opt, const std::string& help)
	{
		const char c = opt[0];
		if ('0' <= c && c <= '9') throw cybozu::Exception("Option::appendSub:opt must begin with not number") << opt;
		if (optMap_.find(opt) != optMap_.end()) {
			throw cybozu::Exception("Option::append:duplicate option") << opt;
		}
		optMap_[opt] = infoVec_.size();
		infoVec_.push_back(Info(pvar, mode, isMust, opt, help));
	}

	template<class T, class U>
	void append(T *pvar, const U& defaultVal, bool isMust, const char *opt, const std::string& help = "")
	{
		*pvar = static_cast<const T&>(defaultVal);
		appendSub(pvar, N_is1, isMust, opt, help);
	}
	/*
		don't deal with negative number as option
	*/
	bool isOpt(const char *str) const
	{
		if (str[0] != '-') return false;
		const char c = str[1];
		if ('0' <= c && c <= '9') return false;
		return true;
	}
	void verifyParamMode()
	{
		if (paramMode_ != P_exact) throw cybozu::Exception("Option:appendParamVec:appendParam is forbidden after appendParamOpt/appendParamVec");
	}
	std::string getBaseName(const std::string& name) const
	{
		size_t pos = name.find_last_of("/\\");
		if (pos == std::string::npos) return name;
		return name.substr(pos + 1);
	}
	bool inDelimiters(const std::string& str) const
	{
		return std::find(delimiters_.begin(), delimiters_.end(), str) != delimiters_.end();
	}
public:
	Option()
		: showOptUsage_(true)
		, paramMode_(P_exact)
		, remainsAfterDelimiter_(0)
		, nextDelimiter_(-1)
	{
	}
	virtual ~Option() {}
	/*
		append optional option with default value
		@param pvar [in] pointer to option variable
		@param defaultVal [in] default value
		@param opt [in] option name
		@param help [in] option help
		@note you can use 123k, 56M if T is int/long/long long
		k : *1000
		m : *1000000
		g : *1000000000
		K : *1024
		M : *1024*1024
		G : *1024*1024*1024
	*/
	template<class T, class U>
	void appendOpt(T *pvar, const U& defaultVal, const char *opt, const std::string& help = "")
	{
		append(pvar, defaultVal, false, opt, help);
	}
	/*
		default value of *pvar is false
	*/
	void appendBoolOpt(bool *pvar, const char *opt, const std::string& help = "")
	{
		*pvar = false;
		appendSub(pvar, N_is0, false, opt, help);
	}
	/*
		append necessary option
		@param pvar [in] pointer to option variable
		@param opt [in] option name
		@param help [in] option help
	*/
	template<class T>
	void appendMust(T *pvar, const char *opt, const std::string& help = "")
	{
		append(pvar, T(), true, opt, help);
	}
	/*
		append vector option
		@param pvar [in] pointer to option variable
		@param opt [in] option name
		@param help [in] option help
	*/
	template<class T, class Alloc, template<class T_, class Alloc_>class Container>
	void appendVec(Container<T, Alloc> *pvar, const char *opt, const std::string& help = "")
	{
		appendSub(pvar, N_any, false, opt, help);
	}
	/*
		append parameter
		@param pvar [in] pointer to parameter
		@param opt [in] option name
		@param help [in] option help
	*/
	template<class T>
	void appendParam(T *pvar, const char *opt, const std::string& help = "")
	{
		verifyParamMode();
		paramVec_.push_back(Info(pvar, N_is1, true, opt, help));
	}
	/*
		append optional parameter
		@param pvar [in] pointer to parameter
		@param defaultVal [in] default value
		@param opt [in] option name
		@param help [in] option help
		@note you can call appendParamOpt once after appendParam
	*/
	template<class T, class U>
	void appendParamOpt(T *pvar, const U& defaultVal, const char *opt, const std::string& help = "")
	{
		verifyParamMode();
		*pvar = defaultVal;
		paramMode_ = P_optional;
		paramVec_.push_back(Info(pvar, N_is1, false, opt, help));
	}
	/*
		append remain parameter
		@param pvar [in] pointer to vector of parameter
		@param opt [in] option name
		@param help [in] option help
		@note you can call appendParamVec once after appendParam
	*/
	template<class T, class Alloc, template<class T_, class Alloc_>class Container>
	void appendParamVec(Container<T, Alloc> *pvar, const char *name, const std::string& help = "")
	{
		verifyParamMode();
		paramMode_ = P_variable;
		remains_.var = option_local::Var(pvar);
		remains_.mode = N_any;
		remains_.isMust = false;
		remains_.opt = name;
		remains_.help = help;
	}
	void appendHelp(const char *opt, const std::string& help = ": show this message")
	{
		helpOpt_ = opt;
		help_ = help;
	}
	/*
		stop parsing after delimiter is found
		@param delimiter [in] string to stop
		@param remain [out] set remaining strings if remain
	*/
	void setDelimiter(const std::string& delimiter, std::vector<std::string> *remain = 0)
	{
		delimiters_.push_back(delimiter);
		remainsAfterDelimiter_ = remain;
	}
	/*
		stop parsing after delimiter is found
		@param delimiter [in] string to stop to append list of delimiters
	*/
	void appendDelimiter(const std::string& delimiter)
	{
		delimiters_.push_back(delimiter);
	}
	/*
		clear list of delimiters
	*/
	void clearDelimiterList() { delimiters_.clear(); }
	/*
		return the next position of delimiter between [0, argc]
		@note return argc if delimiter is not set nor found
	*/
	int getNextPositionOfDelimiter() const { return nextDelimiter_; }
	/*
		parse (argc, argv)
		@param argc [in] argc of main
		@param argv [in] argv of main
		@param startPos [in] start position of argc
		@param progName [in] used instead of argv[0]
	*/
	bool parse(int argc, const char *const argv[], int startPos = 1, const char *progName = 0)
	{
		if (argc < 1 || startPos > argc) return false;
		progName_ = getBaseName(progName ? progName : argv[startPos - 1]);
		nextDelimiter_ = argc;
		OptionError err;
		for (int pos = startPos; pos < argc; pos++) {
			if (inDelimiters(argv[pos])) {
				nextDelimiter_ = pos + 1;
				if (remainsAfterDelimiter_) {
					for (int i = nextDelimiter_; i < argc; i++) {
						remainsAfterDelimiter_->push_back(argv[i]);
					}
				}
                break;
			}
			if (isOpt(argv[pos])) {
				const std::string str = argv[pos] + 1;
				if (helpOpt_ == str) {
					usage();
					exit(0);
				}
				OptMap::const_iterator i = optMap_.find(str);
				if (i == optMap_.end()) {
					err.set(OptionError::BAD_OPT, pos);
					goto ERR;
				}

				Info& info = infoVec_[i->second];
				switch (info.mode) {
				case N_is0:
					if (!info.var.set("1")) {
						err.set(OptionError::BAD_VALUE, pos);
						goto ERR;
					}
					break;
				case N_is1:
					pos++;
					if (pos == argc) {
						err.set(OptionError::BAD_VALUE, pos) << (std::string("no value for -") + info.opt);
						goto ERR;
					}
					if (!info.var.set(argv[pos])) {
						err.set(OptionError::BAD_VALUE, pos) << (std::string(argv[pos]) + " for -" + info.opt);
						goto ERR;
					}
					break;
				case N_any:
				default:
					{
						pos++;
						int j = 0;
						while (pos < argc && !isOpt(argv[pos])) {
							if (!info.var.set(argv[pos])) {
								err.set(OptionError::BAD_VALUE, pos) << (std::string(argv[pos]) + " for -" + info.opt) << j;
								goto ERR;
							}
							pos++;
							j++;
						}
						if (j > 0) {
							pos--;
						} else {
							err.set(OptionError::NO_VALUE, pos) << (std::string("for -") + info.opt);
							goto ERR;
						}
					}
					break;
				}
			} else {
				bool used = false;
				for (size_t i = 0; i < paramVec_.size(); i++) {
					Info& param = paramVec_[i];
					if (!param.var.isSet()) {
						if (!param.var.set(argv[pos])) {
							err.set(OptionError::BAD_VALUE, pos) << (std::string(argv[pos]) + " for " + param.opt);
							goto ERR;
						}
						used = true;
						break;
					}
				}
				if (!used) {
					if (paramMode_ == P_variable) {
						remains_.var.set(argv[pos]);
					} else {
						err.set(OptionError::REDUNDANT_VAL, pos) << argv[pos];
						goto ERR;
					}
				}
			}
		}
		// check whether must-opt is set
		for (size_t i = 0; i < infoVec_.size(); i++) {
			const Info& info = infoVec_[i];
			if (info.isMust && !info.var.isSet()) {
				err.set(OptionError::OPT_IS_NECESSARY) << info.opt;
				goto ERR;
			}
		}
		// check whether param is set
		for (size_t i = 0; i < paramVec_.size(); i++) {
			const Info& param = paramVec_[i];
			if (param.isMust && !param.var.isSet()) {
				err.set(OptionError::PARAM_IS_NECESSARY) << param.opt;
				goto ERR;
			}
		}
		// check whether remains is set
		if (paramMode_ == P_variable && remains_.isMust && !remains_.var.isSet()) {
			err.set(OptionError::PARAM_IS_NECESSARY) << remains_.opt;
			goto ERR;
		}
		return true;
	ERR:
		assert(err.type);
		printf("%s\n", err.what());
		return false;
	}
	/*
		show desc at first in usage()
	*/
	void setDescription(const std::string& desc)
	{
		desc_ = desc;
	}
	/*
		show command line after desc
		don't put option message if not showOptUsage
	*/
	void setUsage(const std::string& usage, bool showOptUsage = false)
	{
		usage_ = usage;
		showOptUsage_ = showOptUsage;
	}
	void usage() const
	{
		if (!desc_.empty()) printf("%s\n", desc_.c_str());
		if (usage_.empty()) {
			printf("usage:%s", progName_.c_str());
			if (!infoVec_.empty()) printf(" [opt]");
			for (size_t i = 0; i < infoVec_.size(); i++) {
				if (infoVec_[i].isMust) infoVec_[i].shortUsage();
			}
			for (size_t i = 0; i < paramVec_.size(); i++) {
				printf(" %s", paramVec_[i].opt.c_str());
			}
			if (paramMode_ == P_variable) {
				printf(" %s", remains_.opt.c_str());
			}
			printf("\n");
		} else {
			printf("%s\n", usage_.c_str());
			if (!showOptUsage_) return;
		}
		for (size_t i = 0; i < paramVec_.size(); i++) {
			const Info& param = paramVec_[i];
			if (!param.help.empty()) printf("  %s %s\n", paramVec_[i].opt.c_str(), paramVec_[i].help.c_str());
		}
		if (!remains_.help.empty()) printf("  %s %s\n", remains_.opt.c_str(), remains_.help.c_str());
		if (!helpOpt_.empty()) {
			printf("  -%s %s\n", helpOpt_.c_str(), help_.c_str());
		}
		for (size_t i = 0; i < infoVec_.size(); i++) {
			infoVec_[i].usage();
		}
	}
	friend inline std::ostream& operator<<(std::ostream& os, const Option& self)
	{
		for (size_t i = 0; i < self.paramVec_.size(); i++) {
			const Info& param = self.paramVec_[i];
			os << param.opt << '=' << param.var.toStr() << std::endl;
		}
		if (self.paramMode_ == P_variable) {
			os << "remains=" << self.remains_.var.toStr() << std::endl;
		}
		for (size_t i = 0; i < self.infoVec_.size(); i++) {
			os << self.infoVec_[i] << std::endl;
		}
		return os;
	}
	void put() const
	{
		std::cout << *this;
	}
	/*
		whether pvar is set or not
	*/
	template<class T>
	bool isSet(const T* pvar) const
	{
		const void *p = static_cast<const void*>(pvar);
		for (size_t i = 0; i < paramVec_.size(); i++) {
			const Info& v = paramVec_[i];
			if (v.get() == p) return v.isSet();
		}
		if (remains_.get() == p) return remains_.isSet();
		for (size_t i = 0; i < infoVec_.size(); i++) {
			const Info& v = infoVec_[i];
			if (v.get() == p) return v.isSet();
		}
		throw cybozu::Exception("Option:isSet:no assigned var") << pvar;
	}
};

} // cybozu
