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
elif [ -d "/media/sim/BITCOIN/blocks/" ]
then
	echo "datadir = /media/sim/BITCOIN/"
    src/bitcoind -datadir=/media/sim/BITCOIN/ -debug=researcher
else
	echo "datadir = ~/.bitcoin"
    echo "Running without blockchain (keeping only 550 blocks)"
    src/bitcoind -debug=researcher -prune=550
fi
