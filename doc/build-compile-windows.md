# Install WSL on non-system drive (D:\, F:\...)
## 1. Enable windows subsystem for Linux system feature  
Open **Powershell** as admin  
*Run this command :*  
        
        Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux 
## 2. Download and install a Linux distribution (example : Ubuntu 18.04)
*Run this command :*  
        
        Invoke-WebRequest -Uri https://aka.ms/wsl-ubuntu-1804 -OutFile Ubuntu.appx -UseBasicParsing 

*Unpack the zip file*  
        
        move .\Ubuntu.appx .\Ubuntu.zip 
        Expand-Archive .\Ubuntu.zip```*

*Initialize Linux distro*  
        
        cd .\Ubuntu\  
        .\ubuntu1804.exe 

Once the instllation is done, you would have something like :  
        
        PS D:\WSL\Ubuntu> ls   

            Repository : D:\WSL\Ubuntu


    Mode                 LastWriteTime         Length Name
    ----                 -------------         ------ ----
    d-----        20/06/2021     11:40                AppxMetadata
    d-----        20/06/2021     11:40                Assets
    da----        20/06/2021     11:41                rootfs
    d-----        20/06/2021     11:41                temp
    -a----        22/05/2019     08:15         219038 AppxBlockMap.xml
    -a----        22/05/2019     08:15           3851 AppxManifest.xml
    -a----        22/05/2019     08:15          11209 AppxSignature.p7x
    -a---l        20/06/2021     11:42              0 fsserver
    -a----        22/05/2019     08:15      231179584 install.tar.gz
    -a----        22/05/2019     08:15           5400 resources.pri
    -a----        22/05/2019     08:15         211968 ubuntu1804.exe
    -a----        22/05/2019     08:15            744 [Content_Types].xml  
    ```

# Compile Bitcoin on Ubuntu 18.04  

### 1. After installing, update and upgrade the system

        sudo apt-get update
        sudo apt-get upgrade


### 2. Install dependencies

        sudo apt-get install build-essential libtool autotools-dev autoconf pkg-config libssl-dev
        sudo apt-get install libboost-all-dev
        sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler
        sudo apt-get install libqrencode-dev autoconf openssl libssl-dev libevent-dev
        sudo apt-get install libminiupnpc-dev


### 3. Download Bitcoin source code

        cd ~
        git clone https://github.com/bitcoin/bitcoin.git


### 4. Download & install Berkeley DB
    
PS : Do not forget to replace the **"username"** below by yours.

        cd ~
        mkdir bitcoin/db4/
        wget 'http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz'
        tar -xzvf db-4.8.30.NC.tar.gz
        cd db-4.8.30.NC/build_unix/
        ../dist/configure --enable-cxx --disable-shared --with-pic --prefix=/home/theusername/bitcoin/db4/
        make install


### 5. Compile Bitcoin with Berkeley DB

PS : Do not forget to replace the **"username"** below by yours.  

        cd ~/bitcoin/
        ./autogen.sh
        ./configure LDFLAGS="-L/home/theusername/bitcoin/db4/lib/" CPPFLAGS="-I/home/theusername/bitcoin/db4/include/"

### 6. Build Bitcoin

This command below may take **5-10 minutes**  

        make -s -j5

### 7. Check everything is ok by trying to access below directory and the binaries

        cd ~/bitcoin/
        ./src/bitcoind
        ./src/bitcoin-qt
        ./src/bitcoin-cli
