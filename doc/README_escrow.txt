Bitcoin Multisign a.k.a "Escrow"

Bitcoin multisign are coins that are under the control of multiple
parties.  The initial implementation allows n parties to vote, with
k good signatures needed (k <= n).

This eliminates single points of failure and reduces the trust required
in many transaction use cases.

Use Cases
---------

"Escrow": send money to a multisign coin with three parties - sender,
receiver and an observer.  Require 2 signatures.  If sender and
receiver agree, they can send the coin back to sender, or to receiver.
If they disagree, the observer can break the tie by signing with
the sender or with the receiver.

Immediate payment: send money from sender to a coin with two parties -
sender and payment observer.  The payment observer will only agree to a
single spend of the money, which prevents double spending.  Of course,
the receiver has to trust the observer.  For protection against observer
failure, additional observers can be added.

Secured funds: to increase security of funds, require 2 out of 3
parties to sign for disbursement.  This reduces the probability that funds
will be stolen because more than one party must be compromised for a
successful theft.  This is also useful for storage at third parties
such as exchanges, to reduce the probability of mass theft.  Instead of the
exchange having full control of the funds, funds can be stored requiring
signatures from 2 out of: account holder, exchange, third party observer.

API Calls
---------

sendmultisign <multisignaddr> <amount> [comment] [comment-to]
    <multisignaddr> is of the form <n>,<addr>,<addr...>
    where <n> of the addresses must sign to redeem the multisign
    <amount> is a real and is rounded to the nearest 0.00000001

redeemmultisign <inputtx> <addr> <amount> [<txhex>]
    where <inputtx> is the multisign transaction ID
    <addr> is the destination bitcoin address
    <txhex> is a partially signed transaction
    <amount> is a real and is rounded to the nearest 0.00000001
    the output is either ['partial', <txhex>] if more signatures are needed
    or ['complete', <txid>] if the transaction was broadcast
