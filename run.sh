#gnome-terminal -e ./use.sh -t "Custom Bitcoin Console"
gnome-terminal -t "Custom Bitcoin Console" -- python3 bitcoin_console.py
if [ ! -d "src" ]
then
    cd ..
fi

if [ -d "/media/sf_Bitcoin" ]
then
    src/bitcoind -datadir=/media/sf_Bitcoin/ -debug=researcher # Virtual machine shared folder
elif [ -d "../blocks" ]
then
    src/bitcoind -datadir=.. -debug=researcher # Super computer shared folder
else
    src/bitcoind -datadir=/media/sim/BITCOIN/ -debug=researcher
fi
