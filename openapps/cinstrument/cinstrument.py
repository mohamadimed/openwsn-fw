import os
import sys
import time
here = sys.path[0]
print here
sys.path.insert(0,os.path.join(here,'..','..','..','coap'))

from openvisualizer.eventbus.eventbusclient import EventBusClient
from coap import coap
import netmanager
import signal
import struct
import json

import logging
import threading


SLOTDURATION = .01
BATTERY_WH=8.2

class Cinstrument(EventBusClient):

    def __init__(self):
        pass

    def init_mote(self,id_mote,ip_mote):
        return {
                            'mote_id'         : id_mote,
                            'mote_ip'         : 'bbbb::'+ip_mote,#'bbbb::1415:92cc:0:2'
                            'mote_eui64'      : ip_mote
                            }

#-------------------------RUN File-------------------------#



#DO you want to process PDR? if so: type the number of packets to send and the interval in-between
process_pdr = 0
nb_packets = 10
interval = 1 #second

offset = 0

#Destination mote (if want to set manually)
#this has been used before I get to print the liste of reachable motes downward
#dst = [0, 18, 75, 0, 20, 181, 211, 47]
dst = [0x00, 0x12, 0x4b, 0x00, 0x14, 0xb5, 0xd3, 0x2f]
MOTE_IP = 'bbbb:0:0:0:12:4b00:14b5:d32f'
#------------Starting CoAP communication----------#


UDPPORT = 61618 #61618 can't be the port used in OV

c = coap.coap(udpPort=UDPPORT)
print('----------------')
print('Sending CoAP PUT')
print('----------------')
# read the information about the mote status and recorde time befor and after receiving the ack
x = time.time()

"""
    Payload format for Track creation. This packet should be created by 
     the PCE and sent to every concerened node in the mesh network


  1 Byte      1 Byte       2 Bytes    2 Bytes    1Byte    2 Bytes        1 Byte       1Byte
+---------+------------+-----------+-----------+--------+-----------+------------+-----------+
|Track ID |Sub_Track ID|Ingress adr|Egress adr |Bundle n|Parent adr |Parent radio|add 1/del 0|
+---------+------------+-----------+-----------+--------+-----------+------------+-----------+

"""

                                                                 #0/1/2 = O-QPSK/FSK/OFDM

"""                                                                            #we must assume that the bundle_length should same when performing an update to ensure correct function, otherwise we must delete the track vars and create it all new
p = c.PUT('coap://[{0}]/ci'.format(MOTE_IP),True,[],[1,1,0xd3, 0x2f,0xd3, 0x19,2,0xd3, 0x19,0,0]) #PUT function has been modified in coap.py to allow insertion of payload

print('{0}'.format(p))
time.sleep(2)

#        __d32f
#d319 --|__d282--d34a  for this topology the following req. work well

p = c.PUT('coap://[{0}]/ci'.format(MOTE_IP),True,[],[1,2,0xd3, 0x2f,0xd3, 0x19,2,0xd3, 0x19,1,0])
"""
                                                                   #d366->d32f OFDM (1)
p = c.PUT('coap://[{0}]/ci'.format('bbbb:0:0:0:12:4b00:14b5:d366'),True,[],[1,1,0xd3, 0x66,0xd3, 0x19,1,0xd3, 0x2f,2,1])
print('{0}'.format(p))   
time.sleep(5)
                                                                        #d32f->d319 OFDM (1)
# p = c.PUT('coap://[{0}]/ci'.format(MOTE_IP),True,[],[1,1,0xd2, 0x82,0xd3, 0x19,1,0xd3, 0x19,2,0])
# print('{0}'.format(p))
# time.sleep(5)

                                                                        #d366->d319 FSK (1)
p = c.PUT('coap://[{0}]/ci'.format('bbbb:0:0:0:12:4b00:14b5:d366'),True,[],[1,2,0xd3, 0x66,0xd3, 0x19,1,0xd3, 0x19,1,1]) 
print('{0}'.format(p))
time.sleep(5)


                                                                        #d32f->d319 QPSK (1)
p = c.PUT('coap://[{0}]/ci'.format('bbbb:0:0:0:12:4b00:14b5:d32f'),True,[],[1,2,0xd3, 0x66,0xd3, 0x19,1,0xd3, 0x19,0,1]) 
print('{0}'.format(p))



#p = c.PUT('coap://[{0}]/ci'.format(MOTE_IP),True,[],[2]) #PUT function has been modified in coap.py to allow insertion of payload

#p = c.DELETE('coap://[{0}]/ci'.format(MOTE_IP))
x = time.time()-x
print("--Request Time (Latency): {0:.2f} ms\n".format(x*1000))

#close UDPport
c.close()

#print ''.join([chr(b) for b in p])
print('{0}'.format(p))

#----------------Init packet and process its content--------------#


while True:
        input = raw_input("Done. Press q to close. ")
        if input=='q':
            print 'bye bye.'
            #c.close()
            os.kill(os.getpid(), signal.SIGTERM)







