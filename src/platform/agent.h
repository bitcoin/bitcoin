#ifndef CROWN_PLATFORM_AGENT_H
#define CROWN_PLATFORM_AGENT_H

#include <memory>
#include <map>

#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "uint256.h"

namespace Platform
{
    class VotingRound;
    class ApprovalVoting;

    struct Identity
    {
        uint256 id;

    public:
        Identity(uint256 id)
            : id(id)
        {}
    };

    class AgentRegistryIteratorImpl;

    // TODO: After Identities are in place decide, whether we need AgentRegistry as a separate entity,
    //       or is it sufficient to have just a Voting Round to decide who is an agent and who is not
    class AgentRegistry
    {
    public:
        void Add(const Identity& agent);
        void Remove(const Identity& agent);
        bool IsAgent(uint256 id) const;

        AgentRegistry(std::unique_ptr<VotingRound> agentsVoting);

    public:
        class Iterator:
            public boost::iterator_facade<Iterator, const Identity, boost::single_pass_traversal_tag>
        {
        public:
            using Impl = AgentRegistryIteratorImpl;

            explicit Iterator(std::unique_ptr<Impl> impl);
            ~Iterator();

            Iterator(Iterator&&);
            Iterator& operator=(Iterator&&);

        public:
            void increment();
            bool equal(const Iterator& other) const;
            const Identity& dereference() const;

        private:
            std::unique_ptr<Impl> m_impl;
        };

        typedef Iterator iterator;
        typedef Iterator const_iterator;

    public:
        const_iterator begin() const;
        const_iterator end() const;

        VotingRound& AgentsVoting() { return *m_agentsVoting; }

    private:
        std::unique_ptr<VotingRound> m_agentsVoting;
        std::map<uint256, Identity> m_agents;
    };

    AgentRegistry& GetAgentRegistry();
    VotingRound& AgentsVoting();
}

#endif //CROWN_PLATFORM_AGENT_H
