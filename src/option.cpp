// Copyright (c) 2009-2011 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "option.h"

#include <string.h>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/lambda/lambda.hpp>

// This file is a custom Bitcoin-like binding to boost::program_options

using namespace std;
namespace po = boost::program_options;
namespace fs = boost::filesystem;


static void validate(
    boost::any& v, const std::vector<std::string>& values,
    money* target_type, int)
{
    po::validators::check_first_occurrence(v);
    const string& s = po::validators::get_single_string(values);
    int64 value;
    if (!ParseMoney(s, value))
        throw po::validation_error(po::validation_error::invalid_option_value);
    else
        v = boost::any(money(value));
}


typedef struct {
    const char *optGroup;
    const char *optClass;
    const char *optName;
    const char *argDesc;
    const char *optDesc;
} optattr;

// these stupid accessors are necessary to avoid
// the infamous "initialization disaster"
static vector<optattr> &allOptionsDesc()
{
    static vector<optattr> *p = new vector<optattr>;
    return *p;
}
static po::options_description &allOptions()
{
    static po::options_description *p = new po::options_description();
    return *p;
}

template <typename T>
void option<T>::notify(const T &val)
{
    value = val;
    defined = true;
}

template <typename T>
option<T>::option(
    const char *optGroup, const char *optClass,
    const char *optName, const T &def,
    const char *argDesc, const char *optDesc)
{
    defined = false;
    value = def;
    optattr d;
    d.optGroup = optGroup;
    d.optClass = optClass;
    d.optName = optName;
    d.argDesc = argDesc;
    d.optDesc = optDesc;
    allOptionsDesc().push_back(d);

    boost::function1<void, const T&> f =
        (boost::lambda::_1 ->* &option<T>::notify)(this);
    allOptions().add_options()(optName, po::value<T>(&value)->notifier(f));
}

template <typename T>
option<T>::option(
    const char *optGroup, const char *optClass,
    const char *optName, const T &def, const T &imp,
    const char *argDesc, const char *optDesc)
{
    defined = false;
    value = def;
    optattr d;
    d.optGroup = optGroup;
    d.optClass = optClass;
    d.optName = optName;
    d.argDesc = argDesc;
    d.optDesc = optDesc;
    allOptionsDesc().push_back(d);

    boost::function1<void, const T&> f =
        (boost::lambda::_1 ->* &option<T>::notify)(this);
    allOptions().add_options()
        (optName, po::value<T>(&value)->implicit_value(imp,"")->notifier(f));
}

template class option<bool>;
template class option<string>;
template class option<int>;
template class option<int64>;
template class option<money>;
template class option<vector<string> >;

static po::variables_map vmBase;

static pair<string, string> CustomOptionParser(const string& s)
{
    if (s.size() < 2 || !IsSwitchChar(s[0]) || s[1] == '-')
        return make_pair(string(), string());

    string::size_type p = s.find('=');
    if (p != string::npos)
        return make_pair(s.substr(1, p - 1), s.substr(p + 1, s.size() - p - 1));
    else
        return make_pair(s.substr(1), string());

}

bool ParseCommandLine(int argc, char **argv, string &strErrors)
{
    int optStyle =
        po::command_line_style::allow_long |
        po::command_line_style::long_allow_adjacent;

    po::command_line_parser parser(argc, argv);
    parser.extra_parser(CustomOptionParser);
    parser.style(optStyle);
    parser.options(allOptions());

    try
    {
        po::store(parser.run(), vmBase);
    }
    catch (po::error &e)
    {
        strErrors += string(_("Error on command line:")) + " " + e.what();
        return false;
    }
    po::notify(vmBase);
    return true;
}

bool ParseConfigFile(string &strErrors)
{
    fs::ifstream streamConfig(strConfigFile);
    po::variables_map vm = vmBase; // do not override cmd line
    try
    {
        po::store(po::parse_config_file(streamConfig, allOptions(), true), vm);
    }
    catch (po::error &e)
    {
        strErrors += string(_("Error in configuration file:")) + " " + e.what();
        return false;
    }
    po::notify(vm);
    return true;
}

static bool optdescSortGroup(optattr d1, optattr d2)
{
    return strcmp(d1.optGroup, d2.optGroup) < 0;
}

string GetOptionsDescriptions(
    const string &groupMasks, const string &classMasks, bool showgroups)
{
    int pad = 24;

    vector <string> vGroupMask;
    ParseString(groupMasks, ',', vGroupMask);
    vector <string> vClassMask;
    ParseString(classMasks, ',', vClassMask);

    vector<optattr> &v = allOptionsDesc();
    static bool sorted = false;
    if (!sorted)
    {
        stable_sort(v.begin(), v.end(), optdescSortGroup);
        sorted = true;
    }

    ostringstream ss;
    string prevgroup = "";
    bool groupdisplayed = false;

    BOOST_FOREACH(optattr d, v)
    {
        if (!d.optGroup || !d.optClass || !d.optName) continue;
        if (showgroups && prevgroup != d.optGroup)
        {
            groupdisplayed = false;
            prevgroup = d.optGroup;
        }
        bool group_match = false;
        BOOST_FOREACH(string mask, vGroupMask)
            if (WildcardMatch(d.optGroup, mask))
                group_match = true;
        bool class_match = false;
        BOOST_FOREACH(string mask, vClassMask)
            if (WildcardMatch(d.optClass, mask))
                class_match = true;
        if (group_match && class_match)
        {
            if (showgroups && !groupdisplayed)
            {
                ss << "(" << d.optGroup << ")" << endl;
                groupdisplayed = true;
            }
            string optUsage = string("  -") + d.optName;
            if (d.argDesc) optUsage += d.argDesc;
            if (optUsage.size() >= pad) optUsage += "  \t";
            else optUsage.resize(pad, ' ');
            if (d.optDesc) optUsage += string(" ") + d.optDesc;
            ss << optUsage << endl;
        }
    }

    return ss.str();
}
