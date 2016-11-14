alias bclir="bitcoin-cli -regtest"

bclir stop

echo "Warning, do not run bitcoind in the background"
wait `pgrep bitcoind`
sleep 0.5

bitcoind -daemon -regtest

while [ 1 ]; do
  if [ -n "`bitcoin-cli getinfo 2>1 | grep -i '{'`" ]; then
    break;
  fi
  echo 'sleep'
  sleep 0.1;
done

echo "bitcoin up"

bclir getinfo

if [ "`bitcoin-cli getblockcount`" -eq "0" ]; then
  bclir generate 200 > /dev/null
  bclir getinfo
fi

echo "testing nulldatas"

bclir getnulldatas `bclir getblockhash 0`
bclir getmanynulldatas 0 1 false
bclir getmanynulldatas 0 1 true

echo "testing sendnulldata"

txout=$(bclir getrawtransaction `bclir sendnulldata 00`)

echo $txout
bclir decoderawtransaction $txout

echo "Confirming..."

bclir generate 1
bclir getnulldatas `bclir getbestblockhash`
