sudo iptables -P FORWARD DROP # we aren't a router
sudo iptables -A INPUT -i enp0s3 -m iprange --src-range 10.0.2.1-10.0.2.255 -j ACCEPT
sudo iptables -P INPUT DROP # Drop everything we don't accept