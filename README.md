# ECCPoW - ECCBTC

## What is ECCBTC?

LiberVance develops ECCPoW(ECCPoW, Error Correction Codes Proof-of-Work) that combines the LDPC decoder with a hash function. Error-Correction Codes are used to correct errors in wireless communication. One of the representative codes is the LDPC code. The ASIC of the LDPC decoder is less flexible in implementation due to structure and cost issues. We use this feature to make ASIC implementations very inefficient. ECCPoW generates randomly the PCM utilizing as input to the LDPC decoder for each block. And random PCM generation has the effect of solving a different hash problem every block. We released ECCBTC (Bitcoin hard fork version) with ECCPoW applied. Anyone can download and check the operation. 

![picture1](https://user-images.githubusercontent.com/25213941/57541109-3f3a2600-7389-11e9-9bf4-5170ded0eeaa.jpg)

