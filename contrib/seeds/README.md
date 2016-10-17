### Seeds ###

Utility to generate the seeds.txt list that is compiled into the client
(see [src/chainparamsseeds.h](/src/chainparamsseeds.h) and [share/seeds](/share/seeds)).

The 512 seeds compiled into the 0.10 release were created from sipa's DNS seed data, like this:

<<<<<<< HEAD
	curl -s http://bitcoin.sipa.be/seeds.txt | makeseeds.py
=======
	curl -s http://crowncoin.sipa.be/seeds.txt | head -1000 | makeseeds.py

The input to makeseeds.py is assumed to be approximately sorted from most-reliable to least-reliable,
with IP:port first on each line (lines that don't match IPv4:port are ignored).
>>>>>>> origin/dirty-merge-dash-0.11.0
