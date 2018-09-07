#!/bin/bash
# Copyright (c) 2018 The Crown developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Usage: ./crown-server-install.sh [OPTION]...
#
# Setup crown server or update existing one

LATEST_RELEASE="v0.12.5.1"

systemnode=false
masternode=false
help=false
install=false
unknown=()
appname=$(basename "$0")

print_help()
{
echo "Usage: $(basename "$0") [OPTION]...
Setup crown server or update existing one

  -m, --masternode                  create a masternode
  -s, --systemnode                  create a systemnode
  -p, --privkey=privkey             set private key
  -v, --version=version             set version, default will be the latest release
  -h, --help                        display this help and exit

"
}

handle_arguments()
{
    while [[ $# -gt 0 ]]
    do
        key="$1"
        case $key in
            -h|--help)
                help=true
                shift
                ;;
            -m|--masternode)
                masternode=true
                shift
                ;;
            -s|--systemnode)
                systemnode=true
                shift
                ;;
            -p|--privkey)
                privkey="$2"
                shift
                shift
                ;;
            --privkey=*)
                privkey="${key#*=}"
                shift
                ;;
            -v|--version)
                LATEST_RELEASE="$2"
                shift
                shift
                ;;
            --version=*)
                LATEST_RELEASE="${key#*=}"
                shift
                ;;

            *)    # unknown option
                unknown+=("$1") # save it in an array
                shift
                ;;
        esac
    done
    if [ "$help" = true ] ; then
        print_help
        exit 0
    fi

    # Check if there are unknown arguments
    if [ ${#unknown[@]} -gt 0 ] ; then
        printf "$appname: unrecognized option '${unknown[0]}'\nTry '$appname --help' for more information.\n"
        exit 1
    fi

    # Check if only one of the options is set
    if [ "$masternode" = true ] && [ "$systemnode" = true ] ; then
        echo "'-m|masternode' and '-s|--systemnode' options are mutually exclusive."
        exit 1
    fi

    # Check if private key is set and not empty
    if [ ! -z ${privkey+x} ] && [ -z "$privkey" ]; then
        printf "$appname: option '-p|--privkey' requires an argument'\nTry '$appname --help' for more information.\n"
        exit 1
    fi

    # Check if '-m' or '-s' option is set with '-p'
    if [ ! -z "$privkey" ] && [ "$masternode" != true ] && [ "$systemnode" != true ] ; then
        printf "$appname: If private key is set '-m' or '-s' option is mandatory'\nTry '$appname --help' for more information.\n"
        exit 1
    fi

    # If private key is set then install otherwise update
    if [ ! -z "$privkey" ]; then
        install=true
    fi
    echo $LATEST_RELEASE
}

install_dependencies() {
    sudo apt-get install ufw unzip -y
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
    # 32 or 64 bit? If getconf fails we'll assume 64
    BITS=$(getconf LONG_BIT 2>/dev/null)
    if [ $? -ne 0 ]; then
       BITS=64
    fi
    # Change this later to take latest release version.
    wget "https://github.com/Crowndev/crowncoin/releases/download/$LATEST_RELEASE/Crown-Linux$BITS.zip" -O $dir/crown.zip
}

install_package() {
    sudo unzip -d $dir/crown $dir/crown.zip
    cp -f $dir/crown/*/bin/* /usr/local/bin/
    cp -f $dir/crown/*/lib/* /usr/local/lib/
    rm -rf $tmp
}

configure_conf() {
    cd $HOME
    mkdir -p .crown
    sudo mv .crown/crown.conf .crown/crown.bak
    touch .crown/crown.conf
    IP=$(curl http://checkip.amazonaws.com/)
    PW=$(< /dev/urandom tr -dc a-zA-Z0-9 | head -c32;echo;)
    echo "==========================================================="
    pwd
    echo "daemon=1" > .crown/crown.conf 
    echo "rpcallowip=127.0.0.1" >> .crown/crown.conf 
    echo "rpcuser=crowncoinrpc">> .crown/crown.conf 
    echo "rpcpassword="$PW >> .crown/crown.conf 
    echo "listen=1" >> .crown/crown.conf 
    echo "server=1" >>.crown/crown.conf 
    echo "externalip="$IP >>.crown/crown.conf 
    if [ "$systemnode" = true ] ; then
        echo "systemnode=1" >>.crown/crown.conf
        echo "systemnodeprivkey="$privkey >>.crown/crown.conf
    elif [ "$masternode" = true ] ; then
        echo "masternode=1" >>.crown/crown.conf
        echo "masternodeprivkey="$privkey >>.crown/crown.conf
    fi
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
    (crontab -l 2>/dev/null; echo "@reboot /usr/local/bin/crownd") | crontab -
}

main() {
    # Stop crownd (in case it's running)
    /usr/local/bin/crown-cli stop
    # Install Packages
    install_dependencies
    # Download the latest release
    download_package
    # Extract and install
    install_package

    if [ "$install" = true ] ; then
        # Create swap to help with sync
        create_swap
        # Update Repos
        update_repos
        # Create folder structures and configure crown.conf
        configure_conf
        # Configure firewall
        configure_firewall
        # Add cron job to restart crownd on reboot
        add_cron_job
    fi
    # Start Crownd to begin sync
    /usr/local/bin/crownd
}

handle_arguments "$@"
main
