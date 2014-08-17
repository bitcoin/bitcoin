from jsonrpc import JSONRPCException, ServiceProxy

MaxProcessSum = 200000   # Maximum amount of coins to merge
MaxOutputSum = 500       # Maximum transaction value
MaxInputSum = 50         # Maximum input value, inputs with greater size will be ignored

access = ServiceProxy("http://alexd:123456789@127.0.0.1:8344")   # http://username:password@host:port/

try:
    balance = access.getbalance()
    print 'Balance = ', balance

    if balance > MaxProcessSum:
        print 'Balance is above MaxProcessSum, setting amount to ', MaxProcessSum
        balance = MaxProcessSum

        if balance > MaxOutputSum:
            access.mergecoins(balance, MaxOutputSum, MaxInputSum)
except JSONRPCException,e:
    print 'Error: %s' % e.error


