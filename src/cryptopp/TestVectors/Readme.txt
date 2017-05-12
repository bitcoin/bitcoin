Test Data Format
================

A test data file is an ASCII text file composed of sections separated by
blank lines. Each section is stand-alone and independent of other
sections that may be in the same file, and contains one or more tests.

A section is composed of a sequence of fields. Each field is one or more
lines composed of a field name, followed by a colon (":"), followed by a
field body. All but the last line of a field must end with a backslash
("\"). If any line contains a hash mark ("#"), the hash mark and
everything after it on the same line is not considered part of the field
body.

Each section must contain fields named AlgorithmType, Name, Source, and
Test. The presence and semantics of other fields depend on the algorithm
being tested and the tests to be run.

Each section may contain more than one test and therefore more than one
field named Test. In that case the order of the fields is significant. A
test should always use the last field with any given name that occurs
before the Test field.

Data Types
==========

int - small integer (less than 2^32) in decimal representation
string - human readable string
encoded string - can be one of the following
	- quoted string: "message" means "message" without the quotes
	  or terminating '\0'
	- hex encoded string: 0x74657374 or 74657374 means "test"
	- repeated string: r100 "message" to repeat "message" 100 times, or
	  r256 0x0011 to repeat 0x0011 256 times

Field Types
===========

AlgorithmType - string, for example "Signature", "AsymmetricCipher",
    "SymmetricCipher", "MAC", "MessageDigest", or "KeyFactory"
Name - string, an algorithm name from SCAN
Test - string, identifies the test to run
Source - string, text explaining where the test data came from
Comment - string, other comments about the test data
KeyFormat - string, specifies the key format. "Component" here means
    each component of the key or key pair is specified separately as a name,
    value pair, with the names depending on the algorithm being tested.
    Otherwise the value names "Key", or "PublicKey" and "PrivateKey" are
    used.
Key - encoded string
PublicKey - encoded string
PrivateKey - encoded string
Modulus - the modulus when KeyFormat=Component
SubgroupOrder - the subgroup order when KeyFormat=Component
SubgroupGenerator - the subgroup generator when KeyFormat=Component
PublicElement - the public element when KeyFormat=Component
PrivateExponent - the private exponent when KeyFormat=Component
Message - encoded string, message to be signed or verified
Signature - encoded string, signature to be verified or compared with
Plaintext - encoded string
Ciphertext - encoded string
Header - encoded string
Footer - encoded string
DerivedKey - encoded string
DerivedLength - encoded string
Digest - encoded string
TruncatedSize - int, size of truncated digest in bytes
Seek - int, seek location for random access ciphers
(more to come here)

Possible Tests
==============

KeyPairValidAndConsistent - public and private keys are both valid and
consistent with each other
PublicKeyInvalid - public key validation should not pass
PrivateKeyInvalid - private key validation should not pass
Verify - signature/digest/MAC verification should pass
VerifyTruncated - truncated digest/MAC verification should pass
NotVerify - signature/digest/MAC verification should not pass
DeterministicSign - sign message using given seed, and the resulting
    signature should be equal to the given signature
DecryptMatch - ciphertext decrypts to plaintext

(more to come here)
