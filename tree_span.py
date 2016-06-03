#! /usr/bin/python

#Tree spanning Alg

# Currently unused.
# Could be used for generating efficient connections between points.
# And navigating units around the map / etc

import sys
import heapq

class Vertex(object):

    def __init__(self, node):
        self.id = node
        self.adjacent = {}
        # Distance to infinity for all nodes
        self.distance = sys.maxint
        self.visited = False
        self.previous = None

    def addNeighbor(self, neighbor, weight=0):
        self.adjacent[neighbor] = weight

    def getConnections(self):
        return self.adjacent.keys()

    def getId(self):
        return self.id

    def getWeight(self, neighbor):
        return self.adjacent[neighbor]

    def setDistance(self, dist):
        self.distance = dist

    def getDistance(self):
        return self.distance

    def setPrevious(self, prev):
        self.previous = prev

    def getPrevious(self):
        return self.previous

    def setVisited(self):
        self.visited = True

    def __str__(self):
        return str(self.id) + ' adjacent: '+str([x.id for x in self.adjacent])


class Graph(object):

    def __init__(self):
        self.vert_dict = {}
        self.num_vertices = 0

    def __iter__(self):
        return iter(self.vert_dict.values())

    def addVertex(self, node):
        self.num_vertices += 1
        new_vertex = Vertex(node)
        self.vert_dict[node] = new_vertex
        return new_vertex

    def getVertex(self, n):
        if n in self.vert_dict:
            return self.vert_dict[n]
        return None

    def addEdge(self, frm, to, cost=0):
        if frm not in self.vert_dict:
            self.addVertex(frm)
        if to not in self.vert_dict:
            self.addVertex(to)

        self.vert_dict[frm].addNeighbor(self.vert_dict[to], cost)
        self.vert_dict[to].addNeighbor(self.vert_dict[frm], cost)

    def getVertices(self):
        return self.vert_dict.keys()



def shortest(v, path):
    if v.previous:
        path.append(v.previous.getId())
        shortest(v.previous, path)
    return


def dijkstra(graph, start):
    #Sets start node distance 0
    start.setDistance(0)

    #create priority heap/queue
    unvisited_queue = [(v.getDistance(), v) for v in graph]
    heapq.heapify(unvisited_queue)

    while len(unvisited_queue):
        # Get smallest vertex
        uv = heapq.heappop(unvisited_queue)
        current = uv[1]
        current.setVisited()

        for next in current.adjacent:
            if next.visited:
                continue
            new_dist = current.getDistance() + current.getWeight(next)

            if new_dist < next.getDistance():
                next.setDistance(new_dist)
                next.setPrevious(current)

        while len(unvisited_queue):
            heapq.heappop(unvisited_queue)

        unvisited_queue = [
            (v.getDistance(), v) for v in graph if not v.visited
        ]
