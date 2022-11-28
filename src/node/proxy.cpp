#include <netaddress.h>
#include <node/proxy.h>
#include <sync.h>

bool ProxyManager::SetProxy(enum Network net, const Proxy& proxy)
{
    assert(net >= 0 && net < NET_MAX);
    if (!proxy.IsValid())
        return false;

    LOCK(m_mutex);
    m_proxies[net] = proxy;
    return true;
}

std::optional<Proxy> ProxyManager::GetProxy(enum Network net) const
{
    assert(net >= 0 && net < NET_MAX);
    LOCK(m_mutex);
    return m_proxies[net];
}

bool ProxyManager::HasProxy(const CNetAddr& addr) const
{
    // Invalid address can't be a proxy
    if (!addr.IsValid()) return false;

    LOCK(m_mutex);
    for (int i = 0; i < NET_MAX; i++) {
        if (addr == static_cast<CNetAddr>(m_proxies[i].value_or(Proxy{}).proxy))
            return true;
    }
    return false;
}

bool ProxyManager::SetNameProxy(const Proxy& proxy)
{
    if (!proxy.IsValid())
        return false;

    LOCK(m_mutex);
    m_name_proxy = proxy;
    return true;
}

bool ProxyManager::HaveNameProxy() const
{
    LOCK(m_mutex);
    return m_name_proxy.has_value();
}

std::optional<Proxy> ProxyManager::GetNameProxy() const
{
    LOCK(m_mutex);
    return m_name_proxy;
}
