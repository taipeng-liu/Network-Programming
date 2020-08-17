
#bring up
sudo iptables -I OUTPUT -s 10.1.1."$1" -d 10.1.1."$2" -j ACCEPT ; sudo iptables -I OUTPUT -s 10.1.1."$2" -d 10.1.1."$1" -j ACCEPT
