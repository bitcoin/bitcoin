#include <netaddress.h>
#include <node/proxy.h>
#include <sync.h>

static GlobalMutex g_proxyinfo_mutex;
static Proxy proxyInfo[NET_MAX] GUARDED_BY(g_proxyinfo_mutex);
static Proxy nameProxy GUARDED_BY(g_proxyinfo_mutex);

bool SetProxy(enum Network net, const Proxy& addrProxy)
{
    assert(net >= 0 && net < NET_MAX);
    if (!addrProxy.IsValid())
        return false;
    LOCK(g_proxyinfo_mutex);
    proxyInfo[net] = addrProxy;
    return true;
}

bool GetProxy(enum Network net, Proxy& proxyInfoOut)
{
    assert(net >= 0 && net < NET_MAX);
    LOCK(g_proxyinfo_mutex);
    if (!proxyInfo[net].IsValid())
        return false;
    proxyInfoOut = proxyInfo[net];
    return true;
}

bool IsProxy(const CNetAddr& addr)
{
    LOCK(g_proxyinfo_mutex);
    for (int i = 0; i < NET_MAX; i++) {
        if (addr == static_cast<CNetAddr>(proxyInfo[i].proxy))
            return true;
    }
    return false;
}

bool SetNameProxy(const Proxy& addrProxy)
{
    if (!addrProxy.IsValid())
        return false;
    LOCK(g_proxyinfo_mutex);
    nameProxy = addrProxy;
    return true;
}

bool HaveNameProxy()
{
    LOCK(g_proxyinfo_mutex);
    return nameProxy.IsValid();
}

bool GetNameProxy(Proxy& nameProxyOut)
{
    LOCK(g_proxyinfo_mutex);
    if (!nameProxy.IsValid())
        return false;
    nameProxyOut = nameProxy;
    return true;
}
