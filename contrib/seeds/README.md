### Seeds ###

Utility to generate the seeds.txt list that is compiled into the client
(see [src/chainparamsseeds.h](/src/chainparamsseeds.h) and other utilities in [contrib/seeds](/contrib/seeds)).

The 512 seeds compiled into the 0.10 release were created from sipa's DNS seed data, like this:

	curl -s http://bitcoin.sipa.be/seeds.txt | makeseeds.py
