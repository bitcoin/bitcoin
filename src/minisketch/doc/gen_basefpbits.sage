# Require exact values up to
FPBITS = 256

# Overkill accuracy
F = RealField(400)

def BaseFPBits(bits, capacity):
    return bits * capacity - int(ceil(F(log(sum(binomial(2**bits - 1, i) for i in range(capacity+1)), 2))))

def Log2Factorial(capacity):
    return int(floor(log(factorial(capacity), 2)))

print("uint64_t BaseFPBits(uint32_t bits, uint32_t capacity) {")
print("    // Correction table for low bits/capacities")
TBLS={}
FARS={}
SKIPS={}
for bits in range(1, 32):
    TBL = []
    for capacity in range(1, min(2**bits, FPBITS)):
        exact = BaseFPBits(bits, capacity)
        approx = Log2Factorial(capacity)
        TBL.append((exact, approx))
    MIN = 10000000000
    while len(TBL) and ((TBL[-1][0] == TBL[-1][1]) or (TBL[-1][0] >= FPBITS and TBL[-1][1] >= FPBITS)):
        MIN = min(MIN, TBL[-1][0] - TBL[-1][1])
        TBL.pop()
    while len(TBL) and (TBL[-1][0] - TBL[-1][1] == MIN):
        TBL.pop()
    SKIP = 0
    while SKIP < len(TBL) and TBL[SKIP][0] == TBL[SKIP][1]:
        SKIP += 1
    DIFFS = [TBL[i][0] - TBL[i][1] for i in range(SKIP, len(TBL))]
    if len(DIFFS) > 0 and len(DIFFS) * Integer(max(DIFFS)).nbits() > 64:
        print("    static constexpr uint8_t ADD%i[] = {%s};" % (bits, ", ".join(("%i" % (TBL[i][0] - TBL[i][1])) for i in range(SKIP, len(TBL)))))
    TBLS[bits] = DIFFS
    FARS[bits] = MIN
    SKIPS[bits] = SKIP
print("")
print("    if (capacity == 0) return 0;")
print("    uint64_t ret = 0;")
print("    if (bits < 32 && capacity >= (1U << bits)) {")
print("        ret = uint64_t{bits} * (capacity - (1U << bits) + 1);")
print("        capacity = (1U << bits) - 1;")
print("    }")
print("    ret += Log2Factorial(capacity);")
print("    switch (bits) {")
for bits in sorted(TBLS.keys()):
    if len(TBLS[bits]) == 0:
        continue
    width = Integer(max(TBLS[bits])).nbits()
    if len(TBLS[bits]) == 1:
        add = "%i" % TBLS[bits][0]
    elif len(TBLS[bits]) * width <= 64:
        code = sum((2**(width*i) * TBLS[bits][i]) for i in range(len(TBLS[bits])))
        if width == 1:
            add = "(0x%x >> (capacity - %i)) & 1" % (code, 1 + SKIPS[bits])
        else:
            add = "(0x%x >> %i * (capacity - %i)) & %i" % (code, width, 1 + SKIPS[bits], 2**width - 1)
    else:
        add = "ADD%i[capacity - %i]" % (bits, 1 + SKIPS[bits])
    if len(TBLS[bits]) + SKIPS[bits] == 2**bits - 1:
        print("        case %i: return ret + (capacity <= %i ? 0 : %s);" % (bits, SKIPS[bits], add))
    else:
        print("        case %i: return ret + (capacity <= %i ? 0 : capacity > %i ? %i : %s);" % (bits, SKIPS[bits], len(TBLS[bits]) + SKIPS[bits], FARS[bits], add))
print("        default: return ret;")
print("    }")
print("}")

print("void TestBaseFPBits() {")
print("    static constexpr uint16_t TBL[20][100] = {%s};" % (", ".join("{" + ", ".join(("%i" % BaseFPBits(bits, capacity)) for capacity in range(0, 100)) + "}" for bits in range(1, 21))))
print("    for (int bits = 1; bits <= 20; ++bits) {")
print("        for (int capacity = 0; capacity < 100; ++capacity) {")
print("        uint64_t computed = BaseFPBits(bits, capacity), exact = TBL[bits - 1][capacity];")
print("            CHECK(exact == computed || (exact >= 256 && computed >= 256));")
print("        }")
print("    }")
print("}")
