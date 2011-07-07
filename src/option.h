// Copyright (c) 2009-2011 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OPTION_H
#define BITCOIN_OPTION_H

// Possible uses of
//
//  option<T> fooOpt(
//      "group", "class", "foo", default_val, [implicit_val,]
//      "=<num>", _("Description of this option"));
//
// are
//
//  T value = fooOpt();         (get the current value)
//  T value = fooOpt[default];  (use a special case default value)
//  if (+fooOpt) { ... }        (was the option explicitely defined by the user)
//
// and the help message shows:
//
//  Options:
//    -foo=<num>        Description of this option

class money
{
private:
    int64 amount;
public:
    money() { amount = 0; }
    money(int64 v) { amount = v; }
    operator int64() const { return amount; }
};


template <typename T>
class option {
private:
    bool defined;  // Was this option explicitely defined?
    T value;       // Current value
    void notify(const T &);
public:
    option(const char *optGroup, const char *optClass,
           const char *optName, const T &defaultValue,
           const char *argDesc = NULL, const char *optDesc = NULL);
    option(const char *optGroup, const char *optClass,
           const char *optName, const T &def, const T &imp,
           const char *argDesc = NULL, const char *optDesc = NULL);

    bool operator+() const { return defined; }
    const T operator() () { return value; }
    const T operator[] (const T &d) { return defined ? value : d; }
};

extern template class option<bool>;
extern template class option<std::string>;
extern template class option<int>;
extern template class option<int64>;
extern template class option<money>;
extern template class option<std::vector<std::string> >;

std::string GetOptionsDescriptions(const std::string &groupMasks, const std::string &classMasks, bool showgroups=false);

bool ParseCommandLine(int argc, char **argv);
bool ParseConfigFile();

#endif
