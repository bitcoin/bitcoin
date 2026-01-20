# Managing the Wallet

## 1. Backing Up and Restoring The Wallet

### 1.1 Creating the Wallet

Since version 0.21, Bitcoin Core no longer has a default wallet.
Wallets can be created with the `createwallet` RPC or with the `Create wallet` GUI menu item.

In the GUI, the `Create a new wallet` button is displayed on the main screen when there is no wallet loaded. Alternatively, there is the option `File` ->`Create wallet`.

The following command, for example, creates a descriptor wallet. More information about this command may be found by running `bitcoin-cli help createwallet`.

```
$ bitcoin-cli createwallet "wallet-01"
```

`bitcoin rpc` can also be substituted for `bitcoin-cli`.

By default, wallets are created in the `wallets` folder of the data directory, which varies by operating system, as shown below. The user can change the default by using the `-datadir` or `-walletdir` initialization parameters.

| Operating System | Default wallet directory                                    |
| -----------------|:------------------------------------------------------------|
| Linux            | `/home/<user>/.bitcoin/wallets`                             |
| Windows          | `C:\Users\<user>\AppData\Local\Bitcoin\wallets`             |
| macOS            | `/Users/<user>/Library/Application Support/Bitcoin/wallets` |

### 1.2 Encrypting the Wallet

The `wallet.dat` file is not encrypted by default and is, therefore, vulnerable if an attacker gains access to the device where the wallet or the backups are stored.

Wallet encryption may prevent unauthorized access. However, this significantly increases the risk of losing coins due to forgotten passphrases. There is no way to recover a passphrase. This tradeoff should be well thought out by the user.

Wallet encryption may also not protect against more sophisticated attacks. An attacker can, for example, obtain the password by installing a keylogger on the user's machine.

After encrypting the wallet or changing the passphrase, a new backup needs to be created immediately. The reason is that the keypool is flushed and a new HD seed is generated after encryption. Any bitcoins received by the new seed cannot be recovered from the previous backups.

The wallet's private key may be encrypted with the following command:

```
$ bitcoin-cli -rpcwallet="wallet-01" encryptwallet "passphrase"
```

Once encrypted, the passphrase can be changed with the `walletpassphrasechange` command.

```
$ bitcoin-cli -rpcwallet="wallet-01" walletpassphrasechange "oldpassphrase" "newpassphrase"
```

The argument passed to `-rpcwallet` is the name of the wallet to be encrypted.

Only the wallet's private key is encrypted. All other wallet information, such as transactions, is still visible.

The wallet's private key can also be encrypted in the `createwallet` command via the `passphrase` argument:

```
$ bitcoin-cli -named createwallet wallet_name="wallet-01" passphrase="passphrase"
```

Note that if the passphrase is lost, all the coins in the wallet will also be lost forever.

### 1.3 Unlocking the Wallet

If the wallet is encrypted and the user tries any operation related to private keys, such as sending bitcoins, an error message will be displayed.

```
$ bitcoin-cli -rpcwallet="wallet-01" sendtoaddress "tb1qw508d6qejxtdg4y5r3zarvary0c5xw7kxpjzsx" 0.01
error code: -13
error message:
Error: Please enter the wallet passphrase with walletpassphrase first.
```

To unlock the wallet and allow it to run these operations, the `walletpassphrase` RPC is required.

This command takes the passphrase and an argument called `timeout`, which specifies the time in seconds that the wallet decryption key is stored in memory. After this period expires, the user needs to execute this RPC again.

```
$ bitcoin-cli -rpcwallet="wallet-01" walletpassphrase "passphrase" 120
```

In the GUI, there is no specific menu item to unlock the wallet. When the user sends bitcoins, the passphrase will be prompted automatically.

### 1.4 Backing Up the Wallet

To backup the wallet, the `backupwallet` RPC or the `Backup Wallet` GUI menu item must be used to ensure the file is in a safe state when the copy is made.

In the RPC, the destination parameter must include the name of the file. Otherwise, the command will return an error message like "Error: Wallet backup failed!".

```
$ bitcoin-cli -rpcwallet="wallet-01" backupwallet /home/node01/Backups/backup-01.dat
```

In the GUI, the wallet is selected in the `Wallet` drop-down list in the upper right corner. If this list is not present, the wallet can be loaded in `File` ->`Open Wallet` if necessary. Then, the backup can be done in `File` -> `Backup Wallet…`.

This backup file can be stored on one or multiple offline devices, which must be reliable enough to work in an emergency and be malware free. Backup files can be regularly tested to avoid problems in the future.

If the computer has malware, it can compromise the wallet when recovering the backup file. One way to minimize this is to not connect the backup to an online device.

If both the wallet and all backups are lost for any reason, the bitcoins related to this wallet will become permanently inaccessible.

### 1.5 Backup Frequency

The original Bitcoin Core wallet was a collection of unrelated private keys. If a non-HD wallet had received funds to an address and then was restored from a backup made before the address was generated, then any funds sent to that address would have been lost because there was no deterministic mechanism to derive the address again.

Bitcoin Core [version 0.13](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.13.0.md) introduced HD wallets with deterministic key derivation. With HD wallets, users no longer lose funds when restoring old backups because all addresses are derived from the HD wallet seed.

This means that a single backup is enough to recover the coins at any time. It is still recommended to make regular backups (once a week) or after a significant number of new transactions to maintain the metadata, such as labels. Metadata cannot be retrieved from a blockchain rescan, so if the backup is too old, the metadata will be lost forever.

Wallets created before version 0.13 are not HD and must be backed up every 100 keys used since the previous backup, or even more often to maintain the metadata.

### 1.6 Restoring the Wallet From a Backup

To restore a wallet, the `restorewallet` RPC or the `Restore Wallet` GUI menu item (`File` -> `Restore Wallet…`) must be used.

```
$ bitcoin-cli restorewallet "restored-wallet" /home/node01/Backups/backup-01.dat
```

After that, `getwalletinfo` can be used to check if the wallet has been fully restored.

```
$ bitcoin-cli -rpcwallet="restored-wallet" getwalletinfo
```

The restored wallet can also be loaded in the GUI via `File` ->`Open wallet`.

## Wallet Passphrase

Understanding wallet security is crucial for safely storing your Bitcoin. A key aspect is the wallet passphrase, used for encryption. Let's explore its nuances, role, encryption process, and limitations.

- **Not the Seed:**
The wallet passphrase and the seed are two separate components in wallet security. The seed, or HD seed, functions as a master key for deriving private and public keys in a hierarchical deterministic (HD) wallet. In contrast, the passphrase serves as an additional layer of security specifically designed to secure the private keys within the wallet. The passphrase serves as a safeguard, demanding an additional layer of authentication to access funds in the wallet.

- **Protection Against Unauthorized Access:**
The passphrase serves as a protective measure, securing your funds in situations where an unauthorized user gains access to your unlocked computer or device while your wallet application is active. Without the passphrase, they would be unable to access your wallet's funds or execute transactions. However, it's essential to be aware that someone with access can potentially compromise the security of your passphrase by installing a keylogger.

- **Doesn't Encrypt Metadata or Public Keys:**
It's important to note that the passphrase primarily secures the private keys and access to funds within the wallet. It does not encrypt metadata associated with transactions or public keys. Information about your transaction history and the public keys involved may still be visible.

- **Risk of Fund Loss if Forgotten or Lost:**
If the wallet passphrase is too complex and is subsequently forgotten or lost, there is a risk of losing access to the funds permanently. A forgotten passphrase will result in the inability to unlock the wallet and access the funds.

## Migrating Legacy Wallets to Descriptor Wallets

Legacy wallets (traditional non-descriptor wallets) can be migrated to become Descriptor wallets
through the use of the `migratewallet` RPC. Migrated wallets will have all of their addresses and private keys added to
a newly created Descriptor wallet that has the same name as the original wallet. As Descriptor
wallets do not support having both private keys and watch-only scripts, there may be up to two
additional wallets created after migration. In addition to a descriptor wallet of the same name,
there may also be a wallet named `<name>_watchonly` and `<name>_solvables`. `<name>_watchonly`
contains all of the watchonly scripts. `<name>_solvables` contains any scripts that the wallet
knows but for which it is not watching the corresponding P2(W)SH scripts. If the legacy wallet
contains only watch-only scripts and no private keys, then only the `<name>_watchonly` wallet
will be created and the descriptor wallet with the same name will not be created. Additionally,
the created watch-only descriptor wallet will not have private keys enabled.

Migrated wallets will also generate new addresses differently. While the same BIP 32 seed will be
used, the BIP 44, 49, 84, and 86 standard derivation paths will be used. After migrating, a new
backup of the wallet(s) will need to be created.

Given that there is an extremely large number of possible configurations for the scripts that
Legacy wallets can know about, be watching for, and be able to sign for, `migratewallet` only
makes a best effort attempt to capture all of these things into Descriptor wallets. There may be
unforeseen configurations which result in some scripts being excluded. If a migration fails
unexpectedly or otherwise misses any scripts, please create an issue on GitHub. A backup of the
original wallet can be found in the wallet directory with the name `<name>-<timestamp>.legacy.bak`.

The backup can be restored using the methods discussed in the
[Restoring the Wallet From a Backup](#16-restoring-the-wallet-from-a-backup) section.
