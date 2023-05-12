# Per-Peer Message Capture

## Purpose

This feature allows for message capture on a per-peer basis.  It answers the simple question: "Can I see what messages my node is sending and receiving?"

## Usage and Functionality

* Run `bitcoind` with the `-capturemessages` option.
* Look in the `message_capture` folder in your datadir.
  * Typically this will be `~/.bitcoin/message_capture`.
  * See that there are many folders inside, one for each peer names with its IP address and port.
  * Inside each peer's folder there are two `.dat` files: one is for received messages (`msgs_recv.dat`) and the other is for sent messages (`msgs_sent.dat`).
* Run `contrib/message-capture/message-capture-parser.py` with the proper arguments.
  * See the `-h` option for help.
  * To see all messages, both sent and received, for all peers use:
    ```
    ./contrib/message-capture/message-capture-parser.py -o out.json \
    ~/.bitcoin/message_capture/**/*.dat
    ```
  * Note:  The messages in the given `.dat` files will be interleaved in chronological order.  So, giving both received and sent `.dat` files (as above with `*.dat`) will result in all messages being interleaved in chronological order.
  * If an output file is not provided (i.e. the `-o` option is not used), then the output prints to `stdout`.
* View the resulting output.
  * The output file is `JSON` formatted.
  * Suggestion: use `jq` to view the output, with `jq . out.json`
