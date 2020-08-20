# ECCPoW - ECCBTC

## What is ECCBTC?

LiberVance develops ECCPoW(ECCPoW, Error Correction Codes Proof-of-Work) that combines the LDPC decoder with a hash function. Error-Correction Codes are used to correct errors in wireless communication. One of the representative codes is the LDPC code. The ASIC of the LDPC decoder is less flexible in implementation due to structure and cost issues. We use this feature to make ASIC implementations very inefficient. ECCPoW generates randomly the PCM utilizing as input to the LDPC decoder for each block. And random PCM generation has the effect of solving a different hash problem every block. We released ECCBTC (Bitcoin hard fork version) with ECCPoW applied. Anyone can download and check the operation. 

![ECCBTC](https://user-images.githubusercontent.com/29197938/90708226-3e09df80-e2d4-11ea-82c3-b0a3f5a146d2.png)
![ECCBTC2](https://user-images.githubusercontent.com/29197938/90708313-6c87ba80-e2d4-11ea-99f3-3fc08fe78021.png)


## ECCBTC Explorer

http://112.169.30.29:8080/


