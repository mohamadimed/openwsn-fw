
# -*- coding: utf-8 -*-
# Author: Cristian Bastidas
# GitHub: https://github.com/crixodia
# Date: 2020-10-7




class Djikstra():

	def __init__(self):
		
		pass
		return


	
	def best_rssi_among_radios(self,rssi_per_radio):
			best = 0
			index = 0
			for i in range(3):
				if rssi_per_radio[i]<0:

					best = rssi_per_radio[i]
					index = i
					break

			for i in range(0,len(rssi_per_radio)):
				if (rssi_per_radio[i] >= best and rssi_per_radio[i] < 0):
					index = i
					best = rssi_per_radio[i]
					# print rssi_per_radio[0], rssi_per_radio[1], rssi_per_radio[2]
					# print index, best
			return best,index #returning the best value and it's index


	def find_all(self,wmat, start, end=-1):
	    """
	    Returns a tuple with a distances' list and paths' list of
	    all remaining vertices with the same indexing.

	        (distances, paths)

	    For example, distances[x] are the shortest distances from x
	    vertex which shortest path is paths[x]. x is an element of
	    {0, 1, ..., n-1} where n is the number of vertices

	    Args:
	    wmat    --  weighted graph's adjacency matrix
	    start   --  paths' first vertex
	    end     --  (optional) path's end vertex. Return just the 
	                distance and its path

	    Exceptions:
	    Index out of range, Be careful with start and end vertices
	    """
	    n = len(wmat)

	    # print "M=",wmat
	    # print "S=",start
	    #dist = [inf]*n

	    dist = [-999]*n
	    dist[start] = self.best_rssi_among_radios(wmat[start][start])[0]  # 0


	    spVertex = [False]*n
	    parent_value = [-1]*n
	    parent_index = [-1]*n


	    path = [{}]*n
	    radio = []

	    for count in range(n-1):
	        minix = -999 #inf

	        #minix = 999 #inf
	        u = 0
	        
	        
	        for v in range(len(spVertex)):

	            
	            # print "-------",dist[v]

	            if spVertex[v] == False and dist[v] >= minix:
	            	# print 'Before: minx = ',minix, ' dist[',v,']= ',dist[v]
	                minix = dist[v]
	                # print 'After: minx = ',minix
	                u = v
	            
	            	
        	spVertex[u] = True
        	for v in range(n):
            		wmat_value = self.best_rssi_among_radios(wmat[u][v])[0]
            		wmat_index = self.best_rssi_among_radios(wmat[u][v])[1]

            		# print dist[u] , wmat_value , dist[v]
            		if not(spVertex[v]) and wmat[u][v] != [0,0,0]  and  dist[u] + wmat_value > dist[v]:
            			# print wmat_value, wmat_index
                		parent_value[v] = u
                		# print "IAM HERE"
                		parent_index[v] = wmat_index
                		dist[v] = dist[u]+wmat_value

                		#mark point as selected 
                		wmat[u][v][wmat_index] = 0
                		# print dist[v]
                		
	    
	    
	    for i in range(n):
	        j = i
	        s = []
	        while parent_value[j] != -1:
	            s.append(j)
	            j = parent_value[j]
       
	        s.append(start)
	        path[i] = s[::-1]


	    for i in range(1,len(path[end])):
	    	radio.append(parent_index[path[end][i]])


	    # print parent_index
	    # print radio
	    # print "M=",wmat
	    return (dist[end], path[end],radio) if end >= 0 else (dist, path)


	# def find_shortest_path(self, wmat, start, end=-1):
	#     """
	#     Returns paths' list of all remaining vertices.

	#     Args:
	#     wmat    --  weigthted graph's adjacency matrix
	#     start   --  paths' first vertex
	#     end     --  (optional) path's end vertex. Return just
	#                 the path

	#     Exceptions:
	#     Index out of range, Be careful with start and end vertices.
	#     """
	#     return find_all(wmat, start, end)[1]


	# def find_shortest_distance(self, wmat, start, end=-1):
	#     """
	#     Returns distances' list of all remaining vertices.

	#     Args:
	#     wmat    --  weigthted graph's adjacency matrix
	#     start   --  paths' first vertex
	#     end     --  (optional) path's end vertex. Return just
	#                 the distance

	#     Exceptions:
	#     Index out of range, Be careful with start and end vertices.
	#     """
	#     return find_all(wmat, start, end)[0]


# wmat = [[[0,0,0], [3,4,2], [0,0,0], [0,0,0],[0,0,0], [1,2,3], [0,0,0], [0,0,0]],
#         [[2,3,4], [0,0,0], [3,2,4], [2,3,4], [4,5,6], [0,0,0], [0,0,0], [0,0,0]],
#         [[0,0,0], [2,3,4], [0,0,0], [0,0,0], [3,4,5], [0,0,0], [0,0,0], [2,1,3]],
#         [[0,0,0], [2,3,4], [0,0,0], [0,0,0], [4,5,6], [3,4,5], [0,0,0], [0,0,0]],
#         [[0,0,0], [4,5,6], [3,4,5], [4,5,6], [0,0,0], [0,0,0], [7,8,9], [0,0,0]],
#         [[1,2,3], [0,0,0], [0,0,0], [3,4,5], [0,0,0], [0,0,0], [5,6,7], [0,0,0]],
#         [[0,0,0], [0,0,0], [0,0,0], [0,0,0], [7,8,9], [5,6,7], [0,0,0], [6,7,8]],
#         [[0,0,0], [0,0,0], [1,2,3], [0,0,0], [0,0,0], [0,0,0], [6,7,8], [0,0,0]]]

# wmat = [[[0,0,0],    [0,0,0],[-80,-97,-100],[0,0,0],    [0,0,0]], #node 0
#  		      						[[-95, 0,-83],[0,0,0],    [-80,-97,-100],[-83,-95,-101],[0,0,0]], #node 1 --> 0()
#  		      						[[-83,-95,-101],[-80,-97,-100],[0,0,0],    [-81,-99,-94], [0,0,0]], #node 2 --> 
#  		      						[[0,0,0],    [-95,-80,-92], [-81,-99,-94], [0,0,0],    [-83,-95,-101]], #node 3 --> 1(FSK)
#  		      						[[0,0,0],    [0,0,0],    [0,0,0],    [-80,-97,-100],[0,0,0]]] #node 4 --> 3(QPSK)

# wmat = [[0, 2, 0, 0, 0, 1, 0, 0],
#         [2, 0, 2, 2, 4, 0, 0, 0],
#         [0, 2, 0, 0, 3, 0, 0, 1],
#         [0, 2, 0, 0, 4, 3, 0, 0],
#         [0, 4, 3, 4, 0, 0, 7, 0],
#         [1, 0, 0, 3, 0, 0, 5, 0],
#         [0, 0, 0, 0, 7, 5, 0, 6],
#         [0, 0, 1, 0, 0, 0, 6, 0]]

# wmat =  [[[0,0,0], [0,0,0], [0,0,0], [0,0,0]], [[-52, -45, -47], [0,0,0], [-20, -23, -27], [-39, -44, -48]], [[-05, 0, 0], [0,0,0], [0,0,0], [-41, -44, -49]], [[-34, -27, -29], [-42, -42, -46], [-38, -43, -46], [0,0,0]]]



# djikstra = Djikstra()

# print(djikstra.find_all(wmat, 0))

# print(djikstra.find_all(wmat, 4,0))
# print(djikstra.find_all(wmat, 2,0))



###############################################################################
"""
import sys
 
class Graph(object):
    
    def __init__(self, nodes, init_graph):
        self.nodes = nodes
        self.graph = self.construct_graph(nodes, init_graph)
     	 

    def construct_graph(self, nodes, init_graph):
        '''
        This method makes sure that the graph is symmetrical. In other words, if there's a path from node A to B with a value V, there needs to be a path from node B to node A with a value V.
        '''
        graph = {}
        for node in nodes:
            graph[node] = {}
        
        graph.update(init_graph)
        
        for node, edges in graph.items():
            for adjacent_node, value in edges.items():
                if graph[adjacent_node].get(node, False) == False:
                    graph[adjacent_node][node] = value
        print graph            
        return graph
    @staticmethod
    def test():
		print 'ok'
    def get_nodes(self):
        "Returns the nodes of the graph."
        return self.nodes
    
    def get_outgoing_edges(self, node):
        "Returns the neighbors of a node."
        connections = []
        for out_node in self.nodes:
            if self.graph[node].get(out_node, False) != False:
                connections.append(out_node)
        return connections
    
    def value(self, node1, node2):
        "Returns the value of an edge between two nodes."
        return self.graph[node1][node2]

    @staticmethod
    def dijkstra_algorithm(graph, start_node):
	    unvisited_nodes = list(graph.get_nodes())
	 
	    # We'll use this dict to save the cost of visiting each node and update it as we move along the graph   
	    shortest_path = {}
	 
	    # We'll use this dict to save the shortest known path to a node found so far
	    previous_nodes = {}
	 
	    # We'll use max_value to initialize the "infinity" value of the unvisited nodes   
	    max_value = sys.maxsize
	    for node in unvisited_nodes:
	        shortest_path[node] = max_value
	    # However, we initialize the starting node's value with 0   
	    shortest_path[start_node] = 0
	    
	    # The algorithm executes until we visit all nodes
	    while unvisited_nodes:
	        # The code block below finds the node with the lowest score
	        current_min_node = None
	        for node in unvisited_nodes: # Iterate over the nodes
	            if current_min_node == None:
	                current_min_node = node
	            elif shortest_path[node] < shortest_path[current_min_node]:
	                current_min_node = node
	                
	        # The code block below retrieves the current node's neighbors and updates their distances
	        neighbors = graph.get_outgoing_edges(current_min_node)
	        for neighbor in neighbors:
	            tentative_value = shortest_path[current_min_node] + graph.value(current_min_node, neighbor)
	            if tentative_value < shortest_path[neighbor]:
	                shortest_path[neighbor] = tentative_value
	                # We also update the best path to the current node
	                previous_nodes[neighbor] = current_min_node
	 
	        # After visiting its neighbors, we mark the node as "visited"
	        unvisited_nodes.remove(current_min_node)
	    
	    return previous_nodes, shortest_path


    @staticmethod
    def print_result(previous_nodes, shortest_path, start_node, target_node):

	    path = []
	    node = target_node
	    
	    while node != start_node:
	        path.append(node)
	        node = previous_nodes[node]
	 
	    # Add the start node manually
	    path.append(start_node)
	    
	    print("We found the following best path with a value of {}.".format(shortest_path[target_node]))
	    print(" -> ".join(reversed(path)))



nodes = ["Reykjavik", "Oslo", "Moscow", "London", "Rome", "Berlin", "Belgrade", "Athens"]
 
init_graph = {}
for node in nodes:
    init_graph[node] = {}
    
init_graph["Reykjavik"]["Oslo"] = 5
init_graph["Reykjavik"]["London"] = 4
init_graph["Oslo"]["Berlin"] = 1
init_graph["Oslo"]["Moscow"] = 3
init_graph["Moscow"]["Belgrade"] = 5
init_graph["Moscow"]["Athens"] = 4
init_graph["Athens"]["Belgrade"] = 1
init_graph["Rome"]["Berlin"] = 2
init_graph["Rome"]["Athens"] = 2

graph = Graph(nodes, init_graph)

previous_nodes, shortest_path = Graph.dijkstra_algorithm(graph=graph, start_node="Reykjavik")

Graph.print_result(previous_nodes, shortest_path, start_node="Reykjavik", target_node="Belgrade")
"""
##############################################################################################

"""
import sys
 
class Graph():
 
    def __init__(self, vertx):
        self.V = vertx
        self.graph = [[0 for column in range(vertx)]
                      for row in range(vertx)]
 
    def pSol(self, dist):
        print("Distance of vertex from source")
        for node in range(self.V):
            print(node, "t", dist[node])
 

    def minDistance(self, dist, sptSet):
 

        min = sys.maxsize
 

        for v in range(self.V):
            if dist[v] < min and sptSet[v] == False:
                min = dist[v]
                min_index = v
 
        return min_index
 

    def dijk(self, source):
 
        dist = [sys.maxsize] * self.V
        dist[source] = 0
        sptSet = [False] * self.V
 
        for cout in range(self.V):
 
            u = self.minDistance(dist, sptSet)
 
            sptSet[u] = True
 

            for v in range(self.V):
                if self.graph[u][v] > 0 and  sptSet[v] == False and  dist[v] > dist[u] + self.graph[u][v]:
                    dist[v] = dist[u] + self.graph[u][v]
 
        self.pSol(dist)

f = Graph(9)
f.graph = [[0, 4, 0, 0, 0, 0, 0, 8, 0],
           [4, 0, 8, 0, 0, 0, 0, 11, 0],
           [0, 8, 0, 7, 0, 4, 0, 0, 2],
           [0, 0, 7, 0, 9, 14, 0, 0, 0],
           [0, 0, 0, 9, 0, 10, 0, 0, 0],
           [0, 0, 4, 14, 10, 0, 2, 0, 0],
           [0, 0, 0, 0, 0, 2, 0, 1, 6],
           [8, 11, 0, 0, 0, 0, 1, 0, 7],
           [0, 0, 2, 0, 0, 0, 6, 7, 0]
           ]
 
f.dijk(0)
"""