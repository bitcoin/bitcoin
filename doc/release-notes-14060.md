Configuration
-------------

The outbound message high water mark of the ZMQ PUB sockets are now
configurable via the options:

`-zmqpubhashtxhwm=n`

`-zmqpubhashblockhwm=n`

`-zmqpubrawblockhwm=n`

`-zmqpubrawtxhwm=n`

Each high water mark value must be an integer greater than or equal to 0.
The high water mark limits the maximum number of messages that ZMQ will
queue in memory for any single subscriber. A value of 0 means no limit.
When not specified, the default value continues to be 1000.
When a ZMQ PUB socket reaches its high water mark for a subscriber, then
additional messages to the subscriber are dropped until the number of
queued messages again falls below the high water mark value.
