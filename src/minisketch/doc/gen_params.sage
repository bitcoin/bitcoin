#!/usr/bin/env sage
r"""
Generate finite field parameters for minisketch.

This script selects the finite fields used by minisketch
 for various sizes and generates the required tables for
 the implementation.

The output (after formatting) can be found in src/fields/*.cpp.

"""
B.<b> = GF(2)
P.<p> = B[]

def apply_map(m, v):
    r = 0
    i = 0
    while v != 0:
        if (v & 1):
            r ^^= m[i]
        i += 1
        v >>= 1
    return r

def recurse_moduli(acc, maxweight, maxdegree):
    for pos in range(maxweight, maxdegree + 1, 1):
        poly = acc + p^pos
        if maxweight == 1:
            if poly.is_irreducible():
                return (pos, poly)
        else:
            (deg, ret) = recurse_moduli(poly, maxweight - 1, pos - 1)
            if ret is not None:
                return (pos, ret)
    return (None, None)

def compute_moduli(bits):
    # Return all optimal irreducible polynomials for GF(2^bits)
    # The result is a list of tuples (weight, degree of second-highest nonzero coefficient, polynomial)
    maxdegree = bits - 1
    result = []
    for weight in range(1, bits, 2):
        deg, res = None, None
        while True:
            ret = recurse_moduli(p^bits + 1, weight, maxdegree)
            if ret[0] is not None:
                (deg, res) = ret
                maxdegree = deg - 1
            else:
                break
        if res is not None:
            result.append((weight + 2, deg, res))
    return result

def bits_to_int(vals):
    ret = 0
    base = 1
    for val in vals:
        ret += Integer(val) * base
        base *= 2
    return ret

def sqr_table(f, bits, n=1):
    ret = []
    for i in range(bits):
        ret.append((f^(2^n*i)).integer_representation())
    return ret

# Compute x**(2**n)
def pow2(x, n):
    for i in range(n):
        x = x**2
    return x

def qrt_table(F, f, bits):
    # Table for solving x2 + x = a
    # This implements the technique from https://www.raco.cat/index.php/PublicacionsMatematiques/article/viewFile/37927/40412, Lemma 1
    for i in range(bits):
        if (f**i).trace() != 0:
            u = f**i
    ret = []
    for i in range(0, bits):
        d = f^i
        y = sum(pow2(d, j) * sum(pow2(u, k) for k in range(j)) for j in range(1, bits))
        ret.append(y.integer_representation() ^^ (y.integer_representation() & 1))
    return ret

def conv_tables(F, NF, bits):
    # Generate a F(2) linear projection that maps elements from one field
    #  to an isomorphic field with a different modulus.
    f = F.gen()
    fp = f.minimal_polynomial()
    assert(fp == F.modulus())
    nfp = fp.change_ring(NF)
    nf = sorted(nfp.roots(multiplicities=False))[0]
    ret = []
    matrepr = [[B(0) for x in range(bits)] for y in range(bits)]
    for i in range(bits):
        val = (nf**i).integer_representation()
        ret.append(val)
        for j in range(bits):
            matrepr[j][i] = B((val >> j) & 1)
    mat = Matrix(matrepr).inverse().transpose()
    ret2 = []
    for i in range(bits):
        ret2.append(bits_to_int(mat[i]))

    for t in range(100):
        f1a = F.random_element()
        f1b = F.random_element()
        f1r = f1a * f1b
        f2a = NF.fetch_int(apply_map(ret, f1a.integer_representation()))
        f2b = NF.fetch_int(apply_map(ret, f1b.integer_representation()))
        f2r = NF.fetch_int(apply_map(ret, f1r.integer_representation()))
        f2s = f2a * f2b
        assert(f2r == f2s)

    for t in range(100):
        f2a = NF.random_element()
        f2b = NF.random_element()
        f2r = f2a * f2b
        f1a = F.fetch_int(apply_map(ret2, f2a.integer_representation()))
        f1b = F.fetch_int(apply_map(ret2, f2b.integer_representation()))
        f1r = F.fetch_int(apply_map(ret2, f2r.integer_representation()))
        f1s = f1a * f1b
        assert(f1r == f1s)

    return (ret, ret2)

def fmt(i,typ):
    if i == 0:
        return "0"
    else:
        return "0x%x" % i

def lintranstype(typ, bits, maxtbl):
    gsize = min(maxtbl, bits)
    array_size = (bits + gsize - 1) // gsize
    bits_list = []
    total = 0
    for i in range(array_size):
        rsize = (bits - total + array_size - i - 1) // (array_size - i)
        total += rsize
        bits_list.append(rsize)
    return "RecLinTrans<%s, %s>" % (typ, ", ".join("%i" % x for x in bits_list))

INT=0
CLMUL=1
CLMUL_TRI=2
MD=3

def print_modulus_md(mod):
    ret = ""
    pos = mod.degree()
    for c in reversed(list(mod)):
        if c:
            if ret:
                ret += " + "
            if pos == 0:
                ret += "1"
            elif pos == 1:
                ret += "x"
            else:
                ret += "x<sup>%i</sup>" % pos
        pos -= 1
    return ret

def pick_modulus(bits, style):
    # Choose the lexicographicly-first lowest-weight modulus
    #  optionally subject to implementation specific constraints.
    moduli = compute_moduli(bits)
    if style == INT or style == MD:
        multi_sqr = False
        need_trans = False
    elif style == CLMUL:
        # Fast CLMUL reduction requires that bits + the highest
        #  set bit are less than 66.
        moduli = list(filter((lambda x: bits+x[1] <= 66), moduli)) + moduli
        multi_sqr = True
        need_trans = True
        if not moduli or moduli[0][2].change_ring(ZZ)(2) == 3 + 2**bits:
            # For modulus 3, CLMUL_TRI is obviously better.
            return None
    elif style == CLMUL_TRI:
        moduli = list(filter(lambda x: bits+x[1] <= 66, moduli)) + moduli
        moduli = list(filter(lambda x: x[0] == 3, moduli))
        multi_sqr = True
        need_trans = True
    else:
        assert(False)
    if not moduli:
        return None
    return moduli[0][2]

def print_result(bits, style):
    if style == INT:
        multi_sqr = False
        need_trans = False
        table_id = "%i" % bits
    elif style == MD:
        pass
    elif style == CLMUL:
        multi_sqr = True
        need_trans = True
        table_id = "%i" % bits
    elif style == CLMUL_TRI:
        multi_sqr = True
        need_trans = True
        table_id = "TRI%i" % bits
    else:
        assert(False)

    nmodulus = pick_modulus(bits, INT)
    modulus = pick_modulus(bits, style)
    if modulus is None:
        return

    if style == MD:
        print("* *%s*" % print_modulus_md(modulus))
        return

    if bits > 32:
        typ = "uint64_t"
    elif bits > 16:
        typ = "uint32_t"
    elif bits > 8:
        typ = "uint16_t"
    else:
        typ = "uint8_t"

    ttyp = lintranstype(typ, bits, 4)
    rtyp = lintranstype(typ, bits, 6)

    F.<f> = GF(2**bits, modulus=modulus)

    include_table = True
    if style != INT and style != CLMUL:
        cmodulus = pick_modulus(bits, CLMUL)
        if cmodulus == modulus:
            include_table = False
            table_id = "%i" % bits

    if include_table:
        print("typedef %s StatTable%s;" % (rtyp, table_id))
        rtyp = "StatTable%s" % table_id
        if (style == INT):
            print("typedef %s DynTable%s;" % (ttyp, table_id))
            ttyp = "DynTable%s" % table_id

    if need_trans:
        if modulus != nmodulus:
            # If the bitstream modulus is not the best modulus for
            #  this implementation a conversion table will be needed.
            ctyp = rtyp
            NF.<nf> = GF(2**bits, modulus=nmodulus)
            ctables = conv_tables(NF, F, bits)
            loadtbl = "&LOAD_TABLE_%s" % table_id
            savetbl = "&SAVE_TABLE_%s" % table_id
            if include_table:
                print("constexpr %s LOAD_TABLE_%s({%s});" % (ctyp, table_id, ", ".join([fmt(x,typ) for x in ctables[0]])))
                print("constexpr %s SAVE_TABLE_%s({%s});" % (ctyp, table_id, ", ".join([fmt(x,typ) for x in ctables[1]])))
        else:
            ctyp = "IdTrans"
            loadtbl = "&ID_TRANS"
            savetbl = "&ID_TRANS"
    else:
        assert(modulus == nmodulus)

    if include_table:
        print("constexpr %s SQR_TABLE_%s({%s});" % (rtyp, table_id, ", ".join([fmt(x,typ) for x in sqr_table(f, bits, 1)])))
    if multi_sqr:
        # Repeated squaring is a linearised polynomial so in F(2^n) it is
        #  F(2) linear and can be computed by a simple bit-matrix.
        # Repeated squaring is especially useful in powering ladders such as
        #  for inversion.
        # When certain repeated squaring tables are not in use, use the QRT
        # table instead to make the C++ compiler happy (it always has the
        # same type).
        sqr2 = "&QRT_TABLE_%s" % table_id
        sqr4 = "&QRT_TABLE_%s" % table_id
        sqr8 = "&QRT_TABLE_%s" % table_id
        sqr16 = "&QRT_TABLE_%s" % table_id
        if ((bits - 1) >= 4):
            if include_table:
                print("constexpr %s SQR2_TABLE_%s({%s});" % (rtyp, table_id, ", ".join([fmt(x,typ) for x in sqr_table(f, bits, 2)])))
            sqr2 = "&SQR2_TABLE_%s" % table_id
        if ((bits - 1) >= 8):
            if include_table:
                print("constexpr %s SQR4_TABLE_%s({%s});" % (rtyp, table_id, ", ".join([fmt(x,typ) for x in sqr_table(f, bits, 4)])))
            sqr4 = "&SQR4_TABLE_%s" % table_id
        if ((bits - 1) >= 16):
            if include_table:
                print("constexpr %s SQR8_TABLE_%s({%s});" % (rtyp, table_id, ", ".join([fmt(x,typ) for x in sqr_table(f, bits, 8)])))
            sqr8 = "&SQR8_TABLE_%s" % table_id
        if ((bits - 1) >= 32):
            if include_table:
                print("constexpr %s SQR16_TABLE_%s({%s});" % (rtyp, table_id, ", ".join([fmt(x,typ) for x in sqr_table(f, bits, 16)])))
            sqr16 = "&SQR16_TABLE_%s" % table_id
    if include_table:
        print("constexpr %s QRT_TABLE_%s({%s});" % (rtyp, table_id, ", ".join([fmt(x,typ) for x in qrt_table(F, f, bits)])))

    modulus_weight = modulus.hamming_weight()
    modulus_degree = (modulus - p**bits).degree()
    modulus_int = (modulus - p**bits).change_ring(ZZ)(2)

    lfsr = ""

    if style == INT:
        print("typedef Field<%s, %i, %i, %s, %s, &SQR_TABLE_%s, &QRT_TABLE_%s%s> Field%i;" % (typ, bits, modulus_int, rtyp, ttyp, table_id, table_id, lfsr, bits))
    elif style == CLMUL:
        print("typedef Field<%s, %i, %i, %s, &SQR_TABLE_%s, %s, %s, %s, %s, &QRT_TABLE_%s, %s, %s, %s%s> Field%i;" % (typ, bits, modulus_int, rtyp, table_id, sqr2, sqr4, sqr8, sqr16, table_id, ctyp, loadtbl, savetbl, lfsr, bits))
    elif style == CLMUL_TRI:
        print("typedef FieldTri<%s, %i, %i, %s, &SQR_TABLE_%s, %s, %s, %s, %s, &QRT_TABLE_%s, %s, %s, %s> FieldTri%i;" % (typ, bits, modulus_degree, rtyp, table_id, sqr2, sqr4, sqr8, sqr16, table_id, ctyp, loadtbl, savetbl, bits))
    else:
        assert(False)

for bits in range(2, 65):
    print("#ifdef ENABLE_FIELD_INT_%i" % bits)
    print("// %i bit field" % bits)
    print_result(bits, INT)
    print("#endif")
    print("")

for bits in range(2, 65):
    print("#ifdef ENABLE_FIELD_INT_%i" % bits)
    print("// %i bit field" % bits)
    print_result(bits, CLMUL)
    print_result(bits, CLMUL_TRI)
    print("#endif")
    print("")

for bits in range(2, 65):
    print_result(bits, MD)
