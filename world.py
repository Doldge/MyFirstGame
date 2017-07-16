#! /usr/bin/python

import random
import tree_span
import uuid
import math
import os
import logging
import xml.etree.cElementTree as ET

"""
NB: The co-ordinates are a bit of a mess.
normally you do x, y but because of my use of
    line in world:
        y in line:
we've ended up with our references generally being backwards.
so to get a point on the map it's:
    matrix[y][x]
instead of the expected:
    matrix[x][y]

Because of this, I *generally* pass the y value in first.
'"""

#from collections import namedtuple
#Point = namedtuple('Point', ['x', 'y'])

logger = logging.getLogger('world')

TOP=(-1,0)
LEFT=(0,-1)
BOTTOM=(1,0)
RIGHT=(0,1)
DIRECTIONS = [
    TOP,
    LEFT,
    BOTTOM,
    RIGHT,
]


class World(object):

    def __init__(self, height=30, width=80):
        self.height = height
        self.width = width
        self.matrix = [ [Wall() for x in range(width)] for y in
        range(height) ]
        self.players = []
        self.rooms = []

    def reset(self):
        self.destroy()
        self.__init__(0,0)

    def destroy(self):
        # TODO: write a function that ensures all the
        # rooms/corridors/walls/player objects are destroyed,
        # without any lingering references
        pass

    def getMatrix(self):
        return self.matrix

    def getXML(self):
        xml_world = ET.Element('world',
            {
                'width': str(self.width),
                'height': str(self.height)
            }
        )
        player_group = ET.SubElement(xml_world, 'players',
            {
                'count': str(len(self.players))
            }
        )
        for player in self.players:
            player = ET.SubElement(player_group, 'player',
                {
                    'number': str(player.number),
                    'char': player.representation,
                    'name': player.name,
                    'x': str(player.x),
                    'y': str(player.y)
                }
            )

        room_group = ET.SubElement(xml_world, 'rooms',
            {
                'count': str(len(self.rooms))
            }
        )
        for room in self.rooms:
            room = ET.SubElement(room_group, 'room',
                {
                    'width': str(room.width),
                    'height': str(room.height),
                    'x': str(room.x),
                    'y': str(room.y)
                }
            )

        contents = ET.SubElement(xml_world,'contents')
        for row in self.getMatrix():
            r = ET.SubElement(contents, 'row')
            for item in row:
                if isinstance(item,Wall):
                    ET.SubElement(r, 'wall')
                elif isinstance(item,Gold):
                    ET.SubElement(r, 'gold',
                        {
                            'amount' : str(item.amount)
                        }
                    )
                elif isinstance(item, BaseTile):
                    ET.SubElement(r, 'basetile')
        return xml_world

    def getXMLasString(self):
        return ET.tostring(self.getXML())

    def getXMLPlayers(self):
        pass

    def createFromXML(self, xml):
        return self.fromXML(xml, True)

    def updateFromXML(self, xml):
        return self.fromXML(xml, False)

    def fromXML(self, xml, hard_reset=True):
        #Reset the world.
        xml_world = ET.fromstring(xml)
        #xml_world = tree.getroot()
        if hard_reset:
            self.reset()
            self.__init__(
                width=int(xml_world.get('width')),
                height=int(xml_world.get('height'))
            )

        #Re create rooms, but don't modify the matrix.
        for room in xml_world.iter('room'):
            r = Room(
                self,
                width=int(room.get('width')), height=int(room.get('height')),
                x=int(room.get('x')), y=int(room.get('y'))
            )
            r.dryrun()
            self.addRoom(r)

        #Fill in the Matrix
        x, y = 0, 0
        for row in xml_world.iter('row'):
            for item in row:
                if item.tag == 'gold':
                    self.getMatrix()[y][x] = Gold(int(item.get('amount')))
                elif item.tag == 'wall':
                    self.getMatrix()[y][x] = Wall()
                elif item.tag =='basetile':
                    self.getMatrix()[y][x] = BaseTile()
                x += 1
            y += 1
            x = 0

        #Add the players
        for player in xml_world.iter('player'):
            p = Player(
                player.get('number'),player.get('char'),
                name=player.get('name')
            )
            self.addPlayer(p, (int(player.get('y')), int(player.get('x'))) )

        return self

    def addObject(self, obj, position=()):
        if not position:
            position = self.randomCoords()
        current_tile = self.matrix[position[0]][position[1]]
        if current_tile.isEmpty():
            self.matrix[position[0]][position[1]] = obj
            return True
        return False

    def addPlayer(self, player, position=()):
        if not position:
            position = self.randomCoords()
        if self.addObject(player, position):
            player.setPosition(position[0],position[1])
            player.setWorld(self)
            self.players.append(player)
            return True
        return False

    def removePlayer(self, player_pos):
        #Player pos is the players integer number.
        logger.debug(player_pos)
        player = self.players[player_pos]
        y,x = player.getPosition()
        self.getMatrix()[y][x] = BaseTile()
        del(self.players[player_pos])
        return True

    def getPlayer(self, player_pos):
        return self.players[player_pos]

    def addRoom(self, room):
       self.rooms.append(room)

    def getRooms(self):
        return self.rooms

    def hasUnconnectedRooms(self):
        return True if [x for x in self.rooms if not x.isConnected()] else False

    def getUnconnectedRooms(self):
        return [ x for x in self.rooms if not x.isConnected()]

    def moveObject(self, obj, new_y, new_x):
        if (new_x < 0 or new_x > self.width-1) or\
                (new_y < 0 or new_y > self.width-1):
            return False
        cur_tile = self.matrix[new_y][new_x]
        if cur_tile.isEmpty() and not cur_tile.isWall():
            if obj.hasCoords():
                cur_y, cur_x = obj.getPosition()
                self.matrix[new_y][new_x] = obj
                self.matrix[cur_y][cur_x] = BaseTile()
                return True
        return False

    def randomCoords(self):
        x = int(random.uniform(0,self.width))
        y = int(random.uniform(0,self.height))
        if self.matrix[y][x].isEmpty() and not self.matrix[y][x].isWall():
            return y,x
        return self.randomCoords()



class BaseTile(object):

    def __init__(self, representation=' ', empty=True, wall=False, world=None):
        self.representation = representation
        self.empty = empty
        self.world = world
        self.wall = wall

    def __str__(self):
        return self.representation

    def __repr__(self):
        return self.representation

    def isEmpty(self):
        return self.empty

    def isWall(self):
        return self.wall

    def setWorld(self, world):
        self.world = world

    def hasCoords(self):
        return True if hasattr(self,'x') and hasattr(self,'y') else False


class MoveException(Exception):
    def __init__(self, message, pos):
        self.message = message
        self.pos = pos


class Player(BaseTile):

    def __init__(self, number=1, representation='&', uid=None, name=None):
        super(Player,self).__init__(representation, False)
        self.number = number
        self.uid = uid
        self.name = name
        self.x = None
        self.y = None

    def setPosition(self,y,x):
        self.x = x
        self.y = y

    def getPosition(self):
        return (self.y, self.x)


    def move(self, y, x):
        moved = self.world.moveObject(self, y, x)
        if moved:
            self.setPosition(y, x)
            return True
        raise MoveException("Can't move there", (y,x))


class Wall(BaseTile):
    def __init__(self, representation='#'):
        super(Wall,self).__init__(representation,True,True)


class Gold(BaseTile):
    def __init__(self, representation='G',amount=999):
        super(Gold,self).__init__(representation,True,True)
        self.amount = amount


class Room(object):
    def __init__(self, world, width=None, height=None, x=None,
            y=None):
        self.world = world
        self.width = width
        self.height = height
        self.x = x
        self.y = y
        self.walls = []
        self.connections = []
        # (These are vertices)
        self.corners = []
        #This isn't really a list of vectors,
        #it's just a point in the middle of the vector.
        self.vectors = []
        self.id = uuid.uuid4()
        self.center = None

    def build(self, dryrun=False):
        self.walls = []
        """
        #This only works for getting the outside of the room.
        for col in range(self.x, self.x+self.width+1):
                self.walls.append( (self.y, col) )
                self.walls.append( (self.y+self.height, col) )
        for row in range(self.y, self.y + self.height+1):
                self.walls.append( (row, self.x ) )
                self.walls.append( (row, self.x+self.width) )
        """
        #Carve out the room.
        for col in range(self.x, self.x+self.width+1):
            for row in range(self.y, self.y+self.height+1):
                self.walls.append( (row, col) )
        if not dryrun:
            for coords in self.walls:
                self.world.addObject(BaseTile(), coords)

        # Set the room corners
        self.corners = [
            (self.y, self.x),                           # Top Left
            (self.y, self.x+self.width),                # Top Right
            (self.y+self.height, self.x),               # Bottom Left
            (self.y+self.height, self.x+self.width)     # Bottom Right
        ]
        self.center = (
            ((self.corners[0][0]+self.corners[3][0])/2),
            ((self.corners[0][1]+self.corners[3][1])/2)
        )
        #self.world.addObject(
        #    BaseTile(str(self.world.getRooms().index(self))),
        #    self.center
        #)

        self.vectors = [
            (
                self.corners[0][0],
                ((self.corners[0][1]+self.corners[1][1])/2)
            ),      # Top
            (
                ((self.corners[0][0]+self.corners[2][0])/2),
                self.corners[0][1]
            ),      # Left
            (
                ((self.corners[2][0]+self.corners[3][0])/2),
                ((self.corners[2][1]+self.corners[3][1])/2)
            ),      # Bottom
            (
                ((self.corners[1][0]+self.corners[3][0])/2),
                ((self.corners[1][1]+self.corners[3][1])/2)
            )       # Right
        ]
        #for i, v in enumerate(self.vectors):
        #    self.world.addObject(BaseTile(str(i)), v)

    def dryrun(self):
        #Same as build but without adding objects to the world.
        self.build(True)

    def getWalls(self):
        return self.walls

    def isConnected(self):
        return True if self.connections else False

    def getId(self):
        #return self.id
        return self.world.getRooms().index(self)

    def __str__(self):
        return str(self.getId())

    def __repr__(self):
        return self.__str__()


def buildRooms(world, max_width, max_height, max_rooms=8, min_width=6, min_height=5):

        if not world or not max_width or not max_height:
            return False

        success_count = 0
        while (success_count < max_rooms):
            width = random.randrange(min_width, max_width, 1)
            height = random.randrange(min_height, max_height, 1)
            x = random.randrange(0, (world.width-(width+1)))
            y = random.randrange(0, (world.height-(height+1)))
            can_build = True
            for col in range(x, x+width+1):
                for row in range(y, y+height+1):
                    obj = world.getMatrix()[row][col]
                    if not obj.isEmpty() or not obj.isWall():
                        can_build = False
                        break;
                if not can_build:
                    break
            if can_build:
                room = Room(world, width, height, x=x, y=y)
                world.addRoom(room)
                room.build()
                success_count += 1


class CorridorBuilder(object):

    def __init__(self, world):
        self.world = world
        self.graph = tree_span.Graph()

    def generate(self):
        if not self.world:
            return
        room = None
        roomA = None
        rooms = self.world.getRooms()
        myrooms = list(rooms)
        checkedrooms = list(rooms)
        count = len(rooms)*len(rooms)
        while count > 0:
            if not myrooms and not checkedrooms:
                #Done
                break;
            if not roomA:
                roomA = myrooms.pop()
            elif not checkedrooms:
                checkedrooms = list(rooms)
                roomA = myrooms.pop()
            room = checkedrooms.pop()
            if room == roomA:
                continue
            g = self.overlap(roomA, room)
            #print( g )
            if g == True:
                if roomA not in room.connections:
                    room.connections.append(roomA)
                if room not in roomA.connections:
                    roomA.connections.append(room)
            count -= 1
        print(
            "CONNECTED ROOM COUNT: {}".format(
                len([ x for x in self.world.getRooms() if x.isConnected() ])
            )
        )

        for room in rooms:
            self.distance(room, rooms)
        #self.check(random.choice(rooms), rooms)
        self.growMaze((1,1))
        self.cleanUp()


    def overlap(self, room1, room2):
        if ( (room1.x+room1.width+1) < room2.x ):
            return False
        elif ( room1.x > (room2.x+room2.width+1) ):
            return False
        elif ( (room1.y+room1.height+1) < room2.y ):
            return False
        elif ( room1.y > (room2.y+room2.height+1) ):
            return False
        return True


    def distance(self, room, roomlist):

        for room2 in roomlist:
            if room == room2:
                continue
            a2 = (room.center[1]+1 - room2.center[1]+1)
            a2 = a2 * a2
            b2 = (room.center[0]+1 - room2.center[0]+1)
            b2 = b2 * b2
            #y_dist = abs(room.center[0]-room2.center[0])
            #x_dist = abs(room.center[1]-room2.center[1])
            self.graph.addEdge(
                str(room.getId()),
                str(room2.getId()),
                math.sqrt(a2+b2) if not self.overlap(room, room2) else 0
            )
        """
        for room2 in roomlist:
            if room == room2:
                continue
            for i in range(len(room.vectors)*len(room2.vectors)):
                v1 = room.vectors[i >> 2]
                v2 = room2.vectors[i & 3]
                y_dist = abs(v1[0]-v2[0])
                x_dist = abs(v1[1]-v2[1])
        """

    def growMaze(self, start):
        windingPercent = 15
        cells = []
        last_dir = None
        cells.append(start)
        while cells:
            cell = cells[len(cells)-1]

            unmade_cells = []
            for direction in DIRECTIONS:
                if self.canCarve(cell, direction):
                   unmade_cells.append(direction)

            if unmade_cells:
                if last_dir in unmade_cells and random.randrange(100) >\
                        windingPercent:
                    direction = last_dir
                else:
                    direction = random.choice(unmade_cells)

                self._carve(cell,direction)
                self._carve(cell, (direction[0]*2,direction[1]*2))
                cells.append( (cell[0]+direction[0]*2, cell[1]+direction[1]*2) )
                last_dir = direction

            else:
                cells.pop()
                last_dir = None

    def cleanUp(self):
        done = False
        while not done:
            done = True
            for x in range(1,self.world.width):
                for y in range(1,self.world.height):
                    if self.world.getMatrix()[y][x].isWall():
                        continue
                    exits = 0

                    for d in DIRECTIONS:
                        if (x+d[1]) < 0 or (y+d[0]) < 0 or\
                                (x+d[1]) >= self.world.width or (y+d[0]) >= self.world.height:
                            continue
                        if not self.world.getMatrix()[y+d[0]][x+d[1]].isWall():
                            exits += 1
                    if exits == 1:
                        done = False
                        self.world.addObject(Wall(), (y, x))


    def canCarve(self, cell, d):
        cell = (cell[0]+d[0]*2, cell[1]+d[1]*2)
        if cell[1]>= self.world.width or cell[0] >= self.world.height or cell[1] < 0 or cell[0] < 0:
            return False
        return self.world.getMatrix()[cell[0]][cell[1]].isWall()

    def _carve(self, cell, d):
        cell = (cell[0]+d[0], cell[1]+d[1])
        self.world.addObject(BaseTile(), cell)
        return cell



    def check(self, room, roomlist):
        print('Graph data for room: {}'.format(str(room.getId())))
        for v in self.graph:
            for w in v.getConnections():
                vid = v.getId()
                wid = w.getId()
                print('( %s , %s, %3d)' % (vid, wid, v.getWeight(w)))
        tree_span.dijkstra(
            self.graph,
            self.graph.getVertex(str(room.getId()))
        )
        for r in roomlist:
            t = str(r.getId())
            target = self.graph.getVertex(t)
            path = []
            tree_span.shortest(target, path)
            print('The shortest path for %s: %s' %(t, path[::-1]))

        """
        for r in roomlist:
            shortest_x = None
            shortest_path = None
            roomname = r.getId()
            for x in range(4):
                t = str(r.getId())+'::'+str(x)
                target = self.graph.getVertex(t)
                path = []
                tree_span.shortest(target, path)
                print('The shortest path for %s: %s' %(t, path[::-1]))
        """

def draw(world):
    os.system('clear')
    print("-"*(world.width+2))
    for line in world.getMatrix():
        print('|'+''.join([str(x) for x in line])+'|')
    print("-"*(world.width+2))


if __name__ == '__main__':
    world = World()
    buildRooms(world, 17, 11, 3)
    c = CorridorBuilder(world)
    c.generate()
    draw(world)
