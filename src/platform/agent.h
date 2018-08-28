#ifndef CROWN_PLATFORM_AGENT_H
#define CROWN_PLATFORM_AGENT_H

#include <memory>
#include <map>

#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/shared_ptr.hpp>

#include "uint256.h"

struct Identity
{
    uint256 id;

public:
    Identity(uint256 id)
        : id(id)
    {}
};

class AgentRegistryIteratorImpl;

class AgentRegistry
{
public:
    void Add(const Identity& agent);
    void Remove(const Identity& agent);
    bool IsAgent(uint256 id) const;

    AgentRegistry();

public:
    class Iterator:
        public boost::iterator_facade<Iterator, const Identity, boost::single_pass_traversal_tag>
    {
    public:
        typedef AgentRegistryIteratorImpl Impl;

        explicit Iterator(std::auto_ptr<Impl> impl);

    public:
        void increment();
        bool equal(const Iterator& other) const;
        const Identity& dereference() const;

    private:
        boost::shared_ptr<Impl> m_impl;
    };

    typedef Iterator iterator;
    typedef Iterator const_iterator;

public:
    const_iterator begin() const;
    const_iterator end() const;

private:
    std::map<uint256, Identity> m_agents;
};

AgentRegistry& GetAgentRegistry();


#endif //CROWN_PLATFORM_AGENT_H
