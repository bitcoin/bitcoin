#pragma once
/**
	@file
	@brief Lagrange Interpolation
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
namespace mcl {

/*
	recover out = f(0) by { (x, y) | x = S[i], y = f(x) = vec[i] }
	@retval 0 if succeed else -1
*/
template<class G, class F>
void LagrangeInterpolation(bool *pb, G& out, const F *S, const G *vec, size_t k)
{
	if (k == 0) {
		*pb = false;
		return;
	}
	if (k == 1) {
		out = vec[0];
		*pb = true;
		return;
	}
	/*
		delta_{i,S}(0) = prod_{j != i} S[j] / (S[j] - S[i]) = a / b
		where a = prod S[j], b = S[i] * prod_{j != i} (S[j] - S[i])
	*/
	F a = S[0];
	for (size_t i = 1; i < k; i++) {
		a *= S[i];
	}
	if (a.isZero()) {
		*pb = false;
		return;
	}
	/*
		f(0) = sum_i f(S[i]) delta_{i,S}(0)
	*/
	G r;
	r.clear();
	for (size_t i = 0; i < k; i++) {
		F b = S[i];
		for (size_t j = 0; j < k; j++) {
			if (j != i) {
				F v = S[j] - S[i];
				if (v.isZero()) {
					*pb = false;
					return;
				}
				b *= v;
			}
		}
		G t;
		G::mul(t, vec[i], a / b);
		r += t;
	}
	out = r;
	*pb = true;
}

/*
	out = f(x) = c[0] + c[1] * x + c[2] * x^2 + ... + c[cSize - 1] * x^(cSize - 1)
	@retval 0 if succeed else -1 (if cSize == 0)
*/
template<class G, class T>
void evaluatePolynomial(bool *pb, G& out, const G *c, size_t cSize, const T& x)
{
	if (cSize == 0) {
		*pb = false;
		return;
	}
	if (cSize == 1) {
		out = c[0];
		*pb = true;
		return;
	}
	G y = c[cSize - 1];
	for (int i = (int)cSize - 2; i >= 0; i--) {
		G::mul(y, y, x);
		G::add(y, y, c[i]);
	}
	out = y;
	*pb = true;
}

#ifndef CYBOZU_DONT_USE_EXCEPTION
template<class G, class F>
void LagrangeInterpolation(G& out, const F *S, const G *vec, size_t k)
{
	bool b;
	LagrangeInterpolation(&b, out, S, vec, k);
	if (!b) throw cybozu::Exception("LagrangeInterpolation");
}

template<class G, class T>
void evaluatePolynomial(G& out, const G *c, size_t cSize, const T& x)
{
	bool b;
	evaluatePolynomial(&b, out, c, cSize, x);
	if (!b) throw cybozu::Exception("evaluatePolynomial");
}
#endif

} // mcl
