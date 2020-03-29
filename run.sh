#gnome-terminal -e ./use.sh -t "Custom Bitcoin Console"
gnome-terminal -t "Custom Bitcoin Console" -- python3 bitcoin_console.py
if [ ! -d "src" ]
then
    cd ..
fi

if [ -d "/media/sf_Bitcoin/blocks/" ]
then
	echo "datadir = /media/sf_Bitcoin/"
    src/bitcoind -datadir=/media/sf_Bitcoin/ -debug=researcher # Virtual machine shared folder
elif [ -d "/media/sf_BitcoinAttacker/blocks/" ]
then
	echo "datadir = /media/sf_BitcoinAttacker/"
    src/bitcoind -datadir=/media/sf_BitcoinAttacker/ -debug=researcher
elif [ -d "/media/sf_BitcoinVictim/blocks/" ]
then
	echo "datadir = /media/sf_BitcoinVictim/"
    src/bitcoind -datadir=/media/sf_BitcoinVictim/ -debug=researcher
elif [ -d "/media/sim/BITCOIN/blocks/" ]
then
	echo "datadir = /media/sim/BITCOIN/"
    src/bitcoind -datadir=/media/sim/BITCOIN/ -debug=researcher
else
	echo "datadir = ~/.bitcoin"
    echo "Running without blockchain (keeping only 550 blocks)"
    src/bitcoind -debug=researcher -prune=550
fi
