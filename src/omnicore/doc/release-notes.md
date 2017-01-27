Omni Core v0.0.11.3
===================

TODO

This version is built on top of v0.0.11, which is a major release and consensus critical in terms of the Omni Layer protocol rules. An upgrade is mandatory, and highly recommended. Prior releases will not be compatible with new behavior in this release.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues

Table of contents
=================

- [Omni Core v0.0.11.3](#omni-core-v00113)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Notable changes](#notable-changes)
  - xxx
- [Change log](#change-log)
- [Credits](#credits)

Upgrading and downgrading
=========================

How to upgrade
--------------

If you are running Bitcoin Core or an older version of Omni Core, shut it down. Wait until it has completely shut down, then copy the new version of `omnicored`, `omnicore-cli` and `omnicore-qt`. On Microsoft Windows the setup routine can be used to automate these steps.

During the first startup historical Omni transactions are reprocessed and Omni Core will not be usable for approximately 15 minutes up to two hours. The progress of the initial scan is reported on the console, the GUI and written to the `debug.log`. The scan may be interrupted, but can not be resumed, and then needs to start from the beginning.

Downgrading
-----------

Downgrading to an Omni Core version prior 0.0.11 is generally not supported as older versions will not provide accurate information due to the changes in consensus rules.

Compatibility with Bitcoin Core
-------------------------------

Omni Core is based on Bitcoin Core 0.10.4 and can be used as replacement for Bitcoin Core. Switching between Omni Core and Bitcoin Core is fully supported at any time.

Downgrading to a Bitcoin Core version prior 0.10 is not supported due to the new headers-first synchronization.

Notable changes
=====================

TODO
------------------------

TODO

Change log
==========

The following list includes relevant pull requests merged into this release:
```
TODO
```

Credits
=======

Thanks to everyone who contributed to this release, and especially the Bitcoin Core developers for providing the foundation for Omni Core!
