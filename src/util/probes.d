/* Copyright (c) 2022 The Bitcoin Core developers
   Distributed under the MIT software license, see the accompanying
   file COPYING or http://www.opensource.org/licenses/mit-license.php. */

/*

   This file is part of the more involved process of using USDT probes on macOS
   as described in the BUILDING CODE CONTAINING USDT PROBES section in the
   dtrace(1) manpage https://www.unix.com/man-page/mojave/1/dtrace/

   This defines the providers and specifies their probes in order to generate
   the header file `probes.h` with the necessary tracepoints macros.

   Note: The boolean type is not supported by the D language thus `bool` is
   replaced by `uint8_t` on probe's signature.

*/

provider net {
        probe inbound_message(int64_t, char *, char *, char *, int64_t, unsigned char *);
        probe outbound_message(int64_t, char *, char *, char *, int64_t, unsigned char *);
};

provider validation {
        probe block_connected(unsigned char *, int32_t, uint64_t, int32_t, uint64_t, uint64_t);
};

provider utxocache {
        probe flush(int64_t, uint32_t, uint64_t, uint64_t, uint8_t);
        probe add(unsigned char *, uint32_t, uint32_t, int64_t, uint8_t);
        probe spent(unsigned char *, uint32_t, uint32_t, int64_t, uint8_t);
        probe uncache(unsigned char *, uint32_t, uint32_t, int64_t, uint8_t);
};

provider coin_selection {
        probe selected_coins(char *, char *, int64_t, int64_t, int64_t) ;
        probe normal_create_tx_internal(char *, uint8_t, int64_t, int32_t);
        probe attempting_aps_create_tx(char *);
        probe aps_create_tx_internal(char *, uint8_t, uint8_t, int64_t, int32_t);
};
