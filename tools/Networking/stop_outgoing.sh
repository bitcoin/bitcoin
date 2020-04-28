iptables -A INPUT -i lo -p tcp -j ACCEPT
iptables -A OUTPUT -o lo -p tcp -j ACCEPT

iptables -A OUTPUT -p tcp -j DROP
iptables -A INPUT -p tcp -j DROP

