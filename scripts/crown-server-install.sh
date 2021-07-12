#!/bin/bash
# Copyright (c) 2018-2021 The Crown developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Usage: ./crown-server-install.sh [OPTION]...
#
# Setup crown server or update existing one

LATEST_RELEASE="0.14.0.2"

systemnode=false
masternode=false
help=false
install=false
unknown=()
appname=$(basename "$0")
watchdog=0

print_help()
{
echo "Usage: $appname [OPTION]...
Setup crown server or update existing one

  -m, --masternode           create a masternode
  -s, --systemnode           create a systemnode
  -p, --privkey=privkey      set private key
  -v, --version=version      set version, default will be the latest release
  -j, --job=job#             install/update a pipeline build for testing
  -w, --watchdog=level       enable watchdog (1=check running, 2=pre-emptive restart on low memory)
  -c, --wallet               create a wallet
  -b, --bootstrap            download bootstrap
  -h, --help                 display this help and exit

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
            -w|--watchdog)
                watchdog="$2"
                shift
                shift
                ;;
            --watchdog=*)
                watchdog="${key#*=}"
                shift
                ;;
            -c|--wallet)
                wallet=true
                shift
                ;;
            -j|--job)
                job="$2"
                shift
                shift
                ;;
            --job=*)
                job="${key#*=}"
                shift
                ;;
            -b|-bootstrap)
                bootstrap=true
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
    if [ -n "$privkey" ] || [ -n "$wallet" ]; then
        install=true
    fi

    # '-j' overrides '-v'
    if [ -n "$job" ]; then
        blah="pipeline job $job"
    else
        blah="version $LATEST_RELEASE"
    fi

    echo "==============================================================================="
    echo
    if [ $install == true ]; then
        echo "About to install Crown $blah"
    else
        echo "About to update to Crown $blah"
    fi
    echo
    echo "==============================================================================="
}

install_dependencies() {
    sudo apt-get install curl ufw unzip wget -y >/dev/null 2>&1
}

create_swap() {
    if [ $(sudo swapon | wc -l) -lt 2 ]; then
	echo    
        echo "There is no swap defined. Adding 2GB of swap space."
        sudo mkdir -p /var/cache/swap/   
        sudo dd if=/dev/zero of=/var/cache/swap/myswap bs=1M count=2048
        sudo chmod 600 /var/cache/swap/myswap
        sudo mkswap /var/cache/swap/myswap
        sudo swapon /var/cache/swap/myswap
        swap_line="/var/cache/swap/myswap   none    swap    sw  0   0"
        # Add the line only once 
        sudo grep -q -F "$swap_line" /etc/fstab || echo "$swap_line" | sudo tee --append /etc/fstab > /dev/null
        echo "The updated /etc/fstab looks like this:"
        cat /etc/fstab
        echo
    echo "==============================================================================="
    fi
}

update_repos() {
    echo
    echo "Making sure the system is up to date (this could take a few minutes)."
    sudo apt-get update >/dev/null 2>&1
    sudo DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" upgrade >/dev/null 2>&1
    sudo apt autoremove -y >/dev/null 2>&1
    echo
    echo "==============================================================================="
}

download_package() {
    echo
    # Create temporary directory
    dir=$(mktemp -d)
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
    if [ -n "$job" ]; then
        # Pull a pipeline build by job number
        echo "Downloading pipeline job $job"
        wget "https://gitlab.crownplatform.com/crown/crown-core/-/jobs/$job/artifacts/download" -O $dir/crown.zip
    else
        # Pull the latest release version.
        echo "Downloading $BITS bit Crown package version $LATEST_RELEASE."
        wget "https://github.com/Crowndev/crowncoin/releases/download/v$LATEST_RELEASE-Emerald/Crown-$LATEST_RELEASE-Linux$BITS.zip" -O $dir/crown.zip
    fi
    wget "https://raw.githubusercontent.com/Crowndev/crowncoin/master/scripts/crownwatch.sh" -O $dir/crownwatch.sh
    if [ -s "$dir/crown.zip" ]; then
        # Download bootstrap if requested
        if [ -n "$bootstrap" ]; then
            echo "Downloading bootstrap"
            wget "https://storage.crownplatform.com/s/erB9Y95HkpA4Nmk/download" -O $dir/bootstrap.zip
	    if [ ! -e "$dir/bootstrap.zip" ]; then
	        echo "Failed to download bootstrap. Continuing without it."
	        bootstrap=
            fi
        fi
    else
        echo "Failed to download package. Aborting."
        exit
    fi	
    echo
    echo "==============================================================================="
}

install_package() {
    echo
    echo "Unzipping and installing the Crown package."
    sudo unzip -qd $dir/crown $dir/crown.zip
    sudo cp -f $dir/crown/*/bin/* /usr/local/bin/
    sudo cp -f $dir/crown/*/lib/* /usr/local/lib/
    sudo chmod +x $dir/crownwatch.sh
    sudo cp -f $dir/crownwatch.sh /usr/local/bin
    if [ -n "$bootstrap" ]; then
	    echo "Unzipping bootstrap (which will take a few minutes) and removing old blockchain"
	rm -rf ~/.crown/{blocks,chainstate,bootstrap.dat.old}
        unzip -od ~/.crown $dir/bootstrap.zip
    fi
    sudo rm -rf $dir
    echo
    echo "==============================================================================="
}

configure_conf() {
    echo
    echo "Creating the Crown datadir and config file."
    cd $HOME
    mkdir -p .crown
    sudo mv .crown/crown.conf .crown/crown.bak
    IP=$(curl http://checkip.amazonaws.com/)
    PW=$(< /dev/urandom tr -dc a-zA-Z0-9 | head -c32;echo;)
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
    echo "addnode=92.60.44.40" >>.crown/crown.conf
    echo "addnode=149.248.53.3" >>.crown/crown.conf
    echo "addnode=139.180.141.215" >>.crown/crown.conf
    echo "This is the config file:"
    cat .crown/crown.conf
    echo
    echo "==============================================================================="
}

configure_firewall() {
    echo
    echo "Configuring base firewall."
    sudo ufw default deny
    sudo ufw allow ssh/tcp
    sudo ufw limit ssh/tcp
    sudo ufw allow 9340/tcp
    sudo ufw logging on
    sudo ufw --force enable
    echo
    echo "which looks like this:"
    sudo ufw status numbered
    echo
    echo "==============================================================================="
}

ensure_cron_line() {
    if [ $(crontab -l 2>/dev/null | grep "$*" | wc -l) -eq 0 ]; then
        (crontab -l 2>/dev/null; echo "$*") | crontab -
    fi
}

add_cron_job() {
    echo
    if [ $watchdog -eq 2 ]; then
	echo "Adding watchdog level 2 (memory checking) to crontab"
        ensure_cron_line "*/15 * * * * /usr/local/bin/crownwatch.sh mem"
    elif [ $watchdog -eq 1 ]; then
	echo "Adding watchdog level 1 (daemon running) to crontab"
        ensure_cron_line "*/15 * * * * /usr/local/bin/crownwatch.sh"
    fi
    echo "Adding start on reboot to crontab"
    echo
    ensure_cron_line "@reboot /usr/local/bin/crownd"
    echo "Your crontab now looks like this:"
    crontab -l
    echo
    echo "==============================================================================="
}

main() {
    # Stop crownd if it's running
    PID=$(pidof crownd)
    if [ -n "$PID" ]; then
        echo "Shutting down the crown daemon."
        /usr/local/bin/crown-cli stop 2>/dev/null
    fi
    # Update Repos
    update_repos
    # Install dependencies
    install_dependencies
    # Download the latest release (and bootstrap if requested)
    download_package

    if [ "$install" = true ] ; then
        # Create swap to help with sync and day-to-day running
        create_swap
        # Create folder structures and configure crown.conf
        configure_conf
        # Configure firewall
        configure_firewall
    fi

    if [ -n "$PID" ]; then
        # Allow up to another 5 minutes for old daemon to shutdown gracefully
        for ((i=0; i<5; i++)); do
            if [[ $(ps -p $PID | wc -l) -lt 2 ]]; then
                break
            fi
            echo "...still waiting for shutdown to complete..."
            sleep 60
        done

        # If it still hasn't shutdown, terminate with extreme prejudice
        if [[ $i -eq 5 ]]; then
            echo "Shutdown still incomplete, killing the daemon."
            kill -9 $PID
            sleep 10
            rm -f ~/.crown/{.lock,crownd.pid}
        fi
    fi	

    # Extract and install
    install_package
    # Start Crownd to begin sync or resume normal operations
    /usr/local/bin/crownd -daemon
    # Visual reassurance
    sleep 2
    echo "Check the timestamp on the following message is within the last few seconds"
    echo "(currently it is $(date))"
    echo "and that the version is the one you expect."
    echo
    grep "Crown version" ~/.crown/debug.log | tail -1
    echo
    # Ensure there is a cron job to restart crownd on reboot and run watchdog
    add_cron_job
}

handle_arguments "$@"
main

