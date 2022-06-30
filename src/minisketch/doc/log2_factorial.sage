import bisect

INPUT_BITS = 32
TABLE_BITS = 5
INT_BITS = 64
EXACT_FPBITS = 256

F = RealField(100) # overkill

def BestOverApproxInvLog2(mulof, maxd):
    """
    Compute denominator of an approximation of 1/log(2).

    Specifically, find the value of d (<= maxd, and a multiple of mulof)
    such that ceil(d/log(2))/d is the best approximation of 1/log(2).
    """
    dist=1
    best=0
    # Precomputed denominators that lead to good approximations of 1/log(2)
    for d in [1, 2, 9, 70, 131, 192, 445, 1588, 4319, 11369, 18419, 25469, 287209, 836158, 3057423, 8336111, 21950910, 35565709, 49180508, 161156323, 273132138, 385107953, 882191721]:
        kd = lcm(mulof, d)
        if kd <= maxd:
            n = ceil(kd / log(2))
            dis = F((n / kd) - 1 / log(2))
            if dis < dist:
                dist = dis
                best = kd
    return best


LOG2_TABLE = []
A = 0
B = 0
C = 0
D = 0
K = 0

def Setup(k):
    global LOG2_TABLE, A, B, C, D, K
    K = k
    LOG2_TABLE = []
    for i in range(2 ** TABLE_BITS):
        LOG2_TABLE.append(int(floor(F(K * log(1 + i / 2**TABLE_BITS, 2)))))

    # Maximum for (2*x+1)*LogK2(x)
    max_T = (2^(INPUT_BITS + 1) - 1) * (INPUT_BITS*K - 1)
    # Maximum for A
    max_A = (2^INT_BITS - 1) // max_T
    D = BestOverApproxInvLog2(2 * K, max_A * 2 * K)
    A = D // (2 * K)
    B = int(ceil(F(D/log(2))))
    C = int(floor(F(D*log(2*pi,2)/2)))

def LogK2(n):
    assert(n >= 1 and n < (1 << INPUT_BITS))
    bits = Integer(n).nbits()
    return K * (bits - 1) + LOG2_TABLE[((n << (INPUT_BITS - bits)) >> (INPUT_BITS - TABLE_BITS - 1)) - 2**TABLE_BITS]

def Log2Fact(n):
    # Use formula (A*(2*x+1)*LogK2(x) - B*x + C) / D
    return (A*(2*n+1)*LogK2(n) - B*n + C) // D + (n < 3)

RES = [int(F(log(factorial(i),2))) for i in range(EXACT_FPBITS * 10)]

best_worst_ratio = 0

for K in range(1, 10000):
    Setup(K)
    assert(LogK2(1) == 0)
    assert(LogK2(2) == K)
    assert(LogK2(4) == 2 * K)
    good = True
    worst_ratio = 1
    for i in range(1, EXACT_FPBITS * 10):
        exact = RES[i]
        approx = Log2Fact(i)
        if not (approx <= exact and ((approx == exact) or (approx >= EXACT_FPBITS and exact >= EXACT_FPBITS))):
            good = False
            break
        if worst_ratio * exact > approx:
            worst_ratio = approx / exact
    if good and worst_ratio > best_worst_ratio:
        best_worst_ratio = worst_ratio
        print("Formula: (%i*(2*x+1)*floor(%i*log2(x)) - %i*x + %i) / %i; log(max_ratio)=%f" % (A, K, B, C, D, RR(-log(worst_ratio))))
        print("LOG2K_TABLE: %r" % LOG2_TABLE)
