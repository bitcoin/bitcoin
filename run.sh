#gnome-terminal -f ./use.sh -t "Custom Bitcoin Console"
gnome-terminal -t "Custom Bitcoin Console" -- python3 bitcoin_console.py
if [ ! -d src" ]
then
	cd ..
fi

if [ -d /media/sf_Bitcoin/blocks/ ]; then
	echo "datadir = /media/sf_Bitcoin/"
	if [ ! -f /media/sf_Bitcoin/bitcoin.conf ]; then
		echo "server = 1" > /media/sf_Bitcoin/bitcoin.conf
		echo "rpcuser=cybersec" >> /media/sf_Bitcoin/bitcoin.conf
		echo "rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame" >> /media/sf_Bitcoin/bitcoin.conf
		echo "rpcport=8332" >> /media/sf_Bitcoin/bitcoin.conf
	fi
	src/bitcoind -datadir=/media/sf_Bitcoin/ -debug=researcher # Virtual machine shared folder
elif [ -d /media/sf_BitcoinAttacker/blocks/ ]; then
	echo "datadir = /media/sf_BitcoinAttacker/"
    if [ ! -f /media/sf_BitcoinAttacker/bitcoin.conf ]; then
        echo "server = 1" > /media/sf_BitcoinAttacker/bitcoin.conf
        echo "rpcuser=cybersec" >> /media/sf_BitcoinAttacker/bitcoin.conf
        echo "rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame" >> /media/sf_BitcoinAttacker/bitcoin.conf
        echo "rpcport=8332" >> /media/sf_BitcoinAttacker/bitcoin.conf
    fi
	src/bitcoind -datadir=/media/sf_BitcoinAttacker/ -debug=researcher
elif [ -d /media/sf_BitcoinVictim/blocks/ ]; then
	echo "datadir = /media/sf_BitcoinVictim/"
    if [ ! -f /media/sf_BitcoinVictim/bitcoin.conf ]; then
        echo "server = 1" > /media/sf_BitcoinVictim/bitcoin.conf
        echo "rpcuser=cybersec" >> /media/sf_BitcoinVictim/bitcoin.conf
        echo "rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame" >> /media/sf_BitcoinVictim/bitcoin.conf
        echo "rpcport=8332" >> /media/sf_BitcoinVictim/bitcoin.conf
    fi
	src/bitcoind -datadir=/media/sf_BitcoinVictim/ -debug=researcher
elif [ -d /media/sim/BITCOIN/blocks/ ]; then
	echo "datadir = /media/sim/BITCOIN/"
    if [ ! -f /media/sim/BITCOIN/bitcoin.conf ]; then
        echo "server = 1" > /media/sim/BITCOIN/bitcoin.conf
        echo "rpcuser=cybersec" >> /media/sim/BITCOIN/bitcoin.conf
        echo "rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame" >> /media/sim/BITCOIN/bitcoin.conf
        echo "rpcport=8332" >> /media/sim/BITCOIN/bitcoin.conf
    fi
	src/bitcoind -datadir=/media/sim/BITCOIN/ -debug=researcher
else
	echo "datadir = ~/.bitcoin"
    if [ ! -f ~/.bitcoin/bitcoin.conf ]; then
        echo "server = 1" > ~/.bitcoin/bitcoin.conf
        echo "rpcuser=cybersec" >> ~/.bitcoin/bitcoin.conf
        echo "rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame" >> ~/.bitcoin/bitcoin.conf
        echo "rpcport=8332" >> ~/.bitcoin/bitcoin.conf
    fi
	echo "Running without blockchain (keeping only 550 blocks)"
	src/bitcoind -debug=researcher -prune=550
fi
