# Bech32_mod generator polynomial generation

## Summary
We made modification to bech32 implementation of Bitcoin so that it worked with 165-character bech32 string perfectly detecting up to 5 errors.

To accomplish that, we replaced the 6-degree generator polynomial originally used by bech32 by an 8-degree one as [Bitcoin Cash's cashaddr implementation](https://github.com/bitcoin-cash-node/bitcoin-cash-node/blob/master/src/cashaddr.cpp) and [Jamtis](https://gist.github.com/tevador/50160d160d24cfc6c52ae02eb3d17024) of Monero have done.

In order to find an 8-degree polynomial for our need, we followed the Jamtis polynomial search procedure which is explained in detail in [this document](https://gist.github.com/tevador/5b3fbbd0877a3412ede07263c6b2663d) with a little modificaiton to meet our requirements.

Here are the requirements we had:

1. The generator polynomial should be capable of perfectly detecting up to 5 errors in 165-character bech32 string.
   - We encode 96-byte double public keys into bech32 format. Converting 96-byte vector utilizing all bits of each byte into those that only uses 5 bits of each byte, we end up with 154-byte vector (96 * 8 / 5 = 153.6). In addition, 8-byte checksum, 2-byte HRP and 1-byte separator are needed. Putting those together, the resulting bech32 string became 165-character long.
2. The generator polynomial should have the lowest false-positive error rate for 7 and 8 error cases when the input string is 50-character long.

To find a polynomial satisfying above requirements, we first generated 10-million random degree-8 polynomials, and computed false positive error rates for them.

Amongst all, there were two generator polynomials satisfying the first requirement:

```
U1PIRGA7
AJ4RJKVB
```

For both of the polynomials, we computed false positive errors rates for 7 and 8 error cases, and we concluded that `U1PIRGA7` performed better and chose it as the generator polynomial.

## Actual procedure

### 1. Generation of random 10-million degree-8 polynomials
We used [gen_crc.py](https://gist.github.com/tevador/5b3fbbd0877a3412ede07263c6b2663d#:~:text=2.1-,gen_crc.py,-The%20gen_crc.py) used in Jamtis search that is shown below:

```python
# gen_crc.py

import random

CHARSET = "0123456789ABCDEFGHIJKLMNOPQRSTUV"

def gen_to_str(val, degree):
    gen_str = ""
    for i in range(degree):
        gen_str = CHARSET[int(val) % 32] + gen_str
        val /= 32
    return gen_str

def gen_crc(degree, count, seed=None):
    random.seed(seed)
    for i in range(count):
        while True:
            r = random.getrandbits(5 * degree)
            if (r % 32) != 0:
                break
        print(gen_to_str(r, degree))

gen_crc(8, 10000000, 0x584d52)
```

### 2. Calculation of false-positive error rates

To see the performance of all generated polynomials, we used [crccollide.cpp](https://github.com/sipa/ezbase32/blob/master/crccollide.cpp) that is developed by Bitcoin developers. We compiled it with the default parameters as in:

```bash
$ g++ ezbase32/crccollide.cpp -o crccollide -lpthread -O3
```

Then we run it with 5 errors and 120-character threshold.

```bash
$ mkdir results1
$ parallel -a list.txt ./crccollide {} 5 120 ">" results1/{}.txt
```

The execution took approximately 25 days on Core i5-13500

```bash
39762158.30s user 2845631.54s system 1975% cpu 599:14:56.86 total
```

and generated a huge number of the output files in `results1` directory.

After removing polynomials with a result below the threshold by:

```bash
$ find results1 -name "*.txt" -type f -size -2k -delete
```

`16,976` polynomials were left in the `results1` directory.

```bash
$ ls -1 results1 | wc -l
16976
```

Each file in the `results1` directory looked like:

```bash
...
A00C78KL  123   0.000000000000000   0.000000000000000   0.000000000000000   0.000000000000000   0.000000000000000   1.031711484752184  # 100% done
A00C78KL  124   0.000000000000000   0.000000000000000   0.000000000000000   0.000000000000000   0.010575746914933   1.030752602001270  # 100% done
...
```

The descriptions of the columns are:
1. Polynomial in bech32 hex
1. Input string size
1. False positive error rate for 1-error case
1. False positive error rate for 2-error case
1. False positive error rate for 3-error case
1. False positive error rate for 4-error case
1. False positive error rate for 5-error case
1. False positive error rate for 6-error case

## 3. Extraction of polynomials satisfying the requirements

To extract polynomials that can perfectly detect up to 5 errors, we used `err6-high-perf.py` script below that is a modified version of Jamis's [crc_res.py](https://gist.github.com/tevador/5b3fbbd0877a3412ede07263c6b2663d#:~:text=2.3-,crc_res.py,-The%20crc_res.py) script:

```python
# err6-high-perf.py

import os
from typing import Optional, Tuple

def get_rate(filename, num_char) -> Optional[Tuple[str, float]]:
    gen = ''
    with open(filename) as file:
        for line in file:
            tokens = line.split()
            if len(tokens) == 2 and tokens[1] == "starting":
                gen = tokens[0].rstrip(':')
                continue
            if tokens[0] == gen:
                curr_num_char = int(tokens[1])
                if curr_num_char != num_char:
                    continue
                err4 = float(tokens[1 + 4])
                err5 = float(tokens[1 + 5])
                err6 = float(tokens[1 + 6])
                if err4 > 0 or err5 > 0:
                    return None
                return (gen, err6)
        return None

num_char = 165
dirpath = 'results1'
top_n = 10

gens = []

for entry in os.listdir(dirpath):
    filename = os.path.join(dirpath, entry)
    if os.path.isfile(filename):
        res = get_rate(filename, num_char)
        if res is not None:
            gens.append(res)

gens.sort(key=lambda x: x[1])

for gen in gens[:top_n]:
    print(f"{gen[0]}")
```

This script extracted 2 polynomials.

```bash
$ ./err6-high-perf.py > gens.txt
$ cat gens.txt
U1PIRGA7
AJ4RJKVB
```

Then we built [crccollide.cpp](https://github.com/sipa/ezbase32/blob/master/crccollide.cpp) again with `LENGTH=50` and `ERRORS=4` parameters, and calculated false positive error detection rates of the extracted generators for 7 an 8 error cases:

```bash
$ g++ ezbase32/crccollide.cpp -o crccollide_50_4 -lpthread -O3 -DLENGTH=50 -DERRORS=4 -DTHREADS=4
$ mkdir results2
$ parallel -a gens.txt ./crccollide_50_4 {} ">" results2/{}.txt
```

Comparing the results manually, we found that `U1PIRGA7` is slightly performing better and selected it as the best-performing generator polynomial.

## 4. Generation of mod constants
With the below `enc-gen-to-sage-code.py` script, we generated `SageMath` code to define `U1PIRGA7` as `G`:

```Python
# enc-gen-to-sage-code.py

import sys

if len(sys.argv) < 2:
    exit(f'Usage: {sys.argv[0]} [8-char-poly]')

gen = sys.argv[1]

CHARSET = "0123456789ABCDEFGHIJKLMNOPQRSTUV"
degree = 8

def gen_to_str(gen):
    gen_str = ""
    for i in range(degree):
        gen_str = CHARSET[int(gen) % 32] + gen_str
        gen /= 32
    return gen_str

def str_to_gen(s):
    acc = 0
    coeffs = []
    for c in s:
        acc <<= 5
        i = CHARSET.index(c)
        coeffs.append(i)
        acc += i
    return (acc, coeffs)

def pf_coeffs(coeffs):
    terms = [f'x^{len(coeffs)}']
    for (i,coeff) in enumerate(coeffs):
        if i == len(coeffs) - 1:
            terms.append(f'c({coeff})')
        else:
            terms.append(f'c({coeff})*x^{len(coeffs)-i-1}')
    term_str = ' + '.join(terms)
    return f'G = {term_str}'

acc_coeffs = str_to_gen(gen)
print(acc_coeffs)

recovered_gen = gen_to_str(acc_coeffs[0])
if recovered_gen != gen:
    exit(f'Expected recovered generator to be {gen}, but got {recov
ered_gen}')

print(pf_coeffs(acc_coeffs[1]))
```

The output was:

```bash
$ ./enc-gen-to-sage-code.py U1PIRGA7
(1032724529479, [30, 1, 25, 18, 27, 16, 10, 7])
G = x^8 + c(30)*x^7 + c(1)*x^6 + c(25)*x^5 + c(18)*x^4 + c(27)*x^3
+ c(16)*x^2 + c(10)*x^1 + c(7)
```

Then we embedded the generated `G = ...` line to the below `SageMath` script which is a modified version of the script in `bech32.cpp` comment, and run it to generate `C++` code to compute a modulo by the generator polynomial.

```python
B = GF(2) # Binary field
BP.<b> = B[] # Polynomials over the binary field
F_mod = b**5 + b**3 + 1
F.<f> = GF(32, modulus=F_mod, repr='int') # GF(32) definition
FP.<x> = F[] # Polynomials over GF(32)
E_mod = x**2 + F.fetch_int(9)*x + F.fetch_int(23)
E.<e> = F.extension(E_mod) # GF(1024) extension field definition
for p in divisors(E.order() - 1): # Verify e has order 1023.
   assert((e**p == 1) == (p % 1023 == 0))

c = lambda n: F.fetch_int(n)
G = x^8 + c(30)*x^7 + c(1)*x^6 + c(25)*x^5 + c(18)*x^4 + c(27)*x^3 + c(16)*x^2 + c(10)*x^1 + c(7)

print(G) # Print out the generator

mod_consts = []

for i in [1,2,4,8,16]: # Print out {1,2,4,8,16}*(g(x) mod x^6), packed in hex integers.
  v = 0
  for coef in reversed((F.fetch_int(i)*(G % x**8)).coefficients(sparse=True)):
    v = v*32 + coef.integer_representation()
  mod_consts.append("0x%x" % v)

for (i, mod_const) in enumerate(mod_consts):
    p = 2**i
    s = f'        if (c0 & {p})  c ^= {mod_const}; //  {{{p}}}k(x) ='
    print(s)
```

The generated `C++` code was:

```c++
if (c0 & 1)  c ^= 0xf0732dc147; //  {1}k(x) =
if (c0 & 2)  c ^= 0xa8b6dfa68e; //  {2}k(x) =
if (c0 & 4)  c ^= 0x193fabc83c; //  {4}k(x) =
if (c0 & 8)  c ^= 0x322fd3b451; //  {8}k(x) =
if (c0 & 16)  c ^= 0x640f37688b; //  {16}k(x) =
```

We replaced the corresponding part of the `PolyMod` function with this to use `U1PIRGA7` as the generator polynomial.

