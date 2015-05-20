0.11.1 Release notes
====================

Darkcoin Core 0.11.1 supports a full implementation of InstantX, Darksend improvements
and a new version of enforcement compatible with the newer Bitcoin architechure.
Latest release in 0.11.1.x tree is v0.11.1.26, which is rebranding Darkcoin to Dash.

- Fully implemented IX
- Added support for DSTX messages, as a result DS should be much faster
- Clear vValue in SelectCoinsMinConf - should fix an issue with conflicted txes
- "Debug window" -> "Tools window" renaming
- "Last Darksend message" text added in overview page
- Many new languages are supported, such as German, Vietnamese, Spanish
- Fixed required maturity of coins before sending
- New masternode payments enforcement implementation
- Added support to ignore IX confirmations when needed
- Added --instantxdepth, which will show X confirmations when a transaction lock is present
- fix coin control crash https://github.com/bitcoin/bitcoin/pull/5700
- always get only confirmed coins by AvailableCoins for every DS relative action
- New languages supported Portuguese, German, Russian, Polish, Spanish, Vietnamese, French,
Italian, Catalan, Chinese, Danish, Finnish, Swedish, Czech, Turkish and Bavarian (and many more)
- Full implementation of spork. Currently this includes 4 different sporks, InstantX, InstantX block enforcement, masternode payments enforcement, and reconverge. This uses the inventory system, so it's super efficient and very powerful for future development. Reconverge will put the blockchain into a mode where it will attempt to reprocess any forks without restarting the client, this means if we even had a fork in the future this can be triggered to fix it without any damage to the network.
- Masternode payments now uses the inventory system, which will result in much lower bandwidth usage
- Improved caching
- use wallet db ds flag in rpc
- update tx types in UI / fix tx decomposition / use wallet db ds flag
- use wallet db to save ds flag / debug
- use AvailableCoinsType instead of string in walletmodel
- New inventory system for IX messaging, providing super fast/low bandwidth IX communication

Thanks to who contributed to this release, at least:

- eduffield
- Vertoe
- UdjinM6
- Holger Schinzel
- Raze
- Mario Müller
- Crowning
- Alexandre Devilliers
- Stuart Buck
- Tiago Serôdio
- Slawek
- Moli
- Lukas Jackson
- Snogcel
- Jimbit
- Coingun
- Sub-Ether