### TestGen ###

Utilities to generate test vectors for the data-driven Bitcoin tests.

To use inside a scripted-diff (or just execute directly):

    PYTHONPATH=./test/functional/test_framework ./contrib/testgen/gen_key_io_test_vectors.py valid 70 > ./src/test/data/key_io_valid.json
    PYTHONPATH=./test/functional/test_framework ./contrib/testgen/gen_key_io_test_vectors.py invalid 70 > ./src/test/data/key_io_invalid.json
