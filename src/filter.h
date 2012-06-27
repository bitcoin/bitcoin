// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __BITCOIN_FILTER_H__
#define __BITCOIN_FILTER_H__ 1

#include <string>
#include <map>

class CTransaction;
class CBlock;

class CFilter
{
private:
    std::map<std::string, bool> filter;

public:
    void insert(const std::string& data) { filter[data] = true; }

    bool contains(const std::string& data) const
    {
        return (filter.count(data) > 0);
    }
};

class CBloomers
{
private:
    std::map<std::string, CFilter> mapFilters;
public:
    void clearall()
    {
        mapFilters.clear();
    }

    void clear(const std::string& name)
    {
        mapFilters.erase(name);
    }

    void add(const std::string& name, const std::string& data)
    {
        mapFilters[name].insert(data);
    }

    void add(const std::string& name, std::vector<unsigned char> data_)
    {
        std::string data((char *)&data_[0], data_.size());
        add(name, data);
    }

    std::vector<std::string> contains(const std::string& data)
    {
        std::vector<std::string> retNames;
        for (std::map<std::string, CFilter>::const_iterator mi =
                    mapFilters.begin(); mi != mapFilters.end(); mi++) {
            if (mi->second.contains(data))
                retNames.push_back(mi->first);
        }
        return retNames;
    }
};

void FilterNotifyTx(const CTransaction& tx);
void FilterNotifyBlock(const CBlock& block);

#endif // __BITCOIN_FILTER_H__
