**Create A Debian Package Installer**

1. Download gitian created bitcoinclassic tar file to bitcoinclassic/contrib/gitian-debian directory:

  ```
  cd bitcoinclassic/contrib/gitian-debian
  wget https://github.com/bitcoinclassic/bitcoinclassic/releases/download/v0.12/bitcoin-0.12-linux64.tar.gz
  ```

2. Execute debian installer build script:
  ```
  ./build.sh
  ```

**Test New Debian Package Installer**

1. Install newly created debian package on test debian system:

  ```
  sudo gdebi bitcoincl-0.12.deb
  ```

2. Verify bitcoin classic daemon installed and started:

  ```
  sudo systemctl status bitcoincld
  ```

3. Add your user account to the bitcoin system group:
   
  ```
  sudo usermod -a -G bitcoin <your username>
  ```
  
4. Logout and back into your account so new group assignment takes affect.

5. Verify your username was added to the bitcoin group:

  ```
  groups
  ```

6. Test bitcoincl-cli access:

  ```
  /usr/bin/bitcoincl-cli -conf=/etc/bitcoincl/bitcoin.conf getinfo
  ```
  
7. Test bitcoincl-qt with non-conflicting IP port:
  
  ```
  bitcoincl-qt -listen=0:8444
  ```
  
8. Uninstall bitcoincl without removing config file or data:

  ```
  sudo apt-get remove bitcoincl
  ```

9. Uninstall bitcoincl AND remove config file and data:

  ```
  sudo apt-get purge bitcoincl
  sudo rm -rf /var/lib/bitcoincl
  ```

**Non-Interactive Installation**

The bitcoincl debian package uses debconf to ask the user if they want to automatically enable and start the bitcoincld service as part of the package installation. To skip this question for non-interactive installs the following instructions allow you to pre-answer the question. This question is only asked the first time the bitcoincl package is installed and only if the target system has the systemd systemctl binary present and executable.

1. Install ```debconf-utils```
 ```
 % sudo apt-get install debconf-utils
 ```

2. Pre-answer the question, ***true*** to automatically enable and start the ```bitcoincld``` service and ***false*** to not automatically enable and start the service during package install
 ```
 % sudo sh -c 'echo "bitcoincl bitcoincl/start_service boolean false" | debconf-set-selections'
 ```
