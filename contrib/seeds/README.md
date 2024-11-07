# Seeds

Utility to generate the seeds.txt list that is compiled into the client
(see [src/chainparamsseeds.h](/src/chainparamsseeds.h) and other utilities in [contrib/seeds](/contrib/seeds)).

Be sure to update `PATTERN_AGENT` in `makeseeds.py` to include the current version,
and remove old versions as necessary (at a minimum when SeedsServiceFlags()
changes its default return value, as those are the services which seeds are added
to addrman with).

The seeds compiled into the release are created from sipa's, achow101's and luke-jr's
DNS seed, virtu's crawler, and asmap community AS map data. Run the following commands
from the `/contrib/seeds` directory:

```
curl https://bitcoin.sipa.be/seeds.txt.gz | gzip -dc > seeds_main.txt
curl https://mainnet.achownodes.xyz/seeds.txt.gz | gzip -dc >> seeds_main.txt
curl https://21.ninja/seeds.txt.gz | gzip -dc >> seeds_main.txt
curl https://luke.dashjr.org/programs/bitcoin/files/charts/seeds.txt >> seeds_main.txt
curl https://testnet.achownodes.xyz/seeds.txt.gz | gzip -dc > seeds_test.txt
curl https://raw.githubusercontent.com/asmap/asmap-data/main/latest_asmap.dat > asmap-filled.dat
python3 makeseeds.py -a asmap-filled.dat -s seeds_main.txt > nodes_main.txt
python3 makeseeds.py -a asmap-filled.dat -s seeds_test.txt > nodes_test.txt
# TODO: Uncomment when a seeder publishes seeds.txt.gz for testnet4
# python3 makeseeds.py -a asmap-filled.dat -s seeds_testnet4.txt -m 30000 > nodes_testnet4.txt
python3 generate-seeds.py . > ../../src/chainparamsseeds.h
```
