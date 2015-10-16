#ifndef BITCOIN_HERNELWORKER_H
#define BITCOIN_HERNELWORKER_H

class KernelWorker
{
public:
    KernelWorker()
    { }
    KernelWorker(unsigned char *kernel, uint32_t nBits, uint32_t nInputTxTime, int64_t nValueIn, uint32_t nIntervalBegin, uint32_t nIntervalEnd);
    void Do();
    vector<std::pair<uint256,uint32_t> >& GetSolutions();

private:
#ifdef USE_ASM
#ifdef __x86_64__
    // AVX2 CPUs: 8-way hashing.
    void Do_8way();
#endif
    // SSE2, Neon: 4-way hashing.
    void Do_4way();
#endif
    // One way hashing.
    void Do_generic();

    // Kernel solutions.
    std::vector<std::pair<uint256,uint32_t> > solutions;

    // Kernel metadaya
    uint8_t *kernel;
    uint32_t nBits;
    uint32_t nInputTxTime;
    CBigNum  bnValueIn;

    // Interval boundaries.
    uint32_t nIntervalBegin;
    uint32_t nIntervalEnd;
};

#endif
