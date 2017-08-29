import sys

# Should be in EXACT same order as from src/mongoc/mongoc-handshake-private.h.
# The values are implicit (so we assume 1st entry is 1 << 0,
# second entry is 1 << 1 and so on).
MD_FLAGS = [
    "MONGOC_MD_FLAG_ENABLE_CRYPTO",
    "MONGOC_MD_FLAG_ENABLE_CRYPTO_CNG",
    "MONGOC_MD_FLAG_ENABLE_CRYPTO_COMMON_CRYPTO",
    "MONGOC_MD_FLAG_ENABLE_CRYPTO_LIBCRYPTO",
    "MONGOC_MD_FLAG_ENABLE_CRYPTO_SYSTEM_PROFILE",
    "MONGOC_MD_FLAG_ENABLE_SASL",
    "MONGOC_MD_FLAG_ENABLE_SSL",
    "MONGOC_MD_FLAG_ENABLE_SSL_OPENSSL",
    "MONGOC_MD_FLAG_ENABLE_SSL_SECURE_CHANNEL",
    "MONGOC_MD_FLAG_ENABLE_SSL_SECURE_TRANSPORT",
    "MONGOC_MD_FLAG_EXPERIMENTAL_FEATURES",
    "MONGOC_MD_FLAG_HAVE_SASL_CLIENT_DONE",
    "MONGOC_MD_FLAG_HAVE_WEAK_SYMBOLS",
    "MONGOC_MD_FLAG_NO_AUTOMATIC_GLOBALS",
    "MONGOC_MD_FLAG_ENABLE_SSL_LIBRESSL",
    "MONGOC_MD_FLAG_ENABLE_SASL_CYRUS",
    "MONGOC_MD_FLAG_ENABLE_SASL_SSPI",
    "MONGOC_MD_FLAG_HAVE_SOCKLEN",
    "MONGOC_MD_FLAG_ENABLE_COMPRESSION",
    "MONGOC_MD_FLAG_ENABLE_COMPRESSION_SNAPPY",
    "MONGOC_MD_FLAG_ENABLE_COMPRESSION_ZLIB",
    "MONGOC_MD_FLAG_ENABLE_SASL_GSSAPI",
]

def main():
    flag_to_number = {s: 2 ** i for i,s in enumerate(MD_FLAGS)}

    if len(sys.argv) < 2:
        print "Usage: python {0} config-bitfield".format(sys.argv[0])
        print "Example: python parse_handshake_cfg.py 0x3e65"
        return

    config_bitfield_string = sys.argv[1]
    config_bitfield_num = int(config_bitfield_string, 0)
    print "Decimal value: {}".format(config_bitfield_num)

    for flag, num in flag_to_number.iteritems():
        v = "true" if config_bitfield_num & num else "false"
        print "{:<50}: {}".format(flag, v)

if __name__ == "__main__":
    main()
