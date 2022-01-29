#ifndef CACHE_POLICY_HPP
#define CACHE_POLICY_HPP

#include <unordered_set>

namespace caches
{

template <typename Key> class ICachePolicy
{
  public:
    virtual ~ICachePolicy()
    {
    }
    // handle element insertion in a cache
    virtual void Insert(const Key &key) = 0;
    // handle request to the key-element in a cache
    virtual void Touch(const Key &key) = 0;
    // handle element deletion from a cache
    virtual void Erase(const Key &key) = 0;

    // return a key of a replacement candidate
    virtual const Key &ReplCandidate() const = 0;
};

template <typename Key> class NoCachePolicy : public ICachePolicy<Key>
{
  public:
    NoCachePolicy() = default;
    ~NoCachePolicy() override = default;

    void Insert(const Key &key) override
    {
        key_storage.emplace(key);
    }

    void Touch(const Key &key) override
    {
        // do not do anything
    }

    void Erase(const Key &key) override
    {
        key_storage.erase(key);
    }

    // return a key of a displacement candidate
    const Key &ReplCandidate() const override
    {
        return *key_storage.cbegin();
    }

  private:
    std::unordered_set<Key> key_storage;
};
} // namespace caches

#endif // CACHE_POLICY_HPP
