#include "core.h"
#include "coins.h"
#include "rpcserver.h"

#include <boost/unordered_set.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/functional/hash.hpp>
#include <boost/tokenizer.hpp>

#ifndef REDLIST_URL
#   define REDLIST_URL "https://redlist.url"
#endif

#ifndef REDLIST_SWITCH_THRESHOLD
#   define REDLIST_SWITCH_THRESHOLD 2             //2 blocks
#endif

using namespace std;
using namespace boost;

struct string_ihash: std::unary_function<std::basic_string<char>, std::size_t>
{
    std::size_t operator()(std::basic_string<char> const& x) const {
        boost::hash<std::basic_string<char> > hasher;
        std::basic_string<char> copy = std::basic_string<char>(x);
        boost::to_lower(copy);
        return hasher(copy);
    }
};

struct string_iequals: std::binary_function<std::basic_string<char>, std::basic_string<char>, bool>
{
    bool operator()(std::basic_string<char> const& x, std::basic_string<char> const& y) const {
        return boost::iequals<std::basic_string<char>, std::basic_string<char> >(x, y);
    }
};

/*
 * Unordered list storing the actual red list.
 * Function contains(x) is approximately O(1).
 */
typedef boost::unordered_set<
    std::basic_string<char>,
    string_ihash,
    string_iequals
> RedList;

/*
 * Checks if the given hashPubKey is contained in the red list.
 */
bool IsRedlistedPubKey(RedList* rlist, std::basic_string<char> hashPubKey);
bool IsRedlistedBlock(const CBlock& block, CCoinsViewCache* view);
bool IsRedlistedTransaction(const CTransaction& tx, CCoinsViewCache* view);
bool IsRedlistedScriptPubKey(RedList* rlist, CScript scriptPubKey);
bool IsRedlistedScriptSig(RedList* rlist, CScript scriptPubKey);

void redlistAddress(std::basic_string<char> address);
void resetRedlist();

/* Builds a new redlist from the redlist source
 */
RedList* buildRedList();

/* Get a redlist.
 * Will not rebuild it from source if it exists and is not outdated.
 */
RedList* getRedList();

/* Force an update of the global redlist
 */
bool updateRedList();

uint256 GetWorkSwitchThreshold();
