# Verifying Equations

## Verification Scheme
Points on the LHS of a equation are negated and added to the points on the RHS. Verification is successful if the sum is the additive identity.

## Line (18), (20) and (21)
(18), (20) and (21) are self-contained and each adds up to the additive identity.

## Line (19)
On the `PePoS` paper, the equation (19) is

$h_2^{\mu}\mathbf{Y}^{\mathbf{l}} (\mathbf{h'})^\mathbf{r} = A_1 \cdot A_2^{\omega} \cdot S_2^x \cdot \mathbf{Y}^{-z \cdot \mathbf{1}^n} \cdot (\mathbf{h'})^{\omega z \cdot \mathbf{y}^n + z^2 \cdot \mathbf{1}^n}$

But, due to the use of the improved inner product argument (`IIPA` from this point onward), the verification procedure is not straightforward. The procedure will be explained in details using a 2-round `IIPA` case in order to minimize the complexity.

## Result of 2-round `IIPA`
Given vectors of scalars $a=[a_1,a_2], b=[b_1,b_2]$, generators $G_1,G_2$, $H_1,H_2, u$ and a scalar $c\_factor$, the 2-round `IIPA` returns

- $a = a_1 \cdot x_1 + a_2 \cdot x_1^{-1}$
- $b = b_1 \cdot x_1^{-1}+b_2 \cdot x_1$
- $Ls[0] = G_2^{a_1}+H_1^{b_2 y^{-1}} + u^{a_1 b_2 \cdot c\_factor}$
- $Rs[0] = G_1^{a_2}+H_2^{b_1 y^{-1}} + u^{a_2 b_1 \cdot c\_factor}$

## Verification Procedure
### 1. Addition of $A_1 \cdot A_2^{\omega} \cdot S_2^x - h_2^{\mu}$

$h_2^{\mu}$ on the LHS and $A_1 \cdot A_2^{\omega} \cdot S_2^x$ on the RHS are taken from the equation as is, and added the sum.

```c++
// A1 + A2^omega + S2^x on RHS
points.Add(LazyPoint(proof.A1, one()));
points.Add(LazyPoint(proof.A2, proof.omega));
points.Add(LazyPoint(proof.S2, x));

// h^mu on LHS
points.Add(LazyPoint(h2, proof.mu.Negate()));
```

Since $A_1$, $A_2^{\omega}$, $S_2^x$, and $h_2^{\mu}$ are defined to be

- $A_1 = h_2^{\alpha|}\mathbf{Y}^{\mathbf{bL}}$
- $A_2 = h_2^{\beta}\mathbf{h}^{\mathbf{bR}}$
- $S_2 = h_2^{\rho}\mathbf{Y}^{\mathbf{sL}}\mathbf{h}^{\mathbf{sR}}$
- $\mu = \alpha + \beta \cdot \omega + \rho \cdot x $

$h_2^{\mu} = h_2^{\alpha + \beta \cdot \omega + \rho \cdot x}$

$A_1 \cdot A_2^{\omega} \cdot S_2^x - h_2^{\mu}$ is evaluated to

$(h_2^{\alpha|}\mathbf{Y}^{\mathbf{bL}}) \cdot (h_2^{\beta}\mathbf{h}^{\mathbf{bR}})^{\omega} \cdot (h_2^{\rho}\mathbf{Y}^{\mathbf{sL}}\mathbf{h}^{\mathbf{sR}})^x - h_2^{\alpha + \beta \cdot \omega + \rho \cdot x}$

$= (\mathbf{Y}^{\mathbf{bL}}) \cdot (\mathbf{h}^{\mathbf{bR}})^{\omega} \cdot (\mathbf{Y}^{\mathbf{sL}}\mathbf{h}^{\mathbf{sR}})^x$

$= \mathbf{Y}^{\mathbf{bL} + \mathbf{sL}x} \cdot \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x}$

This is the initial sum we have.

### 2. Recovery of $x$'s in `IIPA`
Below code recovers $x$'s used in all rounds of `IIPA` with `Fiat-Shamir`.

```c++
// recover x's and x_inv's of all rounds
Scalars xs;
Scalars inv_xs;
for (size_t i = 0; i < num_rounds; ++i) {
    fiat_shamir << proof.Ls[i];
    fiat_shamir << proof.Rs[i];
    Scalar x(fiat_shamir.GetHash());
    xs.Add(x);
    inv_xs.Add(x.Invert());
}
```

### 3. Recovery of Generator Exponents
At the end of the `IIPA`, the scalar compressed generators $G$ and $H$ have $G_1^{\alpha_1}, G_2^{\alpha_2}, ..., G_n^{\alpha_n}$ and $H_1^{\beta_1}, H_2^{\beta_2}, ..., H_n^{\beta_n}$ as factors. $n$ here is the number of rounds in `IIPA`.

Below code calculates `acc_xs`'s that directly maps to $\alpha_i$ and maps to $\beta_i$ in reverse. i.e.

- $\alpha_1 = acc\_xs[0], \alpha_2 = acc\_xs[1], ...$
- $\beta_1 = acc\_xs[n-1], \beta_2 = acc\_xs[n-2], ...$.

```c++
// acc_xs's are factors used to recover Gi and Hi contained
// in G and H respectively at the end of inner product argument loop.
// Using acc_xs's, the generators are recoved as follows:
// - G_1*acc_xs[0], G_2*acc_xs[1], ...
// - H_1*acc_xs[last], H_2*acc_xs[last-1], ...
std::vector<Scalar> acc_xs(1ull << num_rounds, 1);
acc_xs[0] = inv_xs[0];
acc_xs[1] = xs[0];

for (size_t i = 1; i < num_rounds; ++i) {
    const size_t sl = 1ull << (i + 1);
    for (long signed int s = sl - 1; s > 0; s -= 2) {
        acc_xs[s] = acc_xs[s / 2] * xs[i];
        acc_xs[s - 1] = acc_xs[s / 2] * inv_xs[i];
    }
}
```

### 4. Addition of $L$'s and $R$'s
Putting the for-loop aside for now, the next verification code is

```c++
for (size_t i=0; i<num_rounds; ++i) {
    points.Add(LazyPoint(proof.Ls[i], xs[i].Square()));
    points.Add(LazyPoint(proof.Rs[i], inv_xs[i].Square()));
}
```

This adds $L$ and $R$ of each round of `IIPA` to the sum multiplying $x^2$ and $x^{-2}$ of the corresponding round.

Note that $x$ and $x^{-1}$ are random values generated in each round except for the final round.  $L$ and $R$ are not calculated in the final round either.

$L$ and $R$ are calculated as follows.

```c++
Point L = (
    LazyPoints<T>(Gi.From(n), a.To(n)) +
    LazyPoints<T>(Hi.To(n), rounds == 0 ? b.From(n) * y_inv_pows.To(n) : b.From(n)) +
    LazyPoint<T>(u, (a.To(n) * b.From(n)).Sum() * c_factor)
).Sum();
Point R = (
    LazyPoints<T>(Gi.To(n), a.From(n)) +
    LazyPoints<T>(Hi.From(n), rounds == 0 ? b.To(n) * y_inv_pows.From(n) : b.To(n)) +
    LazyPoint<T>(u, (a.From(n) * b.To(n)).Sum() * c_factor)
).Sum();
```

2-round `IIPA` calculates below and adds to the sum.

- $Ls[0] \cdot x_1^2$

  $(G_2^{a_1} + H_1^{b_2} + u^{a_1 b_2 \cdot c\_factor})^{x_1^2}$

  $= G_2^{a_1x_1^2}+H_1^{b_2 x_1^2} + u^{a_1 b_2 \cdot c\_factor \cdot x_1^2}$

- $Rs[0] \cdot x_1^{-2}$

  $(G_1^{a_2}+H_2^{b_1 \cdot y^{-1}} + u^{a_2 b_1 \cdot c\_factor})^{x_1^{-2}}$

  $= G_1^{a_2x_1^{-2}} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} + u^{a_2 b_1 \cdot c\_factor \cdot x_1^{-2}}$

Combining above we get

- $Ls[0] \cdot x_1^2 +  Rs[0] \cdot x_1^{-2}$

  $G_2^{a_1x_1^2}+H_1^{b_2 x_1^2} + u^{a_1 b_2 \cdot c\_factor \cdot x_1^2} + G_1^{a_2 x_1^{-2}} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} + u^{a_2 b_1 \cdot c\_factor \cdot x_1^{-2}}$

  $= G_1^{a_2x_1^{-2}} + G_2^{a_1x_1^2} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} + u^{a_1 b_2 \cdot c\_factor \cdot x_1^2} + u^{a_2 b_1 \cdot c\_factor \cdot x_1^{-2}}$

  $= G_1^{a_2 x_1^{-2}} + G_2^{a_1 x_1^2} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} + u^{a_1 b_2 \cdot c\_factor \cdot x_1^2 + a_2 b_1 \cdot c\_factor \cdot x_1^{-2}}$

Now the sum is

$\mathbf{Y}^{\mathbf{bL} + \mathbf{sL}x} \cdot \mathbf{h}^{\mathbf{bR} \omega + \mathbf{sR}x} + G_1^{a_2 x_1^{-2}} + G_2^{a_1 x_1^2} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} + u^{a_1 b_2 \cdot c\_factor \cdot x_1^2 + a_2 b_1 \cdot c\_factor \cdot x_1^{-2}}$

### 5. Addition of $t \cdot c\_factor$ and Subtraction of $a \cdot b \cdot c\_factor$

The next verification code adds and subtracts exponents of the $u$ generator.

```c++
g_pos_exp = g_pos_exp + (proof.t - proof.a * proof.b) * proof.c_factor;
```

This adds $u^t$ and subtracts $u^{a \cdot b \cdot c\_factor}$ to/from the sum.

In 2-round `IIPA`, $a \cdot b \cdot c\_factor$ is equal to

$(a_1 \cdot x_1+a_2 \cdot x_1^{-1})(b_1 \cdot x_1^{-1}+b_2 \cdot x_1) \cdot c\_factor$

$= (a_1 \cdot b_1 + a_1 \cdot b_2 \cdot x_1^2 + a_2 \cdot b_1 \cdot x_1^{-2} + a_2 \cdot b_2) \cdot c\_factor$

$= a_1 \cdot b_1 \cdot c\_factor + a_1 \cdot b_2 \cdot x_1^2 \cdot c\_factor + a_2 \cdot b_1 \cdot x_1^{-2} \cdot c\_factor + a_2 \cdot b_2 \cdot c\_factor$

Adding this, the sum becomes

$\mathbf{Y}^{\mathbf{bL} + \mathbf{sL}x} \cdot \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + G_1^{a_2 x_1^{-2}} + G_2^{a_1 x_1^2} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} + u^{a_1 b_2 \cdot c\_factor \cdot x_1^2 + a_2 b_1 \cdot c\_factor \cdot x_1^{-2}} - u^{a_1 \cdot b_1 \cdot c\_factor + a_1 \cdot b_2 \cdot x_1^2 \cdot c\_factor - a_2 \cdot b_1 \cdot x_1^{-2} \cdot c\_factor + a_2 \cdot b_2 \cdot c\_factor}$

$= \mathbf{Y}^{\mathbf{bL} + \mathbf{sL}x} \cdot \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + G_1^{a_2 x_1^{-2}} + G_2^{a_1 x_1^2} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} + u^{a_1 b_2 \cdot c\_factor \cdot x_1^2 + a_2 b_1 \cdot c\_factor \cdot x_1^{-2}} - (u^{a_1 \cdot b_1 \cdot c\_factor} + u^{a_1 \cdot b_2 \cdot x_1^2 \cdot c\_factor - a_2 \cdot b_1 \cdot x_1^{-2} \cdot c\_factor} + u^{a_2 \cdot b_2 \cdot c\_factor})$

$= \mathbf{Y}^{\mathbf{bL} + \mathbf{sL}x} \cdot \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + G_1^{a_2 x_1^{-2}} + G_2^{a_1 x_1^2} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} - u^{a_1 \cdot b_1 \cdot c\_factor} - u^{a_2 \cdot b_2 \cdot c\_factor}$

The 2nd and 3rd terms $u^{a_1 \cdot b_2 \cdot x_1^2 \cdot c\_factor + a_2 \cdot b_1 \cdot x_1^{-2} \cdot c\_factor}$ removes the $u$ point from the sum $u^{a_1 b_2 \cdot c\_factor \cdot x_1^2 + a_2 b_1 \cdot c\_factor \cdot x_1^{-2}}$.

The 1st and 4th terms are

$a_1 \cdot b_1 \cdot c\_factor + a_2 \cdot b_2 \cdot c\_factor$

$= (a_1 \cdot b_1 + a_2 \cdot b_2) \cdot c\_factor$

$= \langle a, b \rangle \cdot c\_factor$

$= t \cdot c\_factor$

So, $u^t$ is equivalent to $u^{a_1 \cdot b_1 \cdot c\_factor + a_2 \cdot b_2 \cdot c\_factor}$.

Adding this to the sum removes the remaining $u$ terms, and the sum becomes

$= \mathbf{Y}^{\mathbf{bL} + \mathbf{sL}x} \cdot \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + G_1^{a_2 x_1^{-2}} + G_2^{a_1 x_1^2} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}}$

### 6. Generation of Generator Vector Exponents

Now only the for-loop of the verification code remains unexplained. The loop calculates exponents of the $\mathbf{G}$ and $\mathbf{H}$ generator vectors.

```c++
Scalar y_inv_pow(1);
Scalar y_pow(1);
for (size_t i = 0; i < n; ++i) {
    Scalar gi_exp = (proof.a * acc_xs[i]).Negate();
    Scalar hi_exp = (proof.b * y_inv_pow * acc_xs[n - 1 - i]).Negate();

    // With bL + sLx that has been added, constitute
    // l(x) = bL - z*1^n + sLx by additionally subtracting z
    gi_exps[i] = gi_exp - z;

    // With bR*omega + sRx that has been added, constitute
    // y_inv_pow * r(x) = omega*bR + omega*z*1^n + sR*x + z^2*1^n*y_inv_pow
    // by adding omega*z*1^n + z^2*1^n*y_inv_pow
    hi_exps[i] = hi_exp + (proof.omega * z * y_pow + z_sq) * y_inv_pow;

    // update y_pow and y_inv_pow to the next power
    y_inv_pow = y_inv_pow * y_inv;
    y_pow = y_pow * y;
}
```

The calculated exponents are multiplied to the generator vectors used in the prove phase and added to the sum as follows

```c++
auto Hi = setup.hs.To(n);
for (size_t i=0; i<n; ++i) {
    points.Add(LazyPoint(Ys[i], gi_exps[i]));
    points.Add(LazyPoint(Hi[i], hi_exps[i]));
}
```

### $G$ generator vector

Below verification code calculates `gi_exp[]`.

```c++
Scalar gi_exp = (proof.a * acc_xs[i]).Negate();
...
// With bL + sLx that has been added, constitute
// l(x) = bL - z*1^n + sLx by additionally subtracting z
gi_exps[i] = gi_exp - z;
```

In 2-round case, `acc_xs`'s are

- `acc_xs[0]`: $x^{-1}_1$
- `acc_xs[1]`: $x_1$

and the first line results in

- `gi_exps[0]`: $-(a_1 \cdot x_1 + a_2 \cdot x_1^{-1}) \cdot x^{-1}_1 = -(a_1 + a_2 \cdot x_1^{-2})$

- `gi_exps[1]`: $-(a_1 \cdot x_1 + a_2 \cdot x_1^{-1}) \cdot x_1 = -(a_1 \cdot x_1^2 + a_2)$

Multiplying them with $G$ generators and adding the resulting points, we get

$-G_1^{a_1 + a_2 \cdot x_1^{-2}} - G_2^{a_2 + a_1 \cdot x_1^2}$

$= -G_1^{a_1} - G_2^{a_2} - G_1^{a_2 \cdot x_1^{-2}} - G_2^{a_1 \cdot x_1^2}$

Adding these points to the sum, the 3rd and 4th terms removes $G$ based points from the sum, we now have

$\mathbf{Y}^{\mathbf{bL} + \mathbf{sL}x} \cdot \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + G_1^{a_2 x_1^{-2}} + G_2^{a_1 x_1^2} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} - (G_1^{a_1} + G_2^{a_2} + G_1^{a_2 \cdot x_1^{-2}} + G_2^{a_1 \cdot x_1^2})$

$= \mathbf{Y}^{\mathbf{bL} + \mathbf{sL}x} \cdot \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} - G_1^{a_1} - G_2^{a_2}$

Since $\mathbf{Y}$ generator vector is used as $\mathbf{G}$ in `PePos`, replacing $\mathbf{Y}$ with $\mathbf{G}$, we get

$\mathbf{G}^{\mathbf{bL} + \mathbf{sL}x} \cdot \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} - G_1^{a_1} - G_2^{a_2}$

Applying the 2nd line subtracting $z$, next the sum becomes

$\mathbf{G}^{\mathbf{bL} + \mathbf{sL}x - z} \cdot \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} - G_1^{a_1} - G_2^{a_2}$

On the `PePoS` paper,

$l(x) = \mathbf{bL} - z \cdot \mathbf{1}^n + \mathbf{sL} \cdot x$

and $l(x)$ in a form of $[l_1, l_2]$ is sent to `IIPA`.

Since $l_1, l_2$ are called $a_1, a_2$ in `IIPA`, the sum is equivalent to

$G_1^{a_1} + G_2^{a_2} + \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} - G_1^{a_1} - G_2^{a_2}$

$G_i$'s cancel out and that leaves

$\mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}}$

### $H$ generator vector

Below verification code calculates `hi_exp[]`.

```c++
Scalar hi_exp = (proof.b * y_inv_pow * acc_xs[n - 1 - i]).Negate();
...
// With bR*omega + sRx that has been added, constitute
// y_inv_pow * r(x) = omega*bR + omega*z*1^n + sR*x + z^2*1^n*y_inv_pow
// by adding omega*z*1^n + z^2*1^n*y_inv_pow
hi_exps[i] = hi_exp + (proof.omega * z * y_pow + z_sq) * y_inv_pow;
```

In 2-round case, `acc_xs`'s are

- acc_xs[0]: $x^{-1}_1$
- acc_xs[1]: $x_1$

The first line

```c++
Scalar hi_exp = (proof.b * y_inv_pow * acc_xs[n - 1 - i]).Negate();
```

generates

- `hi_exps[0]`: $-((b_1 \cdot x_1^{-1}+b_2 \cdot x_1) \cdot y^0 \cdot x_1) = -(b_1 + b_2 \cdot {x_1^2})$

- `hi_exps[1]`: $-((b_1 \cdot x_1^{-1}+b_2 \cdot x_1) \cdot y^{-1} \cdot x^{-1}_1) = -(b_1 \cdot y^{-1} \cdot x_1^{-2} + b_2 \cdot y^{-1})$

Multiplying them with $H$ generators and adding the resulting points to the sum, we get

$-H_1^{b_1 + b_2 \cdot {x_1^2}} - H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2} + b_2 \cdot y^{-1}}$

$= -H_1^{b_1} - H_1^{b_2 \cdot {x_1^2}} - H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} - H_2^{b_2 \cdot y^{-1}}$

The 2nd and 3rd terms above cancel out two terms in the sum which results in

$\mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + H_1^{b_2 x_1^2} + H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} - H_1^{b_1} - H_1^{b_2 \cdot {x_1^2}} - H_2^{b_1 \cdot y^{-1} \cdot x_1^{-2}} - H_2^{b_2 \cdot y^{-1}}$

$= \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} - H_1^{b_1} - H_2^{b_2 \cdot y^{-1}}$

Then the 2nd line

```c++
hi_exps[i] = hi_exp + (proof.omega * z * y_pow + z_sq) * y_inv_pow;
```

multiplies $\mathbf{H}$ generators by $(\omega \cdot z \cdot y'_i + z^2) \cdot \mathbf{y'}^{-1}_i$ where $\mathbf{y'_i} = [y^0,y^1,y^2,...,y^n]$, $\mathbf{y'}^{-1}_i = [y^0, y^{-1},y^{-2},...,y^{-n}]$ and $n$ is the size of the $\mathbf{G}, \mathbf{H}$ generator vectors.

In 2-round `IIPA`, generated points are

$H_1^{\omega \cdot z + z^2} + H_2^{(\omega \cdot z \cdot y + z^2) \cdot y^{-1}}$

$= H_1^{\omega \cdot z + z^2} + H_2^{\omega \cdot z + z^2 \cdot y^{-1}}$

Adding those points to the sum results in

$\mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} - H_1^{b_1} - H_1^{b_2 \cdot y^{-1}} + H_1^{\omega \cdot z + z^2} + H_2^{\omega \cdot z + z^2 \cdot y^{-1}}$

$= \mathbf{h}^{\mathbf{bR}\omega + \mathbf{sR}x} + H_1^{\omega \cdot z + z^2} + H_2^{\omega \cdot z + z^2 \cdot y^{-1}} - H_1^{b_1} - H_2^{b_2 \cdot y^{-1}}$

Because $\mathbf{h}$ generator vector is called $\mathbf{H}$ in `IIPA`

$\mathbf{H}^{\mathbf{bR}\omega + \mathbf{sR}x} + H_1^{\omega \cdot z + z^2} + H_2^{\omega \cdot z + z^2 \cdot y^{-1}} - H_1^{b_1} - H_2^{b_2 \cdot y^{-1}}$

$= H_1^{bR[0] \cdot \omega + sR[0] \cdot x + \omega z + z^2} + H_2^{bR[1] \cdot \omega + sR[1] \cdot x + \omega \cdot z + z^2 \cdot y^{-1}} - H_1^{b_1} - H_2^{b_2 \cdot y^{-1}}$

$= H_1^{\omega \cdot bR[0] + \omega z + sR[0] \cdot x + z^2} + H_2^{\omega \cdot bR[1] + sR[1] \cdot x + \omega z + z^2 \cdot y^{-1}} - H_1^{b_1} - H_2^{b_2 \cdot y^{-1}}$

Note that

$r(x)= \mathbf{y}^n \circ (\omega \cdot bR + \omega z \cdot \mathbf{1}^n + sR \cdot x) + z^2 \cdot \mathbf{1}^n$

where $\mathbf{y}= \{y^0,y^1,y^2,...,y^{n-1}\}$ and $r(x)$ in 2-round case in $r_i$ form is

$r_1 = \omega \cdot bR[0] + \omega z + sR[0] \cdot x + z^2$

$r_2 = \omega \cdot bR[1] \cdot y + \omega z y + sR[1] \cdot xy + z^2$

Therefore $r(x) \cdot y'^{-1}_i$ in $r_1, r_2$ form is

$r_1 \cdot y^0 = r_1 = \omega \cdot bR[0] + \omega z + sR[0] \cdot x + z^2$

$r_2 \cdot y^{-1} = \omega \cdot bR[1] + \omega z + sR[1] \cdot x + z^2 \cdot y^{-1}$

Replacing the exponents of the 1st and 2nd terms with $r_1$ and $r_2 \cdot y^{-1}$ respectively, the sum becomes

$H_1^{r_1} + H_2^{r_2 \cdot y^{-1}} - H_1^{b_1} - H_2^{b_2 \cdot y^{-1}}$

Since $r_i$ is called $b_i$ in `IIPA`, by replacing $r_i$ with $b_i$, we get

$H_1^{b_1} + H_2^{b_2 \cdot y^{-1}} - H_1^{b_1} - H_2^{b_2 \cdot y^{-1}} = 0$

This completes the verification of (19).

