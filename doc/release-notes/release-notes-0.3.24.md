XBit v0.3.24 is now available for download at
https://sourceforge.net/projects/xbit/files/XBit/xbit-0.3.24/

This is another bug fix release.  We had hoped to have wallet encryption ready for release, but more urgent fixes for existing clients were needed -- most notably block download problems were getting severe.  Wallet encryption is ready for testing at https://github.com/xbit/xbit/pull/352 for the git-savvy, and hopefully will follow shortly in the next release, v0.4.

Notable fixes in v0.3.24, and the main reasons for this release:

F1) Block downloads were failing or taking unreasonable amounts of time to complete, because the increased size of the block chain was bumping up against some earlier buffer-size DoS limits.

F2) Fix crash caused by loss/lack of network connection.

Notable changes in v0.3.24:

C1) DNS seeding enabled by default.

C2) UPNP enabled by default in the GUI client.  The percentage of xbit clients that accept incoming connections is quite small, and that is a problem.  This should help.  xbitd, and unofficial builds, are unchanged (though we encourage use of "-upnp" to help the network!)

C3) Initial unit testing framework.  XBit sorely needs automated tests, and this is a beginning.  Contributions welcome.

C4) Internal wallet code cleanup.  While invisible to an end user, this change provides the basis for v0.4's wallet encryption.
