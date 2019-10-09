The maxsendbuffer bug (0.3.20.1 clients not being able to download the block chain from other 0.3.20.1 clients) was only going to get
worse as people upgraded, so I cherry-picked the bug fix and created a minor release yesterday.

The Amazon Machine Images I used to do the builds are available:

  ami-38a05251   Talkcoin-v0.3.20.2 Mingw    (Windows; Administrator password 'talkcoin development')
  ami-30a05259   Talkcoin_0.3.20.2 Linux32
  ami-8abc4ee3   Talkcoin_0.3.20.2 Linux64

(mac build will be done soon)

If you have already downloaded version 0.3.20.1, please either add this to your talkcoin.conf file:

  maxsendbuffer=10000
  maxreceivebuffer=10000

... or download the new version.
