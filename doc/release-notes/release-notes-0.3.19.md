There's more work to do on DoS, but I'm doing a quick build of what I have so far in case it's needed, before venturing into more complex ideas.  The build for this is version 0.3.19.

- Added some DoS controls
As Gavin and I have said clearly before, the software is not at all resistant to DoS attack.  This is one improvement, but there are still more ways to attack than I can count.  

I'm leaving the -limitfreerelay part as a switch for now and it's there if you need it.

- Removed "safe mode" alerts
"safe mode" alerts was a temporary measure after the 0.3.9 overflow bug.  We can say all we want that users can just run with "-disablesafemode", but it's better just not to have it for the sake of appearances.  It was never intended as a long term feature.  Safe mode can still be triggered by seeing a longer (greater total PoW) invalid block chain.
