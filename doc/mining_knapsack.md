## Description. 

A simplified view of how bitcoin transactions are put into a mining block from free txnmempool is by visiualizing a simplified view of a transaction. Simply put a ```transaction``` can be thought of as consisting of three components ```txn id```, ```addresses```, ```miner fee```. 

What concerns a miner is to pack as many as transactions possible in a ```mining block```. A ```mining block``` has an upper cap to to the 
maximum block-size it can have. It should be noted that each ```transaction``` consumes a certain amount of that block-size since it itself consumes a few bytes of space. 

The miner is successful is he packs transactions in such a way that his ```mining reward``` is maximized keeping the block-size below the maximum cap (say 1000000 bytes). ```Transactions``` can't be added more that once in a block and for simplicity we may assume that no transactions have dependencies. (``` In pratice a DAG, helps```). 

One can easily understand that the above problem is a variation of the famous ```0/1 Knapsack problem```. We want to optimize the ```mining reward``` at ```minimum weight``` of ```block``` possible. 

























