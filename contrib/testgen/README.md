### TestGen ###

Utilities to generate test vectors for the data-driven Dash tests.

To use inside a scripted-diff (or just execute directly):

    ./gen_key_io_test_vectors.py valid 50 > ../../src/test/data/key_io_valid.json
    ./gen_key_io_test_vectors.py invalid 50 > ../../src/test/data/key_io_invalid.json
