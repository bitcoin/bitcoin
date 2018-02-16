#!/bin/bash
# Copyright (c) 2018 The Crown developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

LATEST_RELEASE="v0.12.3.0"

install_dependencies() {
    sudo apt-get install ufw -y
    sudo apt-get install unzip -y
}

create_swap() {
    sudo mkdir -p /var/cache/swap/   
    sudo dd if=/dev/zero of=/var/cache/swap/myswap bs=1M count=1024
    sudo mkswap /var/cache/swap/myswap
    sudo swapon /var/cache/swap/myswap
    swap_line='/var/cache/swap/myswap   none    swap    sw  0   0'
    # Add the line only once 
    sudo grep -q -F "$swap_line" /etc/fstab || echo "$swap_line" | sudo tee --append /etc/fstab > /dev/null
    cat /etc/fstab
}

update_repos() {
    sudo DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" update
    sudo DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" upgrade
}

download_package() {
    # Create temporary directory
    dir=`mktemp -d`
    if [ -z "$dir" ]; then
        # Create directory under $HOME if above operation failed
        dir=$HOME/crown-temp
        mkdir -p $dir
    fi
    wget "https://github.com/Crowndev/crowncoin/releases/download/$LATEST_RELEASE/Crown-Linux64.zip" -O $dir/crown.zip
}

install_package() {
    sudo unzip -d $dir/crown $dir/crown.zip
    cp $dir/crown/*/bin/* /usr/local/bin/
    cp $dir/crown/*/lib/* /usr/local/lib/
    rm -rf $tmp
}

configure_conf() {
    cd $HOME
    mkdir -p .crown
    sudo mv .crown/crown.conf .crown/crown.bak
    touch .crown/crown.conf
    IP=$(curl http://checkip.amazonaws.com/)
    PW=$(date +%s | sha256sum | base64 | head -c 32 ;)
    echo "==========================================================="
    pwd
    echo "daemon=1" > .crown/crown.conf 
    echo "rpcallowip=127.0.0.1" >> .crown/crown.conf 
    echo "rpcuser=crowncoinrpc">> .crown/crown.conf 
    echo "rpcpassword="$PW >> .crown/crown.conf 
    echo "listen=1" >> .crown/crown.conf 
    echo "server=1" >>.crown/crown.conf 
    echo "externalip="$IP >>.crown/crown.conf 
    echo "systemnode=1" >>.crown/crown.conf
    echo "systemnodeprivkey="$privkey >>.crown/crown.conf
    cat .crown/crown.conf
}

configure_firewall() {
    sudo ufw allow ssh/tcp
    sudo ufw limit ssh/tcp
    sudo ufw allow 9340/tcp
    sudo ufw logging on
    sudo ufw --force enable
}

add_cron_job() {
    (crontab -l 2>/dev/null; echo "@reboot sudo /usr/local/bin/crownd") | crontab -
}

main() {
    # Stop crownd (in case it's running)
    sudo crown-cli stop
    # Create swap to help with sync
    create_swap
    # Update Repos
    update_repos
    # Install Packages
    install_dependencies
    # Download the latest release
    download_package
    # Extract and install
    install_package
    # Create folder structures and configure crown.conf
    configure_conf
    # Configure firewall
    configure_firewall
    # Add cron job to restart crownd on reboot
    add_cron_job
    # Start Crownd to begin sync
    sudo crownd
}

privkey=$1
main
