import os
import sys
here = sys.path[0]
print here
import signal
sys.path.insert(0,os.path.join(here,'..','..','..','coap'))

from coap import coap
import struct
import netmanager

#---------------------------CLASS AND METHODS DEF-----------------------#
class Cmonitor():

    DAGRank_List    = [256]
    mote_index_map  = [['d3','19']]
    matrix          = []

    def __init__(self):
       return

    def init_mote(self,id_mote,ip_mote):
        return {
                            'mote_id'         : id_mote,
                            'mote_ip'         : 'bbbb::'+ip_mote,#'bbbb::1415:92cc:0:2'
                            'mote_eui64'      : ip_mote
                            }


    def init_matrix(self,list_of_motes):
        self.matrix = [[[0,0,0]]*(len(list_of_motes)+1) for _ in range(len(list_of_motes)+1)] #+1 for root
        #rssi_values = np.diag([[999,999,999]]*(len(list_of_motes)+1))
        
        # for i in range(len(matrix)):
        #     for j in range(len(matrix)):
        #         if (i==j):
        #             matrix[i][j]= [999,999,999]

        print self.matrix
        return self.matrix

    def update_matrix(self,source,rssi_value,neighbor,radio):#here we update the matrix with to value of rssi to which neighbor on which radio
        # to work on, get source id from adress and same for destination and radio to put value
        for i in range(len(self.matrix)):
            

            for j in range(len(self.matrix)):

                    if i==source and j==neighbor :

                        a = self.matrix[i][j][0] #il a fallu rajouter ces trois lignes pour initialiser l'index de la matrice voulue pour que ca marche
                        b = self.matrix[i][j][1]
                        c = self.matrix[i][j][2]


                        self.matrix[i][j] =[a,b,c]
                        self.matrix[i][j][radio] =rssi_value
                        print self.matrix[i][j]

                        break
        print self.matrix

        return 0


    def get_mote_index_by_address(self,byte0,byte1):

        for i in range(len(self.mote_index_map)):
                #print (int(self.mote_index_map[i][0],16), byte0), (int(self.mote_index_map[i][1],16), byte1)
                if(int(self.mote_index_map[i][0],16) == byte0 and int(self.mote_index_map[i][1],16) == byte1):
                    return i


#---------------------------MAIN APP---------------------------#

MOTE_IP = 'bbbb::12:4b00:14b5:d366'
UDPPORT = 61618 # can't be the port used in OV

NODES_IN_NETWORK = 2



#------------------------instantiation-------------------------#
netinfo = netmanager.NetManager()
monitor = Cmonitor()
#--------------------------------------------------------------#

#-------Init matrix, DagRank Table and motes index table-------#

#--------------------------------------------------------------#





print("\nWaiting for all nodes to be reachable  downward...")
motes_list = netinfo.get_motes_list()

#loop until get at least one reachable mote
while motes_list == [] or len(motes_list) != NODES_IN_NETWORK:
    del motes_list[:]
    motes_list = netinfo.get_motes_list()

#Now print list of available motes in the mesh network (downward RPL route)
print("\nAvailable down routes are for the following nodes:\n")

for i in range(len(motes_list)):
    print("ID: {} -- @: {}".format(i+1,motes_list[i]))

#get the last two bytes of a node address in string format    
for i in range(len(motes_list)):
    byte0 = motes_list[i][-4:-2]
    byte1 = motes_list[i][-2:]
    monitor.mote_index_map.append([byte0,byte1])
print monitor.mote_index_map

#-----------cerate and fill matrix-----------
matrix = monitor.init_matrix(motes_list)
#--------------------------------------------



#-------------Getting all the DAGranks-------------#
resource = 'dr'

for i in range(1,len(monitor.mote_index_map)):

    MOTE_IP = 'bbbb::12:4b00:14b5:'+monitor.mote_index_map[i][0]+monitor.mote_index_map[i][1]
    print "Asking mote: ",MOTE_IP, "for his DAGRank"

    c = coap.coap(udpPort=UDPPORT)
    p = c.GET('coap://[{0}]/m/dr'.format(MOTE_IP))
    c.close()

    DAGRank  = struct.unpack('<H',''.join([chr(c) for c in p]))[0]
    print DAGRank

    monitor.DAGRank_List.append(DAGRank)


print "DAGRank_List = ",monitor.DAGRank_List


#-------------Getting the LIST of Neighbors-------------#
resource = 'nl'

for i in range(1,len(monitor.mote_index_map)):
    
    neighborList = []
    MOTE_IP      = 'bbbb::12:4b00:14b5:'+monitor.mote_index_map[i][0]+monitor.mote_index_map[i][1]
    my_index     = monitor.get_mote_index_by_address(int(monitor.mote_index_map[i][0],16),int(monitor.mote_index_map[i][1],16))
    print "Asking mote: ",MOTE_IP, "for his neighborList"
    print "my index is ", my_index

    c = coap.coap(udpPort=UDPPORT)
    p = c.GET('coap://[{0}]/m/nl'.format(MOTE_IP))
    c.close()

    neighborsCount = p[-1]
    print "Number of neighbors is :",neighborsCount

    for i in range(0,len(p)-1,4):
        byte0 = p[i]
        byte1 = p[i+1]
        radio = p[i+2]
        rssi  = struct.unpack('<b',chr(p[i+3]))[0]#p[i+3] 

        #set a neighbor list
        neighborList.append([hex(byte0),hex(byte1),radio,rssi])

        #get the indext of the neighbor by his address
        neighbor_index = monitor.get_mote_index_by_address(byte0,byte1)
        print "index of node is", neighbor_index

        #update the matrix and set the RSSI values in the correct palce
        monitor.update_matrix(my_index,rssi,neighbor_index,radio)

    print neighborList

print monitor.matrix
# read the information about the board status
#when only sending path0 ---> results is for now list of available resources paths
# result: "</m/pr>,</m/nc>"

"""

resource = 'STOP'#'dr','pr','nc','nl'
c = coap.coap(udpPort=UDPPORT)

if resource== 'nc':
    p = c.GET('coap://[{0}]/m/nc'.format(MOTE_IP))
    print p
    print 'num nighbors = ',p[0]
    


if resource == 'dr':
    p = c.GET('coap://[{0}]/m/dr'.format(MOTE_IP))

    DAGRank  = struct.unpack('<H',''.join([chr(c) for c in p]))[0]
    print DAGRank

if resource == 'pr':
    p = c.GET('coap://[{0}]/m/pr'.format(MOTE_IP))
    byte0 = p[0]
    byte1 = p[1]
    radio = p[2]

    print [hex(byte0),hex(byte1),radio]

if resource == 'nl':
    
    neighborList = []
    p = c.GET('coap://[{0}]/m/nl'.format(MOTE_IP))
    neighborsCount = p[-1]
    print neighborsCount

    for i in range(0,len(p)-1,4):
        byte0 = p[i]
        byte1 = p[i+1]
        radio = p[i+2]
        rssi  = struct.unpack('<b',chr(p[i+3]))[0]#p[i+3] 

        neighborList.append([hex(byte0),hex(byte1),radio,rssi])
    print p
    print neighborList

"""    
while True:
        input = raw_input("Done. Press q to close. ")
        if input=='q':
            print 'bye bye.'
            #c.close()
            os.kill(os.getpid(), signal.SIGTERM)