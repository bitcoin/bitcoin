# Crown Installation Guide.
### Script works with most popular x64 and x32 operating systems

The Crown development team created a useful script to help everyone easily install a Crown client on their VPS server.

Here it is

```
https://github.com/Crowndev/crown-core/blob/master/scripts/crown-server-install.sh
```

There are various ways of executing this script for example, for Masternodes, Systemnodes or just to have a 
full node for use with applications or development. 

## Full node.

For the most basic installation of Crown, use this command
```
sudo apt-get install curl -y && curl -s https://raw.githubusercontent.com/Crowndev/crowncoin/master/scripts/crown-server-install.sh | bash -s
```

This will provide you with the latest Crown daemon and nothing else.

Once the script has finished, Crown will ask for rpc details to be entered into the crown.conf file.
Use this process for most VPS installations of Crown.

```
sudo nano /root/.crown/crown.conf
```

```
server=1
listen=1
rpcuser=RPCUSER
rpcpassword=RPCPASS
```

Save and exit crown.conf.

## Expanding on the Crown install script

Within the script code we find extra functionality for us to utilise
```
  -m, --masternode           create a masternode
  -s, --systemnode           create a systemnode
  -p, --privkey=privkey      set private key
  -v, --version=version      set version, default will be the latest release
  -j, --job=job#             install/update a pipeline build for testing
  -w, --watchdog=level       enable watchdog (1=check running, 2=pre-emptive restart on low memory)
  -c, --wallet               create a wallet
  -b, --bootstrap            download bootstrap
  -h, --help                 display this help and exit
```

To use these functions add to the end of the install script, like this
```
sudo apt-get install curl -y && curl -s https://raw.githubusercontent.com/Crowndev/crowncoin/master/scripts/crown-server-install.sh | bash -s -- -b
```
This will download the latest Crown wallet along with a bootstrap to speed up the syncing process.

## Masternodes

To install a Masternode, use this command
```
sudo apt-get install curl -y && curl -s https://raw.githubusercontent.com/Crowndev/crowncoin/master/scripts/crown-server-install.sh | bash -s -- -m -p PLACE-YOUR-MASTERNODE-GENKEY-HERE
```

Using extra functionality
```
sudo apt-get install curl -y && curl -s https://raw.githubusercontent.com/Crowndev/crowncoin/master/scripts/crown-server-install.sh | bash -s -- -m -b -w -p PLACE-YOUR-MASTERNODE-GENKEY-HERE
```
This adds the bootstrap and watchdog script.

## Systemnodes

To install a Systemnode, use this command
```
sudo apt-get install curl -y && curl -s https://raw.githubusercontent.com/Crowndev/crowncoin/master/scripts/crown-server-install.sh | bash -s -- -s -p PLACE-YOUR-SYSTEMNODE-GENKEY-HERE
```

Using extra functionality
```
sudo apt-get install curl -y && curl -s https://raw.githubusercontent.com/Crowndev/crowncoin/master/scripts/crown-server-install.sh | bash -s -- -s -b -w -p PLACE-YOUR-SYSTEMNODE-GENKEY-HERE
```
This adds the bootstrap and watchdog script.



