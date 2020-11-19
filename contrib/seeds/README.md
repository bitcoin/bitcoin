# Seeds

Utility to generate the seeds.txt list that is compiled into the client
(see [src/chainparamsseeds.h](/src/chainparamsseeds.h) and other utilities in [contrib/seeds](/contrib/seeds)).

The seeds compiled into the release are created from the current protx list, like this:

```bash
dash-cli protx list valid 1 2018966 > protx_list.json

# Make sure the onion seeds still work!
while IFS= read -r line
do
  address=$(echo $line | cut -d':' -f1)
  port=$(echo $line | cut -d':' -f2)
  nc -v -x 127.0.0.1:9050 -z $address $port
done < "onion_seeds.txt"

python3 makeseeds.py protx_list.json onion_seeds.txt > nodes_main.txt
python3 generate-seeds.py . > ../../src/chainparamsseeds.h
```

Make sure to use a recent block height in the "protx list" call. After updating, create a PR and
specify which block height you used so that reviewers can re-run the same commands and verify
that the list is as expected.

## Dependencies

Ubuntu, Debian:

    sudo apt-get install python3-dnspython

and/or for other operating systems:

    pip3 install dnspython3

See https://dnspython.readthedocs.io/en/latest/installation.html for more information.
