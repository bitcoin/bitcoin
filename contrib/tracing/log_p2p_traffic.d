#!/usr/sbin/dtrace -s

#pragma D option quiet

/*
    USAGE:

    dtrace -s contrib/tracing/log_p2p_traffic.d

    This script requires a 'bitcoind' binary compiled with USDT support and the
    'net' tracepoints.
*/

BEGIN
{
    printf("Logging P2P traffic\n")
}

net*:::inbound_message
/execname == "bitcoind"/
{
    self->peer_id = arg0;
    self->peer_addr = copyinstr(arg1);
    self->peer_type = copyinstr(arg2);
    self->msg_type = copyinstr(arg3);
    self->msg_len = arg4;
    printf("inbound  '%s' msg from peer %d (%s, %s) with %d bytes\n", self->msg_type, self->peer_id, self->peer_type, self->peer_addr, self->msg_len);
}

net*:::outbound_message
/execname == "bitcoind"/
{
    self->peer_id = arg0;
    self->peer_addr = copyinstr(arg1);
    self->peer_type = copyinstr(arg2);
    self->msg_type = copyinstr(arg3);
    self->msg_len = arg4;
    printf("outbound '%s' msg to peer %d (%s, %s) with %d bytes\n", self->msg_type, self->peer_id, self->peer_type, self->peer_addr, self->msg_len);
}
