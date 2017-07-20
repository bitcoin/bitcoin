# **HODLcoin** [![Build Status](https://travis-ci.org/HOdlcoin/HOdlcoin.svg?branch=HODLCoin0.11.3)](https://travis-ci.org/HOdlcoin/HOdlcoin) [![GitHub version](https://badge.fury.io/gh/HOdlcoin%2FHOdlcoin.svg)](https://badge.fury.io/gh/HOdlcoin%2FHOdlcoin)
<img src="http://i.imgur.com/5EAaA7b.png" width="400">

[http://www.HOdlcoin.com](http://www.HOdlcoin.com)
### **What is it?**

**HOdlcoin** is a Cryptocurrency just like Bitcoin, but with one major difference. It pays interest on every balance. This is to recognize the importance of **HODLers** and properly reward them for **HODLing**.

## **Compound Interest on all Balances**

Interest is paid on all outputs (**Balances**) and compounded on each block. This is to discourage rolling outputs into new blocks simply to compound interest.

#### **Benefits:**
> -  Exciting for HOdlers to see their balances continually increase as each block is received.
> -  Interest discourages users from leaving large balances on exchanges - expensive to maintain large sell walls.
> -  Interest encourages users to keep their balances, reducing supply
> -  30 day limit will dilute abandoned balances, reducing supply
> -  30 day limit encourages term deposits, reducing supply
> -  Encourages exchanges to trade in the coin, as they can earn interest on customer's deposits
> -  Unconfirmed transactions become more valuable over time, as their inputs continue to earn interest. 

## **Deposit Interest** 
Paid on **Term Deposits** (aka Fixed deposit / time deposit)

This allows users to lock up funds for a specified amount of time up to a year, to earn a bonus rate of interest.  

#### **Why?**

 - This encourages and reward HOdlers.
 - Term deposits also constrain supply - term deposit coins cannot be moved until term ends.

Interest is handled in the blockchain and protocol using **CHECKLOCKTIMEVERIFY**. There is no counterparty. 

## **Fixed Parameters**
> The PoW Algorithm is considered a technical detail and is subject to change to favor CPU and consumer grade hardware with the intention of keeping mining participatory and distributed.

##### **ALL OTHER PARAMETERS ARE SET IN STONE.**

> **Note:** No changes to mining subsidies, interest rates, distribution etc.

 - 154 second blocks
 - 50 HODL subsidy per block (Halving every 4 years)
 - Total of 81,962,100 HODL will be mined
 - Scrupulously fair launch. No Premine, Instamine, Ninja
 - 1 HODL token subsidy for first 100 blocks
 - Standard interest payment 5% on **all** outputs for up to 30 days
 - Term deposit for 1 year 10% APR

### **Term deposits**

Term     | % of Total APR
-------- | ---
1 week   | ~ 6.5%
1 month  | ~ 25%
3 month  | ~ 60%
6 month  | ~ 87%
1 year   | ~ 100% 


#### **Bonus interest payment for early adopters, decreases over 24 months**

Time     |  APR
-------- | ---
1st month | ~ 2143%
2nd month | ~ 1833%
3rd month | ~ 1540%
...      | 
...      |  
22nd month | ~ 0.111255%
23rd month | ~ 0.008%
24th month | no bonus 

> **Note:**
> 
> - When a Term Deposit matures, it stops earning interest - you need to move it to start earning interest again.
> -  Bonus rates are paid on regular balances as well.
> - The bonus rate is locked at the time of the transaction, the rate you can achieve reduces over time due to the multiplier, but once you're earning that bonus rate, it doesn't reduce.

### **Proof of Work**

#### **1GB AES Pattern Search PoW** 
Pattern Search involves filling up RAM with pseudo-random data, and then conducting a search for the start location of an AES encrypted data pattern in that data. Pattern Search is an evolution of the ProtoShares Momentum PoW, first used in MemoryCoin and later modified for use in CryptoNight(Monero,Bytecoin), Ethash(Ethereum).  CPU/GPU friendly.
