#include <axiom.h>
#include <sphlib/sph_shabal.h>

#ifdef GLOBALDEFINED
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL sph_shabal256_context z_shabal;

#define fillz() do { \
sph_shabal256_init(&z_shabal); \
} while (0)

#define ZSHABAL (memcpy(&ctx_shabal, &z_shabal, sizeof(z_shabal)))

inline unsigned int GetInt(const uint256 &ui)
{
    return *(unsigned int *)ui.begin();
}

uint256 AxiomHash(char const* pbegin, char const* pend)
{
    // Axiom Proof of Work Hash
    // based on RandMemoHash https://bitslog.files.wordpress.com/2013/12/memohash-v0-3.pdf
   /* RandMemoHash(s, R, N)
    (1) Set M[0] := s
    (2) For i := 1 to N − 1 do set M[i] := H(M[i − 1])
    (3) For r := 1 to R do
        (a) For b := 0 to N − 1 do
            (i) p := (b − 1 + N) mod N
            (ii) q :=AsInteger(M[p]) mod (N − 1)
            (iii) j := (b + q) mod N
            (iv) M[b] :=H(M[p] || M[j])
*/
    int R = 2;
    int N = 65536;

    std::vector<uint256> M(N);
    sph_shabal256_context ctx_shabal;
    static unsigned char pblank[1];
    uint256 hash1;
    sph_shabal256_init(&ctx_shabal);
    sph_shabal256 (&ctx_shabal, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_shabal256_close(&ctx_shabal, static_cast<void*>(&hash1));
    M[0] = hash1;

    for(int i = 1; i < N; i++)
    {
        //HashShabal((unsigned char*)&M[i - 1], sizeof(M[i - 1]), (unsigned char*)&M[i]);
    sph_shabal256_init(&ctx_shabal);
        sph_shabal256 (&ctx_shabal, (unsigned char*)&M[i - 1], sizeof(M[i - 1]));
        sph_shabal256_close(&ctx_shabal, static_cast<void*>((unsigned char*)&M[i]));
    }

    for(int r = 1; r < R; r ++)
    {
    for(int b = 0; b < N; b++)
    {
        int p = (b - 1 + N) % N;
        int q = GetInt(M[p]) % (N - 1);
        int j = (b + q) % N;
        std::vector<uint256> pj(2);

        pj[0] = M[p];
        pj[1] = M[j];
        //HashShabal((unsigned char*)&pj[0], 2 * sizeof(pj[0]), (unsigned char*)&M[b]);
        sph_shabal256_init(&ctx_shabal);
            sph_shabal256 (&ctx_shabal, (unsigned char*)&pj[0], 2 * sizeof(pj[0]));
            sph_shabal256_close(&ctx_shabal, static_cast<void*>((unsigned char*)&M[b]));
    }
    }

    return M[N - 1];
}
/*
uint256 AxiomHash(char const* pbegin, char const * pend)
{
    return _AxiomHash<char const*>(pbegin,pend);
}
*/
