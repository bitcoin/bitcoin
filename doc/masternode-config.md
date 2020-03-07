# Masternode and Systemnode Configuration

It is possible to control any number of masternodes and/or systemnodes from a single wallet. The wallet needs to have a valid collateral output of 10000 coins for each masternode and 500 coins for each systemnode. 

There are two ways to create and control masternodes and systemnodes: using GUI features of the Crown wallet or manually editing `masternode.conf` or `systemnode.conf` and running console commands. There are instructions for the setup through GUI:
* [Masternode Setup Guide](https://forum.crownplatform.com/index.php?topic=1241.0)
* [Systemnode Setup Guide](https://forum.crownplatform.com/index.php?topic=1240.0)

The rest of the document describes the format of the configuration file and console commands.

## Masternodes

### Configuration File Placement

Masternode configuration file is a file named `masternode.conf` and placed in the following directory:
 * Windows: %APPDATA%\Crown\
 * Mac OS: ~/Library/Application Support/Crown/
 * Unix/Linux: ~/.crown/

### Configuration File Format

The server node configuration file format is spaced separated text. Each line consisting of an alias, IP address & port, masternode private key, collateral output transaction id, collateral output index, and donation address with donation percentage (the latter two are optional and should be in format "address:percentage" or just "address" if percentage=100%). If you don't use a donation address the full reward is sent to the collateral address.

Example:
```
mn1 127.0.0.2:19340 93HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0
mn2 127.0.0.3:19340 93WaAb3htPJEV8E9aQcN23Jt97bPex7YvWfgMDTUdWJvzmrMqey aa9f1034d973377a5e733272c3d0eced1de22555ad45d6b24abadff8087948d4 0 7gnwGHt17heGpG9Crfeh4KGpYNFugPhJdh:33
mn3 127.0.0.4:19340 92Da1aYg6sbenP6uwskJgEY2XWB5LwJ7bXRqc3UPeShtHWJDjDv db478e78e3aefaa8c12d12ddd0aeace48c3b451a8b41c570d0ee375e2a02dfd9 1 7gnwGHt17heGpG9Crfeh4KGpYNFugPhJdh
```

In the example above:
* the collateral for mn1 consists of transaction *2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c*, output index 0 has amount 10000
* masternode 2 will donate 33% of its income
* masternode 3 will donate 100% of its income

### Console Commands

The following RPC subcommands can be used with `masternode` command:
* `list-conf`:  shows the parsed masternode.conf
* `start-alias <alias>`: start a single masternode
* `stop-alias <alias>`: stop a single masternode
* `start-many`: start all masternodes
* `stop-many`: stop all masternodes
* `outputs`: list available collateral output transaction ids and corresponding collateral output indexes

## Systemnodes

Systemnode configuration can be done similarly (with adjustment to the amount of collateral transaction). Systemnode configuration file must be named `systemnode.conf` and placed in the directory specified above; it has the same server node configuration file format.

Command `systemnode` supports the same set of subcommands to control systemnodes.