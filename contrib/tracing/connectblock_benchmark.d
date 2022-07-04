#!/usr/sbin/dtrace -s

#pragma D option quiet

/*
    USAGE:

    dtrace -s contrib/tracing/connectblock_benchmark.d <start height> <end height> <logging threshold in ms>

     - <start height> sets the height at which the benchmark should start. Setting
    the start height to 0 starts the benchmark immediately, even before the
    first block is connected.
    - <end height> sets the height after which the benchmark should end. Setting
        the end height to 0 disables the benchmark.
    - Threshold <logging threshold in ms>.

    This script requires a 'bitcoind' binary compiled with USDT support and the
    'validation:block_connected' USDT.

*/

BEGIN
{
    blocks = 0;
    start_height = $1;
    end_height = $2;
    logging_threshold_ms = $3;

    if (end_height < start_height) {
        printf("Error: start height (%d) larger than end height (%d)!\n", start_height, end_height);
        exit(1);
    }

    if (end_height > 0) {
        printf("ConnectBlock benchmark between height %d and %d inclusive\n", start_height, end_height);
    } else {
        printf("ConnectBlock logging starting at height %d\n", start_height);
    }

    if (logging_threshold_ms > 0) {
        printf("Logging blocks taking longer than %d ms to connect.\n", $3);
    }

    if (start_height == 0) {
        start = timestamp;
    }
}

/*
  Attaches to the 'validation:block_connected' USDT and collects stats when the
  connected block is between the start and end height (or the end height is
  unset).
*/
validation*:::block_connected
/execname == "bitcoind" && arg1 >= start_height && (arg1 <= end_height || end_height == 0 )/
{
    height = arg1;
    transactions = arg2;
    inputs = arg3;
    sigops =  arg4;
    duration = arg5;

    blocks = blocks + 1;
    transactions += transactions;
    inputs += inputs;
    sigops += sigops;

    @durations = quantize(duration / 1000);

    if (height == start_height && height != 0) {
        start = timestamp;
        printf("Starting Connect Block Benchmark between height %d and %d.\n", start_height, end_height);
    }

    if (end_height > 0 && height >= end_height) {
        end = timestamp;
        duration = end - start;
        printf("\nTook %d ms to connect the blocks between height %d and %d.\n", duration / 1000000, start_height, end_height);
        exit(0);
    }
}

/*
  Attaches to the 'validation:block_connected' USDT and logs information about
  blocks where the time it took to connect the block is above the
  <logging threshold in ms>.
*/
validation*:::block_connected
/execname == "bitcoind" && arg5 / 1000 > logging_threshold_ms /
{
    hash = (char *)copyin(arg0, 32);
    height = arg1;
    transactions = arg2;
    inputs = arg3;
    sigops = arg4;
    duration = arg5;

    printf("Block %d ", height);
    /* Prints each byte of the block hash as hex in big-endian (the block-explorer format) */
    /* Note: Loops are not allowed in DTrace scripts */
    printf("( %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x )",
        hash[31], hash[30], hash[29], hash[28], hash[27], hash[26], hash[25], hash[24],
        hash[23], hash[22], hash[21], hash[20], hash[19], hash[18], hash[17], hash[16],
        hash[15], hash[14], hash[13], hash[12], hash[11], hash[10], hash[9], hash[8],
        hash[7], hash[6], hash[5], hash[4], hash[3], hash[2], hash[1], hash[0]);
    printf(" %4d tx  %5d ins  %5d sigops  took %4d ms\n", transactions, inputs, sigops, duration / 1000);
}

/*
  Prints stats about the blocks, transactions, inputs, and sigops processed in
  the last second (if any).
*/
tick-1sec
{
    if (blocks > 0) {
        printf("BENCH %4d blk/s %6d tx/s %7d inputs/s %8d sigops/s (height %d)\n", blocks, transactions, inputs, sigops, height);

        blocks=0;
        transactions=0;
        inputs=0;
        sigops=0;
    }
}

END
{
    printf("\nHistogram of block connection times in milliseconds (ms).\n");
    printa(@durations);
}
