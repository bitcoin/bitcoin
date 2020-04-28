if [ ! -d "src" ]; then
	cd .. # If located in src or any other level of depth 1, return
fi

pruned="false"
if [ -d "/media/sf_Bitcoin/blocks/" ]; then
	dir="/media/sf_Bitcoin"
elif [ -d "/media/sf_BitcoinAttacker/blocks/" ]; then
	dir="/media/sf_BitcoinAttacker"
elif [ -d "/media/sf_BitcoinVictim/blocks/" ]; then
	dir="/media/sf_BitcoinVictim"
elif [ -d "/media/sim/BITCOIN/blocks/" ]; then
	dir="/media/sim/BITCOIN"
else
	dir="$HOME/.bitcoin/"
	pruned="true"
fi

echo "datadir = $dir"

if [ ! -f "$dir/bitcoin.conf" ]; then
	echo "Resetting configuration file"
	echo "server=1" > "$dir/bitcoin.conf"
	echo "rpcuser=cybersec" >> "$dir/bitcoin.conf"
	echo "rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame" >> "$dir/bitcoin.conf"
	echo "rpcport=8332" >> "$dir/bitcoin.conf"
fi

if [ "$1" == "gui" ]; then
	if [ "$pruned" == "true" ]; then
		echo "Pruned mode activated, only keeping 550 block transactions"
		echo
		src/qt/bitcoin-qt -prune=550 -datadir="$dir" -debug=researcher
	else
		echo
		src/qt/bitcoin-qt -datadir="$dir" -debug=researcher
	fi
else
	if [ "$pruned" == "true" ]; then
		echo "Pruned mode activated, only keeping 550 block transactions"
		echo
		# Only open the console if not already open
		if ! wmctrl -l | grep -q "Custom Bitcoin Console"; then
			gnome-terminal -t "Custom Bitcoin Console" -- python3 bitcoin_console.py
		fi
		src/bitcoind -prune=550 -datadir="$dir" -debug=researcher
	else
		echo
		# Only open the console if not already open
		if ! wmctrl -l | grep -q "Custom Bitcoin Console"; then
			gnome-terminal -t "Custom Bitcoin Console" -- python3 bitcoin_console.py
		fi
		src/bitcoind -datadir="$dir" -debug=researcher
	fi
fi
