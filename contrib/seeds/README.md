# Seeds

Utility to generate the seeds.txt list that is compiled into the client
(see [src/chainparamsseeds.h](/src/chainparamsseeds.h) and other utilities in [contrib/seeds](/contrib/seeds)).

The seeds compiled into the release are created from the current protx list, like this:

    dash-cli protx list valid 1 1185193 > protx_list.json
    python3 makeseeds.py < protx_list.json > nodes_main.txt
    python3 generate-seeds.py . > ../../src/chainparamsseeds.h

Make sure to use a recent block height in the "protx list" call. After updating, create a PR and
specify which block height you used so that reviewers can re-run the same commands and verify
that the list is as expected.

## Dependencies

Ubuntu:

    sudo apt-get install python3-pip
    pip3 install dnspython
