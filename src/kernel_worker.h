#ifndef NOVACOIN_KERNELWORKER_H
#define NOVACOIN_KERNELWORKER_H

#include <vector>

class KernelWorker
{
public:
    KernelWorker()
    { }
    KernelWorker(unsigned char *kernel, uint32_t nBits, uint32_t nInputTxTime, int64_t nValueIn, uint32_t nIntervalBegin, uint32_t nIntervalEnd);
    void Do();
    std::vector<std::pair<uint256,uint32_t> >& GetSolutions();

private:
    // One way hashing.
    void Do_generic();

    // Kernel solutions.
    std::vector<std::pair<uint256,uint32_t> > solutions;

    // Kernel metadata.
    uint8_t *kernel;
    uint32_t nBits;
    uint32_t nInputTxTime;
    CBigNum  bnValueIn;

    // Interval boundaries.
    uint32_t nIntervalBegin;
    uint32_t nIntervalEnd;
};

// Scan given kernel for solutions
bool ScanKernelBackward(unsigned char *kernel, uint32_t nBits, uint32_t nInputTxTime, int64_t nValueIn, std::pair<uint32_t, uint32_t> &SearchInterval, std::pair<uint256, uint32_t> &solution);

#endif // NOVACOIN_KERNELWORKER_H