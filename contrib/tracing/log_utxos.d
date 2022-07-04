#!/usr/sbin/dtrace -s

#pragma D option quiet

/*

   USAGE:

    dtrace -s contrib/tracing/log_utxos.d

    This script requires a 'bitcoind' binary compiled with USDT support and the
    'utxocache' tracepoints.

*/

BEGIN
{
    printf("%-7s %-71s %16s %7s %8s\n",
          "OP", "Outpoint", "Value", "Height", "Coinbase");
}

/*
  Attaches to the 'utxocache:{add, spent, uncache}' tracepoints and prints additions/spents to/from the UTXO set cache
  and uncache UTXOs from the UTXO set cache.
*/
utxocache*:::add, utxocache*:::spent, utxocache*:::uncache
/execname == "bitcoind"/
{
    txid = (char *)copyin(arg0, 32);
    self->index = arg1;
    height = arg2;
    value = arg3;
    isCoinbase = arg4;

    self->event = probename == "add" ? "Added" : probename == "spent" ? "Spent" : "Uncache";

    printf("%s   ", self->event);
    printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        txid[31], txid[30], txid[29], txid[28], txid[27], txid[26], txid[25], txid[24],
        txid[23], txid[22], txid[21], txid[20], txid[19], txid[18], txid[17], txid[16],
        txid[15], txid[14], txid[13], txid[12], txid[11], txid[10], txid[9], txid[8],
        txid[7], txid[6], txid[5], txid[4], txid[3], txid[2], txid[1], txid[0]);

    printf(":%-6d %16ld %7d %s\n", self->index, value, height, (isCoinbase ? "Yes" : "No" ));
}
