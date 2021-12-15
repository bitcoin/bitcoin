load("secp256k1_params.sage")

orders_done = set()
results = {}
first = True
for b in range(1, P):
    # There are only 6 curves (up to isomorphism) of the form y^2=x^3+B. Stop once we have tried all.
    if len(orders_done) == 6:
        break

    E = EllipticCurve(F, [0, b])
    print("Analyzing curve y^2 = x^3 + %i" % b)
    n = E.order()
    # Skip curves with an order we've already tried
    if n in orders_done:
        print("- Isomorphic to earlier curve")
        continue
    orders_done.add(n)
    # Skip curves isomorphic to the real secp256k1
    if n.is_pseudoprime():
        print(" - Isomorphic to secp256k1")
        continue

    print("- Finding subgroups")

    # Find what prime subgroups exist
    for f, _ in n.factor():
        print("- Analyzing subgroup of order %i" % f)
        # Skip subgroups of order >1000
        if f < 4 or f > 1000:
            print("  - Bad size")
            continue

        # Iterate over X coordinates until we find one that is on the curve, has order f,
        # and for which curve isomorphism exists that maps it to X coordinate 1.
        for x in range(1, P):
            # Skip X coordinates not on the curve, and construct the full point otherwise.
            if not E.is_x_coord(x):
                continue
            G = E.lift_x(F(x))

            print("  - Analyzing (multiples of) point with X=%i" % x)

            # Skip points whose order is not a multiple of f. Project the point to have
            # order f otherwise.
            if (G.order() % f):
                print("    - Bad order")
                continue
            G = G * (G.order() // f)

            # Find lambda for endomorphism. Skip if none can be found.
            lam = None
            for l in Integers(f)(1).nth_root(3, all=True):
                if int(l)*G == E(BETA*G[0], G[1]):
                    lam = int(l)
                    break
            if lam is None:
                print("    - No endomorphism for this subgroup")
                break

            # Now look for an isomorphism of the curve that gives this point an X
            # coordinate equal to 1.
            # If (x,y) is on y^2 = x^3 + b, then (a^2*x, a^3*y) is on y^2 = x^3 + a^6*b.
            # So look for m=a^2=1/x.
            m = F(1)/G[0]
            if not m.is_square():
                print("    - No curve isomorphism maps it to a point with X=1")
                continue
            a = m.sqrt()
            rb = a^6*b
            RE = EllipticCurve(F, [0, rb])

            # Use as generator twice the image of G under the above isormorphism.
            # This means that generator*(1/2 mod f) will have X coordinate 1.
            RG = RE(1, a^3*G[1]) * 2
            # And even Y coordinate.
            if int(RG[1]) % 2:
                RG = -RG
            assert(RG.order() == f)
            assert(lam*RG == RE(BETA*RG[0], RG[1]))

            # We have found curve RE:y^2=x^3+rb with generator RG of order f. Remember it
            results[f] = {"b": rb, "G": RG, "lambda": lam}
            print("    - Found solution")
            break

    print("")

print("")
print("")
print("/* To be put in src/group_impl.h: */")
first = True
for f in sorted(results.keys()):
    b = results[f]["b"]
    G = results[f]["G"]
    print("#  %s EXHAUSTIVE_TEST_ORDER == %i" % ("if" if first else "elif", f))
    first = False
    print("static const secp256k1_ge secp256k1_ge_const_g = SECP256K1_GE_CONST(")
    print("    0x%08x, 0x%08x, 0x%08x, 0x%08x," % tuple((int(G[0]) >> (32 * (7 - i))) & 0xffffffff for i in range(4)))
    print("    0x%08x, 0x%08x, 0x%08x, 0x%08x," % tuple((int(G[0]) >> (32 * (7 - i))) & 0xffffffff for i in range(4, 8)))
    print("    0x%08x, 0x%08x, 0x%08x, 0x%08x," % tuple((int(G[1]) >> (32 * (7 - i))) & 0xffffffff for i in range(4)))
    print("    0x%08x, 0x%08x, 0x%08x, 0x%08x" % tuple((int(G[1]) >> (32 * (7 - i))) & 0xffffffff for i in range(4, 8)))
    print(");")
    print("static const secp256k1_fe secp256k1_fe_const_b = SECP256K1_FE_CONST(")
    print("    0x%08x, 0x%08x, 0x%08x, 0x%08x," % tuple((int(b) >> (32 * (7 - i))) & 0xffffffff for i in range(4)))
    print("    0x%08x, 0x%08x, 0x%08x, 0x%08x" % tuple((int(b) >> (32 * (7 - i))) & 0xffffffff for i in range(4, 8)))
    print(");")
print("#  else")
print("#    error No known generator for the specified exhaustive test group order.")
print("#  endif")

print("")
print("")
print("/* To be put in src/scalar_impl.h: */")
first = True
for f in sorted(results.keys()):
    lam = results[f]["lambda"]
    print("#  %s EXHAUSTIVE_TEST_ORDER == %i" % ("if" if first else "elif", f))
    first = False
    print("#    define EXHAUSTIVE_TEST_LAMBDA %i" % lam)
print("#  else")
print("#    error No known lambda for the specified exhaustive test group order.")
print("#  endif")
print("")
