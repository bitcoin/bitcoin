#include <support/allocators/node_allocator/memory_resource.h>

#include <memusage.h>

namespace node_allocator::detail {

size_t DynamicMemoryUsage(size_t pool_size_bytes, std::vector<std::unique_ptr<char[]>> const& allocated_pools)
{
    return memusage::MallocUsage(pool_size_bytes) * allocated_pools.size() + memusage::DynamicUsage(allocated_pools);
}

} // namespace node_allocator::detail
