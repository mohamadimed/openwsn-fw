import os
import sys
here = sys.path[0]
print here
import signal
sys.path.insert(0,os.path.join(here,'..','..','..','coap'))

from coap import coap
import struct
import netmanager
import time
import djikstra as djkstra
import numpy as np
import matplotlib.pyplot as plt
import subprocess
#---------------------------CLASS AND METHODS DEF-----------------------#
class Cmonitor():

    DAGRank_List    = [256]
    mote_index_map  = []
    matrix          = []
    ingress         = 0
    egress          = 0
    root_address    = '12:4b00:14b5:d319'
    NODES_IN_NETWORK = 2 #Important to set it correct, according  the number of nodes in the network
    list_of_subTracks = []
    schedule          = []
    schedule_len        = 101
    available_channels  = 16

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

        #print self.matrix
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
                        #print self.matrix[i][j]

                        break
        #print self.matrix

        return 0


    def get_mote_index_by_address(self,byte0,byte1):

        for i in range(len(self.mote_index_map)):
                #print (int(self.mote_index_map[i][0],16), byte0), (int(self.mote_index_map[i][1],16), byte1)
                if(int(self.mote_index_map[i][0],16) == byte0 and int(self.mote_index_map[i][1],16) == byte1):
                    return i


    def get_mote_address_by_index(self,index):

        for i in range(len(self.mote_index_map)):
            if (index == i):
                    return [self.mote_index_map[i][0],self.mote_index_map[i][1]]

    def best_rssi_among_radios(self,rssi_per_radio):
            best = rssi_per_radio [0]
            index = 0
            for i in range(1,len(rssi_per_radio)):
                if rssi_per_radio[i] > best: #here we get the minimum (test is done with positif values)
                    best = rssi_per_radio[i]
                    index = i

            return index #here we return the index of the best radio to be used


    def create_track_native(self): #using greedy heuristic
                                                    #Need to define a way to set the used royte to zero/high number so it won't be chosen when calculating 'n' route
        current_node = self.ingress
        subTrack = []

        while (current_node != self.egress):
                        
            for i in  range(self.NODES_IN_NETWORK):
                
                if(self.matrix[current_node][i]!=[-999,-999,-999]):
                    #we have a neighbor

                    #we should test if our DagRank is lower then it so we don't consider it as a neighbor (goes down)
                    if (self.DAGRank_List[current_node] < self.DAGRank_List [i]):
                        continue; #don't consider it as a neighbor

                    else:
                        node = i
                        radio = self.best_rssi_among_radios(self.matrix[current_node][i])
                        self.matrix[current_node][i][radio] = -999 # set value of link to 0 when select this link to be part of the track
                        subTrack.append([self.get_mote_address_by_index(i),radio]) #next step of track [next node @, node radio] to beused for track creation
                        # print ("next step is node {} on radio {}".format(self.get_mote_address_by_index(i),radio))
                        current_node = node #later i need to do mapping between node index and node address
                        


                else: continue; #we dont have a neighbor
        return subTrack

    def create_track_djikstra(self): #using Djikstra Algo.

        subTrack = []
        djikstra = djkstra.Djikstra() #instanciation of Djikstra class

        result = djikstra.find_all(self.matrix,self.ingress,self.egress) #run algo and get results
        
        succession = result[1] #track succession nodes
        radio      = result[2] #communication radio succession

        # print "path=",succession
        # print "radio=",radio

        for i in range(1,len(succession)):

            subTrack.append([self.get_mote_address_by_index(succession[i]),radio[i-1]]) #next step of track [next node @, node radio] to beused for track creation

        return subTrack


    def init_schedule(self):
        self.schedule = np.array([[0 for _ in range (self.available_channels)] for _ in range(self.schedule_len)])

        return self.schedule


#---------------------------MAIN APP---------------------------#

MOTE_IP = 'bbbb::12:4b00:14b5:d366'
UDPPORT = 61618 # can't be the port used in OV





#------------------------instantiation-------------------------#
netinfo = netmanager.NetManager()
monitor = Cmonitor()
#--------------------------------------------------------------#

#-------Init matrix, DagRank Table and motes index table-------#

#--------------------------------------------------------------#





print("\nWaiting for all nodes to be reachable  downward...")
motes_list = netinfo.get_motes_list()

#loop until get at least one reachable mote
while motes_list == [] or len(motes_list) != (monitor.NODES_IN_NETWORK-1):
    del motes_list[:]
    motes_list = netinfo.get_motes_list()



#Now print list of available motes in the mesh network (downward RPL route)
print("\nAvailable down routes are for the following nodes:\n")

#insert the address of the root before printing it
motes_list.insert(0,monitor.root_address)

for i in range(len(motes_list)):
    
    print("ID: {} -- @: {}".format(i,motes_list[i]))
    
#get the last two bytes of a node address in string format    
for i in range(len(motes_list)):
    byte0 = motes_list[i][-4:-2]
    byte1 = motes_list[i][-2:]
    monitor.mote_index_map.append([byte0,byte1])
print monitor.mote_index_map

#-----------cerate and fill matrix-----------
matrix = monitor.init_matrix(motes_list)

# monitor.matrix = [[[0,0,0], [0,0,0], [0,0,0], [0,0,0]], [[-52, -45, -47], [0,0,0], [-20, -23, -27], [-39, -44, -48]], [[-05, -47, 0], [0,0,0], [0,0,0], [-41, -44, -49]], [[-34, -27, -29], [-42, -42, -46], [-38, -43, -46], [0,0,0]]]
#--------------------------------------------

"""

#-------------Getting all the DAGranks-------------#
resource = 'dr'

for i in range(1,len(monitor.mote_index_map)):

    MOTE_IP = 'bbbb::12:4b00:14b5:'+monitor.mote_index_map[i][0]+monitor.mote_index_map[i][1]
    print "Asking mote: ",MOTE_IP, "for his DAGRank"

    c = coap.coap(udpPort=UDPPORT)
    p = c.GET('coap://[{0}]/m/dr'.format(MOTE_IP))
    
    c.close()
    time.sleep(5)

    DAGRank  = struct.unpack('<H',''.join([chr(c) for c in p]))[0]
    print DAGRank

    monitor.DAGRank_List.append(DAGRank)


print "DAGRank_List = ",monitor.DAGRank_List

"""
#-------------Init the Schedule matrix-------------#
schedule = monitor.init_schedule()
#-------------Getting the Schedule-------------#
resource = 'cl'

for i in range(1,len(monitor.mote_index_map)):
    
    cellList     = []
    MOTE_IP      = 'bbbb::12:4b00:14b5:'+monitor.mote_index_map[i][0]+monitor.mote_index_map[i][1]
    my_index     = monitor.get_mote_index_by_address(int(monitor.mote_index_map[i][0],16),int(monitor.mote_index_map[i][1],16))
    print "Asking mote: ",MOTE_IP, "for his CellList"
    #print "my index is ", my_index
    p = []
    y = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]
    while (p == []):
        print '...'
        c = coap.coap(udpPort=UDPPORT)
        p = c.GET('coap://[{0}]/m/cl'.format(MOTE_IP))
        c.close()
        time.sleep(15)

    CellsCount = 0

    for i in range(0,len(p),7):
        slot    = p[i]
        channel = p[i+1]
        is_hard = p[i+2] # 0: Soft Cell/ 1: Hard Cell
        link_op = p[i+3] # 0: OFF / 1: TX / 2: RX / 3:TXRX
        byte0   = p[i+4]
        byte1   = p[i+5]
        radio   = p[i+6]

        CellsCount=CellsCount+1

        #set a neighbor list
        cellList.append([slot,channel,is_hard,link_op,hex(byte0),hex(byte1),radio])
        if radio == 0:
            monitor.schedule[slot][channel]   = 4
            monitor.schedule[slot+1][channel] = 4
        if radio == 1:
            monitor.schedule[slot][channel]   = 7
            monitor.schedule[slot+1][channel] = 7
            monitor.schedule[slot+2][channel] = 7
            monitor.schedule[slot+3][channel] = 7           
        if radio == 2:
            monitor.schedule[slot][channel]   = 9
            monitor.schedule[slot+1][channel] = 9         
    print "Number of available cells is :",CellsCount
    print cellList
    plt.imshow(schedule.T, aspect='auto',cmap="rainbow")#imshow#Paired
    plt.axis('off')#[0,100,0,15]
    plt.savefig('C:\\Users\\bmhg9130\\Desktop\\test1.png')
    plt.pause(0.001)
    #plt.show()
#-------------Getting the LIST of Neighbors & filling adjency matrix-------------#
resource = 'nl'

for i in range(1,len(monitor.mote_index_map)):
    
    neighborList = []
    MOTE_IP      = 'bbbb::12:4b00:14b5:'+monitor.mote_index_map[i][0]+monitor.mote_index_map[i][1]
    my_index     = monitor.get_mote_index_by_address(int(monitor.mote_index_map[i][0],16),int(monitor.mote_index_map[i][1],16))
    print "Asking mote: ",MOTE_IP, "for his neighborList"
    #print "my index is ", my_index
    p = []

    while (p == []):
        print '...'
        c = coap.coap(udpPort=UDPPORT)
        p = c.GET('coap://[{0}]/m/nl'.format(MOTE_IP))
        c.close()
        time.sleep(15)
#format:[[byte0,byte1,radio,rssi] x num_neighbors, num_neighbors]

    neighborsCount = p[-1]
    #print "Number of neighbors is :",neighborsCount

    for i in range(0,len(p)-1,4):
        byte0 = p[i]
        byte1 = p[i+1]
        radio = p[i+2]
        rssi  = struct.unpack('<b',chr(p[i+3]))[0]#p[i+3] 

        #set a neighbor list
        neighborList.append([hex(byte0),hex(byte1),radio,rssi])

        #get the indext of the neighbor by his address
        neighbor_index = monitor.get_mote_index_by_address(byte0,byte1)
        #print "index of node is", neighbor_index

        #update the matrix and set the RSSI values in the correct palce
        monitor.update_matrix(my_index,rssi,neighbor_index,radio)

    print neighborList


print 'befor',monitor.matrix


#---------------------generating the best TRACK----------------------------#
print "\n\nWe have all the necessary materials to process the best Track.\n\
please indicate the Ingress mote index among the following :"

for i in range(0,len(monitor.mote_index_map)):

    print i,  '= bbbb::12:4b00:14b5:'+monitor.mote_index_map[i][0]+monitor.mote_index_map[i][1] 

choice_ingress = input("Select the source node index\n")

monitor.ingress=choice_ingress

choice_egress = input("Select the destination node index\n")

monitor.egress=choice_egress


nb_subTracks = input("Please indicate the number of Sub-Tracks to process\n")

for i in range(nb_subTracks):
    #monitor.list_of_subTracks.append(monitor.create_track_native())
    monitor.list_of_subTracks.append(monitor.create_track_djikstra())

print 'list of subtrack',(monitor.list_of_subTracks)

for i in range(len(monitor.list_of_subTracks)):
     print 'TRACK',i,'  = ',monitor.list_of_subTracks[i]


#---------------------CREATING the best TRACK----------------------------#
#-----------------------------Code Generation----------------------------#


#init the necessary params

selected_ingress_adr = [999]
selected_ingress_adr.insert(0,monitor.get_mote_address_by_index(monitor.ingress))
selected_egress_adr  = monitor.get_mote_address_by_index(monitor.egress)

# print selected_ingress_adr

addOrUpdate = 1
bundle = 1

for t in range(1,nb_subTracks+1): #strat from 1 just to enable subtrack ID correctly[must start with 1]
    monitor.list_of_subTracks[t-1].insert(0,selected_ingress_adr)
    code_to_execute = ''        

    code_to_execute ='print(\'executing request {0}\'.format(t))\n'

    

    

    track = monitor.list_of_subTracks[t-1]
    
    for i in range(len(track)-1):
        code_to_execute +='c = coap.coap(udpPort=UDPPORT)\n'
        # code_to_execute +='access = []\n'
        # code_to_execute +='while (access == []):\n'
        code_to_execute += 'p = c.PUT(\'coap://[{0}]/ci\'.format(\'bbbb:0:0:0:12:4b00:14b5:' #indentation if enable above line

        
                             #[i][0]+     [i][1]
        code_to_execute+=track[i][0][0]+track[i][0][1]+'\'),True,[],[1,'+str(t)+\
        ',0x'+selected_ingress_adr[0][0]+',0x'+selected_ingress_adr[0][1]+',0x'+selected_egress_adr[0]+',0x'+selected_egress_adr[1]+','+str(bundle)

        code_to_execute+=',0x'+track[i+1][0][0]+',0x'+track[i+1][0][1]+','+str(track[i+1][1])+','+str(addOrUpdate)+'])\n'

        # code_to_execute+='access = p\n'             #indentation if enable above line
        code_to_execute+='print(\'{0}\'.format(p))\n' #indentation if enable above line 
        code_to_execute+='c.close()\n' 
        code_to_execute+='time.sleep(5)\n'
        
    

    print code_to_execute

    ok = input("Please press '1' to execute the generated code for track creation\n")

    print 'executing the generated code...'

    exec(code_to_execute)


#-------------------------------------------------------------------------#

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