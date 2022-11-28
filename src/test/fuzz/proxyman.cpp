#include <node/proxy.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util/net.h>

FUZZ_TARGET(proxyman)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    ProxyManager proxyman;

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                Proxy proxy{ConsumeService(fuzzed_data_provider), fuzzed_data_provider.ConsumeBool()};
                if (proxyman.SetProxy(proxy.proxy.GetNetClass(), proxy)) {
                    assert(proxyman.HasProxy(proxy.proxy));
                    auto _proxy{proxyman.GetProxy(proxy.proxy.GetNetClass())};
                    assert(_proxy && _proxy->IsValid());
                }
            },
            [&] {
                const Network network = fuzzed_data_provider.PickValueInArray(ALL_NETWORKS);
                auto proxy{proxyman.GetProxy(network)};
                assert(!proxy || proxy->IsValid());
                if (proxy) {
                    assert(proxyman.HasProxy(proxy->proxy));
                }
            },
            [&] {
                Proxy proxy{ConsumeService(fuzzed_data_provider), fuzzed_data_provider.ConsumeBool()};
                if (proxyman.HasProxy(proxy.proxy)) {
                    assert(proxy.IsValid());
                    auto _proxy{proxyman.GetProxy(proxy.proxy.GetNetClass())};
                    assert(_proxy && _proxy->IsValid());
                }
            },
            [&] {
                auto proxy_addr{ConsumeService(fuzzed_data_provider)};
                Proxy proxy{ConsumeService(fuzzed_data_provider), fuzzed_data_provider.ConsumeBool()};
                if (proxyman.SetNameProxy(proxy)) {
                    assert(proxy.IsValid());
                    assert(proxyman.HaveNameProxy());
                    auto name_proxy{proxyman.GetNameProxy()};
                    assert(name_proxy && name_proxy->IsValid());
                }
            });
    }
}
