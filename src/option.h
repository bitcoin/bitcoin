// Copyright (c) 2009-2011 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OPTION_H
#define BITCOIN_OPTION_H

/*

Possible uses of

 option<T> fooOpt(
     "group", option_class, "foo", default_val, [implicit_val,]
     "=<num>", _("Description of this option"));

are

 T value = fooOpt();         (get the current value)
 T value = fooOpt[default];  (use a special case default value)
 if (+fooOpt) { ... }        (has the option been specified by the user)

Τhe user can assign a value to an option through the command line
(-foo=value) or from the configuration file (foo=value). An option
can be specified without an explicit value if an implicit_val field is
provided in its definition.

The help message for this option is formatted as follows:

 Options:
   -foo=<num>        Description of this option

In-use option_classes are:
  "show": may appear on main help message
  "hide": listed only using -help=allopts
  "test" or other: never listed

*/

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

std::string GetOptionsDescriptions(
    const std::string &groupMasks, const std::string &classMasks,
    bool showgroups=false);

bool ParseCommandLine(int argc, char **argv, std::string &strErrors);
bool ParseConfigFile(std::string &strErrors);

#endif
