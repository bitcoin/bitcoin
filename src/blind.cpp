


void ECC_Start_Blinding()
{
    assert(secp256k1_ctx_blind == NULL);

    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    assert(ctx != NULL);

    secp256k1_ctx_blind = ctx;
};

void ECC_Stop_Blinding()
{
    secp256k1_context *ctx = secp256k1_ctx_blind;
    secp256k1_ctx_blind = NULL;

    if (ctx)
    {
        secp256k1_context_destroy(ctx);
    };
};
