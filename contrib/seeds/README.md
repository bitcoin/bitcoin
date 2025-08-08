# Seeds

Utility to generate the seeds.txt list that is compiled into the client
(see [src/chainparamsseeds.h](/src/chainparamsseeds.h) and other utilities in [contrib/seeds](/contrib/seeds)).

Be sure to update `PATTERN_AGENT` in `makeseeds.py` to include the current version,
and remove old versions as necessary (at a minimum when SeedsServiceFlags()
changes its default return value, as those are the services which seeds are added
to addrman with).

Update `MIN_BLOCKS` in  `makeseeds.py` and the `-m`/`--minblocks` arguments below, as needed.

The seeds compiled into the release are created from BitGold's DNS seed and
asmap community AS map data. Run the following commands from the
`/contrib/seeds` directory:

```
curl https://seed.bitgold.org/seeds.txt.gz | gzip -dc > seeds_main.txt
curl https://testnet-seed.bitgold.org/seeds.txt.gz | gzip -dc > seeds_test.txt

curl https://raw.githubusercontent.com/asmap/asmap-data/main/latest_asmap.dat > asmap-filled.dat
python3 makeseeds.py -a asmap-filled.dat -s seeds_main.txt > nodes_main.txt
python3 makeseeds.py -a asmap-filled.dat -s seeds_test.txt > nodes_test.txt

python3 generate-seeds.py . > ../../src/chainparamsseeds.h
```
