# Stealth Addresses

Transacting on the MWEB happens via stealth addresses. Stealth addresses are supported using the *Dual-Key Stealth Address Protocol (DKSAP)*. These addresses consist of 2 secp256k1 public keys (**A<sub>i</sub>**, **B<sub>i</sub>**) where **A<sub>i</sub>** is the scan pubkey used for identifying outputs, and **B<sub>i</sub>** is the spend pubkey.

### Notation
* **G** = Curve generator point
* Bold uppercase letter (**A**, **A<sub>i</sub>**, etc.) represents a secp256k1 public key
* Bold lowercase letter (**a**, **a<sub>i</sub>**, etc.) represents a scalar
* A letter in single quotes ('K') represents the ASCII value of the character literal
* HASH32(x | y | z) represents the standard 32-byte `BLAKE3` hash of the conjoined serializations of `x`, `y`, and `z`
* HASH64(m) represents the `BLAKE3` hash of `m` using a 64-byte output digest
* SWITCH(**v**, **r**) represents the pedersen commitment generated using a blind switch with value **v** and key **r**.

### Address Generation

Unique stealth addresses should be generated deterministically from a wallet's seed using the following formula (using **G** = generator point):

1. Generate master scan keypair (**a**, **A**) using HD keychain path `m/1/0/100'`
2. Generate master spend keypair (**b**, **B**) using HD keychain path `m/1/0/101'`
3. Choose the lowest unused address index **i**
4. Calculate one-time spend keypair (**b<sub>i</sub>**, **B<sub>i</sub>**) as:<br/>
    **b<sub>i</sub>** = **b** + HASH32(**A** | **i** | **a**)<br/>
    **B<sub>i</sub>** = **b<sub>i</sub>\*G** where **G** refers to the curve generator point
5. Calculate one-time scan keypair (**a<sub>i</sub>**, **A<sub>i</sub>**) as:<br/>
    **a<sub>i</sub>** = **a\*b<sub>i</sub>**<br/>
    **A<sub>i</sub>** = **a<sub>i</sub>\*G** where **G** refers to the curve generator point

### Outputs

Outputs consist of the following data:

* **C<sub>o</sub>** - The pedersen commitment to the value.
* Output message, **m**, consisting of:
  * **K<sub>o</sub>** - The receiver's one-time public key.
  * **K<sub>e</sub>** - The key exchange public key.
  * **K<sub>s</sub>** - The sender's public key.
  * **t** - The view tag. This is the first byte of the shared secret.
  * **v'** - The masked value.
  * **n'** - The masked nonce.
* A signature of the **m** using the sender's key **k<sub>s</sub>**.
* A rangeproof of **C<sub>o</sub>** that also commits to the **m**.

### Output Construction

To create an output for value **v** to a receiver's stealth address (**A<sub>i</sub>**,**B<sub>i</sub>**):

1. Generate a random sender keypair (**k<sub>s</sub>**, **K<sub>s</sub>**). # TODO: Can we generate this deterministically?
2. Derive the nonce **n** as the first 16 bytes of HASH32('N'|**k<sub>s</sub>**)
3. Derive the sending key **s** = HASH32('S'|**A<sub>i</sub>**|**B<sub>i</sub>**|**v**|**n**)
4. Derive the shared secret **e** = HASH32('D'|**s*A<sub>i</sub>**). The view tag **t** is the first byte of **e**.
5. Calculate the receiver's one-time public key **K<sub>o</sub>** = **B<sub>i</sub>** + HASH32('O'|**e**)***G** 
6. Calculate the key exchange pubkey **K<sub>e</sub>** = **s*B<sub>i</sub>**
7. Derive the 64-byte mask **m** = HASH64(**e**)
8. Encrypt the value using **v'** = **v** &oplus; **m[32,39]**
9. Encrypt the nonce using **n'** = **n** &oplus; **m[40,55]**
10. Calculate the commitment **C<sub>o</sub>** = SWITCH(**v**, **m[0,31]**).
11. Generate the rangeproof for **C<sub>o</sub>**, committing also to **m**.
12. Sign **m** using the sender key **k<sub>s</sub>**.


### Output Identification

NOTE: the wallet must keep a map **B<sub>i</sub>**->**i** of all used spend pubkeys and the next few unused ones.

To check if an output belongs to a wallet:

1. Calculate the ECDHE shared secret **e** = HASH32('D'|**a*K<sub>e</sub>**)
2. If the first byte of **e** does not match the view tag **t**, the output does not belong to the wallet.
3. Calculate the one-time spend pubkey: **B<sub>i</sub>** = **K<sub>o</sub>** - HASH('O'|**e**)***G**
4. Lookup the index **i** that generates **B<sub>i</sub>** from the wallet's map **B<sub>i</sub>**->**i**. If not found, the output does not belong to the wallet.
5. Derive the 64-byte mask **m** = HASH64(**e**)
6. Decrypt the value **v** = **v'** &oplus; **m[32,39]** where **m[32,39]** refers to bytes 32-39 (0-based big-endian) of the 64 byte mask **m**.
7. Verify that SWITCH(**v**,**m[0-31]**) ?= **C<sub>o</sub>**
8. Decrypt the nonce **n** = **n'** &oplus; **m[40,55]**.
9. Calculate the send key **s** = HASH32('S'|**A<sub>i</sub>**|**B<sub>i</sub>**|**v**|**n**)
10. Verify that **K<sub>e</sub>** ?= **s*B<sub>i</sub>**.

If all verifications succeed, the output belongs to the wallet, and is safe to use.

The spend key can be recovered by **k<sub>o</sub>** = HASH32('O'|**e**) + **a<sub>i</sub>**.

