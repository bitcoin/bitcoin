
# see ConnectionType enum in net.h
CONN_TYPES = [
    'inbound',
    'outbound-full-relay',
    'manual',
    'feeler',
    'block-relay',
    'addr-fetch'
]

def describe_conn_type(conn_type):
    if conn_type < len(CONN_TYPES):
        return CONN_TYPES[conn_type]
    return 'unknown'
