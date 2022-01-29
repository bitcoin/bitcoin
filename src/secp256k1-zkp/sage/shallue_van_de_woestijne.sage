
### http://www.di.ens.fr/~fouque/pub/latincrypt12.pdf

# Parameters for secp256k1
p = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F
a = 0
b = 7
F = FiniteField (p)
C = EllipticCurve ([F(a), F(b)])

def svdw(t):
    sqrt_neg_3 = F(-3).nth_root(2)

    ## Compute candidate x values
    w  = sqrt_neg_3 * t / (1 + b + t^2)
    x = [ F(0), F(0), F(0) ]
    x[0] = (-1 + sqrt_neg_3) / 2 - t * w
    x[1] = -1 - x[0]
    x[2] = 1 + 1 / w^2

    print
    print "On %2d" % t
    print " x1 %064x" % x[0]
    print " x2 %064x" % x[1]
    print " x3 %064x" % x[2]

    ## Select which to use
    alph = jacobi_symbol(x[0]^3 + b, p)
    beta = jacobi_symbol(x[1]^3 + b, p)
    if alph == 1 and beta == 1:
        i = 0
    elif alph == 1 and beta == -1:
        i = 0
    elif alph == -1 and beta == 1:
        i = 1
    elif alph == -1 and beta == -1:
        i = 2
    else:
        print "Help! I don't understand Python!"

    ## Expand to full point
    sign = 1 - 2 * (int(F(t)) % 2)
    ret_x = x[i]
    ret_y = sign * F(x[i]^3 + b).nth_root(2)
    return C.point((ret_x, ret_y))


## main
for i in range(1, 11):
    res = svdw(i)
    print "Result: %064x %064x" % res.xy()
